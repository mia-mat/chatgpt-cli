#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "openai-wrapper.h"
#include "curl/curl.h"

#define ENV_API_KEY "CHATGPT_CLI_API_KEY"
#define ENV_SYSTEM_PROMPT "CHATGPT_CLI_SYSTEM_PROMPT"

int main(int argc, char *argv[]) {
    free(0);
    // options for
    // --model
    // --key: api key
    // eventually -s for search
    // --raw for raw response (full json)
    openai_request *request = generateRequestFromOptions(argc, argv);

    openai_response *response = generateResponse(request);

    printf("%s\n", response->output);

    // printf("%s, %s, %s", request->apiKey, request->model, request->prompt);

    free(request->apiKey);
    free(request->input);
    free(request->model);
    free(request->systemPrompt);
    free(request);

    free(response);

    return 0;
}


openai_request *generateRequestFromOptions(int argc, char *argv[]) {
    openai_request *retRequest = malloc(sizeof(openai_request));

    retRequest->model = NULL;

    if (getenv(ENV_API_KEY)) {
        retRequest->apiKey = strdup(getenv(ENV_API_KEY));
    } else retRequest->apiKey = NULL;

    if (getenv(ENV_SYSTEM_PROMPT)) {
        retRequest->systemPrompt = strdup(getenv(ENV_SYSTEM_PROMPT));
    } else {
        retRequest->systemPrompt = NULL;
    }

    char *prompt = strdup("\0");

    const struct option long_options[] = {
        {"model", required_argument, 0, 'm'},
        {"apiKey", required_argument, 0, 'k'},
        {"system", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int opt; // usually a char, the current option. (with arg optarg)
    while ((opt = getopt_long(argc, argv, "m:k:s:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                retRequest->model = strdup(optarg);
                break;
            case 'k':
                retRequest->apiKey = strdup(optarg);
                break;
            case 's':
                retRequest->systemPrompt = strdup(optarg);
                break;
        }
    }
    if (!retRequest->model) {
        fprintf(stderr, "Model not provided. Specify with --model\n");
        exit(EXIT_FAILURE);
    }

    if (!retRequest->apiKey) {
        fprintf(stderr, "OpenAI API key not provided. Specify with %s environment variable or --apiKey\n",
                ENV_API_KEY);
        exit(EXIT_FAILURE);
    }

    // non-option args
    size_t promptLength = 1; // terminator
    for (int i = optind; i < argc; i++) {
        size_t argLength = strlen(argv[i]);
        promptLength += argLength + 1; // +1 for space

        prompt = realloc(prompt, promptLength * sizeof(char));

        strcat(prompt, argv[i]);


        if (i != argc - 1) {
            strcat(prompt, " "); // alloc space
        }
    }

    retRequest->input = prompt;

    return retRequest;
}