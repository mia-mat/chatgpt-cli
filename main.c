#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "config.h"
#include "openai-wrapper.h"
#include "curl/curl.h"

#define ENV_API_KEY "CHATGPT_CLI_API_KEY"

int main(int argc, char* argv[]) {


	openai_request* request = openai_generate_request_from_options(argc, argv);

	openai_response* response = openai_generate_response(request);

	if (response->error != NULL) {
		printf("Error: %s\n", response->error);
		free(response);
		return 1;
	}

	char* consoleResponse;

	if (request->raw) {
		consoleResponse = response->raw_response;
	} else {
		consoleResponse = response->output_text;
	}

	printf("%s\n", consoleResponse);

	openai_request_free(request);
	openai_response_free(response);

	return 0;
}


openai_request* openai_generate_request_from_options(int argc, char* argv[]) {
	openai_request* retRequest = malloc(sizeof(openai_request));

	if (getenv(ENV_API_KEY)) {
		retRequest->api_key = strdup(getenv(ENV_API_KEY));
	} else retRequest->api_key = NULL;

	// fine if NULL
	retRequest->model = chatgpt_cli_config_read_value("model");
	retRequest->instructions = chatgpt_cli_config_read_value("instructions");

	char* prompt = strdup("");

	const struct option long_options[] = {
		{"model", required_argument, 0, 'm'},
		{"key", required_argument, 0, 'k'},
		{"instructions", required_argument, 0, 'i'},
		{"raw", no_argument, 0, 'r'},
		{0, 0, 0, 0}
	};

	int opt; // usually a char, the current option. (with arg optarg)
	while ((opt = getopt_long(argc, argv, "m:k:i:r", long_options, NULL)) != -1) {
		switch (opt) {
		case 'm':
			retRequest->model = strdup(optarg);
			break;
		case 'k':
			retRequest->api_key = strdup(optarg);
			break;
		case 'i':
			retRequest->instructions = strdup(optarg);
			break;
		case 'r':
			retRequest->raw = true;
			break;
		}
	}
	if (!retRequest->model) {
		char* config_path = chatgpt_cli_config_get_path();
		fprintf(stderr, "Model not provided. Specify with --model or in %s\n", config_path);
		free(config_path);
		exit(EXIT_FAILURE);
	}

	if (!retRequest->api_key) {
		fprintf(stderr, "OpenAI API key not provided. Specify with %s environment variable or --key\n",
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
