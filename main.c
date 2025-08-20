#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "config.h"
#include "openai-wrapper.h"
#include "curl/curl.h"

#define ENV_API_KEY "CHATGPT_CLI_API_KEY"


static void print_help(const char* command_name) {
	printf("Usage: %s [OPTIONS] PROMPT...\n", command_name);
	printf("\n");
	printf("Required:\n");
	printf("  -k, --key API_KEY          OpenAI API key (overrides %s env variable)\n", ENV_API_KEY);
	printf("  -m, --model MODEL          OpenAI model to use (overrides MODEL config option)\n");
	printf("\n");
	printf("Optional:\n");
	printf("  -i, --instructions TEXT    System instructions for the model (overrides INSTRUCTIONS config option)\n");
	printf("  -r, --raw                  Print raw JSON response instead of parsed text\n");
	printf("  -h, --help                 Show this help message and exit\n");
	printf("\n");
	printf("Environment:\n");
	printf("  %s  API key if not provided with --key\n", ENV_API_KEY);
	printf("\n");

	char* config_path = chatgpt_cli_config_get_path();
	if (config_path) {
		printf("Configuration:\n");
		printf("  %s\n", config_path);
		printf("  Keys in the config file can include 'MODEL' and 'INSTRUCTIONS',\n");
		printf("  which are used if not provided via command-line or environment.\n");
		printf("  Each key is stored as KEY=VALUE on a separate line. Use | to escape newlines.\n");
		printf("\n");
		free(config_path);
	}
}

int main(int argc, char* argv[]) {
	openai_request* request = openai_generate_request_from_options(argc, argv);

	openai_response* response = openai_generate_response(request);

	if (response->error != NULL) {
		printf("Error: %s\n", response->error);
		free(response);
		exit(EXIT_FAILURE);
	}

	char* console_response;

	if (request->raw) {
		console_response = response->raw_response;
	} else {
		console_response = response->output_text;
	}

	printf("%s\n", console_response);

	openai_request_free(request);
	openai_response_free(response);

	return 0;
}


openai_request* openai_generate_request_from_options(int argc, char* argv[]) {
	openai_request* func_request = malloc(sizeof(openai_request));
	func_request->input = NULL;

	if (getenv(ENV_API_KEY)) {
		func_request->api_key = strdup(getenv(ENV_API_KEY));
	} else func_request->api_key = NULL;

	// fine if NULL
	func_request->model = chatgpt_cli_config_read_value("model");
	func_request->instructions = chatgpt_cli_config_read_value("instructions");

	char* prompt = strdup("");

	const struct option long_options[] = {
		{"model", required_argument, 0, 'm'},
		{"key", required_argument, 0, 'k'},
		{"instructions", required_argument, 0, 'i'},
		{"raw", no_argument, 0, 'r'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	int opt; // usually a char, the current option. (with arg optarg)
	while ((opt = getopt_long(argc, argv, "m:k:i:rh", long_options, NULL)) != -1) {
		switch (opt) {
		case 'm':
			func_request->model = strdup(optarg);
			break;
		case 'k':
			func_request->api_key = strdup(optarg);
			break;
		case 'i':
			func_request->instructions = strdup(optarg);
			break;
		case 'r':
			func_request->raw = true;
			break;
		case 'h':
			openai_request_free(func_request);
			print_help("chatgpt_cli");
			exit(EXIT_SUCCESS);
			break;
		}
	}
	if (!func_request->model) {
		char* config_path = chatgpt_cli_config_get_path();
		fprintf(stderr, "Model not provided. Specify with --model or in %s\n", config_path);
		free(config_path);
		free(func_request);
		exit(EXIT_FAILURE);
	}

	if (!func_request->api_key) {
		fprintf(stderr, "OpenAI API key not provided. Specify with %s environment variable or --key\n",
		        ENV_API_KEY);
		free(func_request);
		exit(EXIT_FAILURE);
	}

	// non-option args
	size_t prompt_length = 1; // terminator
	for (int i = optind; i < argc; i++) {
		size_t argLength = strlen(argv[i]);
		prompt_length += argLength + 1; // +1 for space

		char* new_prompt = realloc(prompt, prompt_length * sizeof(char));
		if (new_prompt == NULL) {
			fprintf(stderr, "Memory allocation failed!\n");
			free(prompt);
			exit(EXIT_FAILURE);
		}

		prompt = new_prompt;

		strcat(prompt, argv[i]);

		if (i != argc - 1) {
			strcat(prompt, " "); // alloc space
		}
	}

	if (prompt[0] == '\0') {
		fprintf(stderr, "Prompt not specified. Use --help for usage.\n");
		free(func_request);
		exit(EXIT_FAILURE);
	}

	func_request->input = prompt;

	return func_request;
}
