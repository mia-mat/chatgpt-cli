//
// Created by mia on 8/15/25.
//

#include "openai-wrapper.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>

#include "history.h"

#define OPENAI_RESPONSES_API_URL "https://api.openai.com/v1/responses"

void openai_request_free(openai_request* request) {
	free(request->api_key);
	free(request->input);
	free(request->model);
	free(request->instructions);
	free(request->previous_response_id);
	free(request);
}

typedef struct {
	openai_delta_callback callback; // pointer to a caller defined function
	void* user_data;
	char* error; // NULL if no error

	openai_request* request;

	// chunks sent back aren't guaranteed to contain a full response so store unfinished here
	char* current_event_buffer;
	size_t current_event_buffer_length;
} curl_callback_stream_callback_data; // to pass as data into the CURL callback

static json_object* curl_callback_openai_stream_extract_data_json(json_tokener* tok, const char* event_name_end,
                                                                  const char* event_end,
                                                                  curl_callback_stream_callback_data* callback_data) {
	const char* json_prefix = "data: "; // after event_name_end and before the JSON
	const char* json_string = strstr(event_name_end + strlen(json_prefix) + 1, "{");
	const size_t json_string_length = event_end - json_string;

	json_tokener_reset(tok);
	json_object* data_json = json_tokener_parse_ex(tok, json_string, (int)json_string_length);

	if (data_json == NULL) {
		fprintf(stderr, "\nJSON parse error: %s\n", json_tokener_error_desc(json_tokener_get_error(tok)));
		json_tokener_free(tok);
		callback_data->error = strdup("Malformed response (JSON)");
		return NULL;
	}

	return data_json;
}

// returns number of bytes handled, as specified in CURL docs
static size_t curl_callback_openai_stream_response(const char* content_ptr, const size_t size_atomic,
                                                   const size_t n_elements,
                                                   curl_callback_stream_callback_data** user_callback_addr) {
	const size_t total_chunk_size = size_atomic * n_elements; // = length of content_ptr
	curl_callback_stream_callback_data* callback_data = *user_callback_addr;

	// don't process further if we've already seen an error
	if (callback_data->error != NULL) {
		return total_chunk_size;
	}

	json_tokener* tok = json_tokener_new();

	// if OpenAI immediately errors, it just returns a full JSON object, check for this:
	json_object* potential_error = json_tokener_parse_ex(tok, content_ptr, (int)total_chunk_size);
	if (potential_error != NULL) {
		const json_object* error_json = json_object_object_get(potential_error, "error");
		if (error_json != NULL) {
			callback_data->error =
				strdup(json_object_get_string(json_object_object_get(error_json, "message")));
			json_object_put(potential_error);
			return 0;
		}
	}

	// copy new chunk onto buffer
	const size_t new_event_length = callback_data->current_event_buffer_length + total_chunk_size;
	char* new_event_buffer = realloc(callback_data->current_event_buffer, new_event_length);
	if (!new_event_buffer) {
		callback_data->error = strdup("Failed to allocate memory for delta");
		return 0;
	}
	callback_data->current_event_buffer = new_event_buffer;
	memcpy(callback_data->current_event_buffer + callback_data->current_event_buffer_length, // append to end
	       content_ptr, total_chunk_size);
	callback_data->current_event_buffer_length = callback_data->current_event_buffer_length + total_chunk_size;

	// complete events are formatted:
	// event: whatever.event\ndata: {some json object}\n\n
	// any \n's etc. inside of the json are escaped which makes parsing much easier
	// i.e. delimiting by \n\n should be safe.

	char* next_event_start = callback_data->current_event_buffer;
	while (true) {
		char* event_end = strstr(next_event_start, "\n\n");
		if (!event_end) break; // event isn't complete, split and wait for new chunks to finish it

		// get event type
		char* event_name_start = strstr(next_event_start, "event: ") + strlen("event: ");
		char* event_name_end = strstr(next_event_start, "\n");
		if (!event_name_start || !event_name_end || event_name_end < event_name_start) {
			callback_data->error = strdup("Malformed response");
			return 0;
		}
		const size_t event_name_length = (event_name_end) - (event_name_start);

		// refer to https://platform.openai.com/docs/api-reference/responses_streaming/response for event details
		#define IS_EVENT(ev) \
			(strlen(ev) == event_name_length && \
			strncmp(ev, event_name_start, strlen(ev)) == 0)

		if (callback_data->request->echo_response_id
			&& !(callback_data->request->raw) && IS_EVENT("response.created")) {
			json_object* data_json = curl_callback_openai_stream_extract_data_json(tok, event_name_end, event_end, callback_data);
			if (data_json == NULL) {
				return 0;
			}

			json_object* response_json = json_object_object_get(data_json, "response");

			printf("# Response ID: %s\n", json_object_get_string(json_object_object_get(response_json, "id")));
			json_object_put(data_json);
		}

		if (!callback_data->request->raw && // don't pass deltas to raw responses, we just pass the final full output
			IS_EVENT("response.output_text.delta")) {
			// extract delta
			json_object* data_json = curl_callback_openai_stream_extract_data_json(
				tok, event_name_end, event_end, callback_data);
			if (data_json == NULL) {
				return 0;
			}

			json_object* delta_json = json_object_object_get(data_json, "delta");

			callback_data->callback(strdup(json_object_get_string(delta_json)), json_object_get_string_len(delta_json),
			                        callback_data->user_data);

			json_object_put(data_json);
		}

		// the final event is only sent once at the end, just 'stream' back the whole json here instead of in deltas
		if (IS_EVENT("response.completed") || IS_EVENT("response.incomplete") || IS_EVENT("response.failed")) {
			json_object* data_json = curl_callback_openai_stream_extract_data_json(
				tok, event_name_end, event_end, callback_data);
			if (data_json == NULL) {
				return 0;
			}

			json_object* response_json = json_object_object_get(data_json, "response");

			if (callback_data->request->raw) {
				const char* response = json_object_to_json_string_ext(response_json, JSON_C_TO_STRING_PRETTY);
				callback_data->callback(strdup(response), strlen(response),
										callback_data->user_data);
			}

			char* resp_id = strdup(json_object_get_string(json_object_object_get(response_json, "id")));
			chatgpt_cli_history_set_previous_response_id(resp_id);
			free(resp_id);

			json_object_put(data_json);
		}


		next_event_start = event_end + 2;
		#undef IS_EVENT
	}
	json_tokener_free(tok);

	// update buffer to remove processed events

	if (next_event_start >= callback_data->current_event_buffer + callback_data->current_event_buffer_length) {
		// there's nothing else in the buffer -> free it
		free(callback_data->current_event_buffer);
		callback_data->current_event_buffer = NULL;
		callback_data->current_event_buffer_length = 0;
		return total_chunk_size;
	}

	// unprocessed data remains, move it to the start of the buffer:

	// size_t is unsigned so not safe to do this before the comparison in the proceeding if
	const size_t leftover_length = callback_data->current_event_buffer + callback_data->current_event_buffer_length -
		next_event_start;

	memmove(callback_data->current_event_buffer, next_event_start, leftover_length);
	callback_data->current_event_buffer_length = leftover_length;

	return total_chunk_size;
}


char* openai_stream_response(openai_request* request, openai_delta_callback callback, void* user_data) {
	CURL* curl = curl_easy_init();
	if (!curl) {
		return strdup("Could not initialize CURL");
	}

	curl_easy_setopt(curl, CURLOPT_URL, OPENAI_RESPONSES_API_URL);

	// set CURL request headers
	struct curl_slist* header_list = NULL;
	header_list = curl_slist_append(header_list, "Content-Type: application/json");

	const char* auth_prefix = "Authorization: Bearer ";
	char* auth_header = malloc(1 + strlen(auth_prefix) + strlen(request->api_key));
	strcpy(auth_header, auth_prefix);
	strcat(auth_header, request->api_key);
	header_list = curl_slist_append(header_list, auth_header);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

	// set CURL request content
	json_object* json_request_data = json_object_new_object();

	json_object_object_add(json_request_data, "stream", json_object_new_boolean(true));
	json_object_object_add(json_request_data, "model", json_object_new_string(request->model));
	json_object_object_add(json_request_data, "input", json_object_new_string(request->input));
	if (request->instructions != NULL) {
		json_object_object_add(json_request_data, "instructions", json_object_new_string(request->instructions));
	}

	if (request->previous_response_id != NULL) {
		json_object_object_add(json_request_data, "previous_response_id", json_object_new_string(request->previous_response_id));
	}

	if (request->temperature != OPENAI_REQUEST_TEMPERATURE_NOT_SET) {
		json_object_object_add(json_request_data, "temperature", json_object_new_double(request->temperature));
	}
	if (request->max_tokens != OPENAI_REQUEST_MAX_TOKENS_NOT_SET) {
		json_object_object_add(json_request_data, "max_output_tokens", json_object_new_uint64(request->max_tokens));
	}

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json_request_data));

	curl_callback_stream_callback_data* curl_callback_data = malloc(sizeof(curl_callback_stream_callback_data));
	curl_callback_data->callback = callback;
	curl_callback_data->user_data = user_data;
	curl_callback_data->error = NULL;
	curl_callback_data->current_event_buffer = NULL;
	curl_callback_data->current_event_buffer_length = 0;
	curl_callback_data->request = request;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback_openai_stream_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_callback_data);
	// copy of pointer is passed through, so just pass address in to get actual reference

	const CURLcode curl_response = curl_easy_perform(curl);


	// duplicate to be able to free curl_callback_data
	char* potential_error = NULL;
	if (curl_callback_data->error != NULL) {
		potential_error = strdup(curl_callback_data->error);
	}

	// finalize
	curl_slist_free_all(header_list);
	memset(auth_header, 0, strlen(auth_header)); // let's not let an API key sit in memory
	free(auth_header);
	free(curl_callback_data->error);
	free(curl_callback_data->current_event_buffer);
	free(curl_callback_data);
	json_object_put(json_request_data);

	if (potential_error != NULL) {
		return potential_error;
	}

	if (curl_response != CURLE_OK) {
		return strdup(curl_easy_strerror(curl_response));
	}

	curl_easy_cleanup(curl);

	return NULL;
}
