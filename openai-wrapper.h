//
// Created by mia on 8/15/25.
//

#ifndef CHATGPT_CLI_OPENAI_WRAPPER_H
#define CHATGPT_CLI_OPENAI_WRAPPER_H
#include <stdbool.h>

typedef struct {
	char* instructions;
	char* input;
	char* model;
	char* api_key;
	bool raw;
} openai_request;

typedef struct {
	char* raw_response;
	char* output_text;
	char* error;
} openai_response;

void openai_request_free(openai_request* request);
void openai_response_free(openai_response* response);

openai_response* openai_create_response_object(const char* curl_response);

openai_response* openai_generate_response(openai_request* request);
#endif //CHATGPT_CLI_OPENAI_WRAPPER_H
