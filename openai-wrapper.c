//
// Created by mia on 8/15/25.
//

#include "openai-wrapper.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>

// returns num. bytes handled
size_t write_callback(const char *contentPtr, size_t size_atomic, size_t nElements, char **data) {
    size_t chunkSize = size_atomic*nElements;

    // data holds the address of our char* data
    // so dereference for char*
    *data = realloc(*data, strlen(*data) + chunkSize + 1);  // +1 for null-terminator
    if (*data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 0;
    }

    strncat(*data, contentPtr, chunkSize);

    return chunkSize;
}


openai_response* generateResponse(openai_request* request) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");

    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "Content-Type: application/json");

    char* authPrefix = "Authorization: Bearer ";
    char* authHeader = malloc(1 + strlen(authPrefix) + strlen(request->apiKey));
    strcpy(authHeader, authPrefix);
    strcat(authHeader, request->apiKey);
    list = curl_slist_append(list, authHeader);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    json_object *jsonRequestData = json_object_new_object();

    json_object_object_add(jsonRequestData, "model", json_object_new_string(request->model));
    json_object_object_add(jsonRequestData, "input", json_object_new_string(request->input));

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jsonRequestData));

    char* responseString = malloc(1);
    responseString[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString); // copy of pointer is given, so just pass address in to get actual reference

    CURLcode curlResponse = curl_easy_perform(curl);

    curl_slist_free_all(list);
    free(authHeader);
    json_object_put(jsonRequestData);

    if (curlResponse != CURLE_OK) {
        fprintf(stderr, "oops: %s", curl_easy_strerror(curlResponse));
        return NULL;
    }

    curl_easy_cleanup(curl);

    // printf("%s\n", responseString);
    json_object* jsonResponseData = json_tokener_parse(responseString);

    openai_response* funcResponse = malloc(sizeof(openai_response));
    funcResponse->output = getResponseOutputText(jsonResponseData);

    return funcResponse;
}

const char* getResponseOutputText(json_object* responseJson) {
    // responseJson structured as
    // object -> "output" array [i]-> "content" array [0]-> "text" string
    // where i has type "message"

    json_object* outputArray = NULL;
    if (!json_object_object_get_ex(responseJson, "output", &outputArray)) {
        return NULL;
    }

    json_object* contentArray = NULL;
    if (!json_object_object_get_ex(json_object_array_get_idx(outputArray, 0), "content", &contentArray)) {
        return NULL;
    } // todo fix

    json_object* jsonTextString = NULL;
    if (!json_object_object_get_ex(json_object_array_get_idx(contentArray, 0), "text", &jsonTextString)) {
        return NULL;
    }
    return json_object_get_string(jsonTextString);
}