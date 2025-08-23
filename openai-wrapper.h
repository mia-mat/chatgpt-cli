//
// Created by mia on 8/15/25.
//

#ifndef CHATGPT_CLI_OPENAI_WRAPPER_H
#define CHATGPT_CLI_OPENAI_WRAPPER_H
#include <stdbool.h>
#include <stddef.h>

typedef struct {
	char* instructions;
	char* input;
	char* model;
	char* api_key;
	bool raw;
} openai_request;

void openai_request_free(openai_request* request);

typedef void (*openai_delta_callback)(const char* delta, size_t length, void* user_data);

// stream response deltas into a callback, returns NULL if successful, or an error if one occurred (caller frees).
char* openai_stream_response(openai_request* request, openai_delta_callback callback, void* user_data);

#endif //CHATGPT_CLI_OPENAI_WRAPPER_H
