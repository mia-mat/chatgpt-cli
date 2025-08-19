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
    const char* output;
    const char* error;
} openai_response;


const char* getResponseOutputText(json_object* responseJson);
openai_response* generateResponse(openai_request* request);
#endif //CHATGPT_CLI_OPENAI_WRAPPER_H