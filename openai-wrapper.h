//
// Created by mia on 8/15/25.
//

#ifndef CHATGPT_CLI_OPENAI_WRAPPER_H
#define CHATGPT_CLI_OPENAI_WRAPPER_H
#include <json-c/json_object.h>


typedef struct {
    char* systemPrompt;
    char* input;
    char* model;
    char* apiKey;
} openai_request;

typedef struct {
    char* raw_response;
    char* output;
    char* error;
} openai_response;

void openai_request_free(openai_request *request);
void openai_response_free(openai_response *response);

const openai_response* create_response_object(const char* curl_response);

const openai_response* generate_response(openai_request* request);
#endif //CHATGPT_CLI_OPENAI_WRAPPER_H