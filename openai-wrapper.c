//
// Created by mia on 8/15/25.
//

#include "openai-wrapper.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>

void openai_request_free(openai_request* request) {
	free(request->api_key);
	free(request->input);
	free(request->model);
	free(request->instructions);
	free(request);
}

void openai_response_free(openai_response* response) {
	free(response->error);
	free(response->output_text);
	free(response);
}

// returns num. bytes handled
size_t write_callback(const char* content_ptr, size_t size_atomic, size_t n_elements, char** data) {
	size_t chunkSize = size_atomic * n_elements;

	// data holds the address of our char* data
	// so dereference for char*
	*data = realloc(*data, strlen(*data) + chunkSize + 1); // +1 for null-terminator
	if (*data == NULL) {
		fprintf(stderr, "Memory allocation failed!\n");
		return 0;
	}

	strncat(*data, content_ptr, chunkSize);

	return chunkSize;
}

openai_response* generate_response(openai_request* request) {
	// only used if CURL errors before we properly generate a response
	openai_response* fallback_response = malloc(sizeof(openai_response));
	fallback_response->output_text = NULL;
	fallback_response->raw_response = NULL;

	CURL* curl = curl_easy_init();
	if (!curl) {
		fallback_response->error = strdup("Could not initialize CURL");
		return fallback_response;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");

	struct curl_slist* list = NULL;
	list = curl_slist_append(list, "Content-Type: application/json");

	char* auth_prefix = "Authorization: Bearer ";
	char* auth_header = malloc(1 + strlen(auth_prefix) + strlen(request->api_key));
	strcpy(auth_header, auth_prefix);
	strcat(auth_header, request->api_key);
	list = curl_slist_append(list, auth_header);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

	json_object* json_request_data = json_object_new_object();

	json_object_object_add(json_request_data, "model", json_object_new_string(request->model));
	json_object_object_add(json_request_data, "input", json_object_new_string(request->input));
	if (request->instructions != NULL) {
		json_object_object_add(json_request_data, "instructions", json_object_new_string(request->instructions));
	}

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(json_request_data));

	char* response_string = malloc(1);
	response_string[0] = '\0';
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
	// copy of pointer is given, so just pass address in to get actual reference

	CURLcode curl_response = curl_easy_perform(curl);

	curl_slist_free_all(list);
	free(auth_header);
	json_object_put(json_request_data);

	if (curl_response != CURLE_OK) {
		fallback_response->error = strdup(curl_easy_strerror(curl_response));
		return fallback_response;
	}

	curl_easy_cleanup(curl);

	return create_response_object(response_string);
}

openai_response* create_response_object(const char* curl_response) {
	openai_response* func_response = malloc(sizeof(openai_response));
	func_response->raw_response = strdup(curl_response);
	func_response->error = NULL;
	func_response->output_text = NULL;

	json_object* response_json = json_tokener_parse(curl_response);
	// response_json *should be* structured as
	// master object -> "output" array [i]-> "content" array [j]-> "text" string
	// where i has type "message"
	// and j has type "output_text"

	// check for any errors first
	json_object* error = json_object_object_get(response_json, "error");
	if (error != NULL) {
		func_response->error = strdup(json_object_get_string(json_object_object_get(error, "message")));
		json_object_put(response_json);
		return func_response;
	}

	json_object* output_array = json_object_object_get(response_json, "output");

	if (output_array == NULL) { // ?
		func_response->error = strdup("Malformed JSON response");
		json_object_put(response_json);
		return func_response;
	}

	json_object* content_array = NULL;

	// loop through output array to find content which is of type 'message'
	// (i.e. has the actual content we're looking for)
	for (int i = 0; i < json_object_array_length(output_array); i++) {
		json_object* output_array_element = json_object_array_get_idx(output_array, i);
		json_object* type = json_object_object_get(output_array_element, "type");


		if (type == NULL || strcmp(json_object_get_string(type), "message") != 0) {
			continue;
		}

		content_array = json_object_object_get(output_array_element, "content");
	}

	if (content_array == NULL) {
		func_response->error = strdup("Malformed JSON response");
		json_object_put(response_json);
		return func_response;
	}

	json_object* json_output_text = NULL;

	// loop through content array to find the object with type 'output_text', and hence the output text
	for (int i = 0; i < json_object_array_length(content_array); i++) {
		json_object* content_array_element = json_object_array_get_idx(content_array, i);
		json_object* type = json_object_object_get(content_array_element, "type");

		if (type == NULL || strcmp(json_object_get_string(type), "output_text") != 0) {
			continue;
		}

		json_output_text = json_object_object_get(content_array_element, "text");
	}

	if (json_output_text == NULL) {
		func_response->error = strdup("Malformed JSON response");
		json_object_put(response_json);
		return func_response;
	}

	func_response->output_text = strdup(json_object_get_string(json_output_text));

	json_object_put(response_json);
	return func_response;
}
