#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <json-c/json_tokener.h>

#include "config.h"
#include "openai-wrapper.h"
#include "curl/curl.h"
#include "version.h"

#define ENV_API_KEY "CHATGPT_CLI_API_KEY"
#define CHATGPT_CLI_PROGRAM_NAME "chatgpt-cli"

static void print_help() {
	printf("Usage: %s [OPTIONS] PROMPT...\n", CHATGPT_CLI_PROGRAM_NAME);
	printf("\n");
	printf("Required:\n");
	printf("  -k, --key API_KEY          OpenAI API key (overrides %s env variable)\n", ENV_API_KEY);
	printf("  -m, --model MODEL          OpenAI model to use (overrides 'model' config option)\n");
	printf("\n");
	printf("Optional:\n");
	printf("  -i, --instructions TEXT    System instructions for the model (overrides 'instructions' config option)\n");
	printf("  -t, --temperature DOUBLE   Sampling temperature for the model, must be in [0,2] (overrides 'temperature' config option)\n");
	printf("  -T, --max-tokens UINT64    Upper bound for output tokens in the response (overrides 'max-tokens' config option)\n");
	printf("  -r, --raw                  Print raw JSON response instead of parsed text\n");
	printf("  -h, --help                 Show this help message and exit\n");
	printf("  -v, --version              Show program version\n");
	printf("\n");
	printf("Environment:\n");
	printf("  %s  API key if not provided with --key\n", ENV_API_KEY);
	printf("\n");

	char* config_path = chatgpt_cli_config_get_path();
	if (config_path) {
		printf("Configuration:\n");
		printf("  %s\n", config_path);
		printf("  Keys in the config file can include 'model' and 'instructions',\n");
		printf("  which are used if not provided via command-line or environment.\n");
		printf("  Each key is stored as KEY=VALUE on a separate line. Use | to escape newlines.\n");
		printf("\n");
		free(config_path);
	}
}

static void openai_stream_callback_print(const char* delta, const size_t length, void* user_data) {
	fprintf(stdout, "%.*s", (int)length, delta);
	fflush(stdout);
}

int main(int argc, char* argv[]) {
	openai_request* request = openai_generate_request_from_options(argc, argv);

	char* error = openai_stream_response(request, openai_stream_callback_print, 0);
	if (error != NULL) {
		printf("Error: %s", error);
		exit(EXIT_FAILURE);
	}
	printf("\n");

	openai_request_free(request);

	return 0;
}


openai_request* openai_generate_request_from_options(int argc, char* argv[]) {
	openai_request* func_request = malloc(sizeof(openai_request));
	func_request->input = NULL;

	if (getenv(ENV_API_KEY)) {
		func_request->api_key = strdup(getenv(ENV_API_KEY));
	} else func_request->api_key = NULL;

	// fine if NULL/0
	func_request->model = chatgpt_cli_config_read_value("model");
	func_request->instructions = chatgpt_cli_config_read_value("instructions");
	func_request->max_tokens = strtoul(chatgpt_cli_config_read_value("max_tokens"), NULL, 10);
	func_request->raw = false;

	// 0 is actually a valid input for temperature, and we're not setting a pointer, so check first,
	// and just put the value outside the normal range if it doesn't exist
	const char* config_temperature = chatgpt_cli_config_read_value("temperature");
	if (config_temperature != NULL) {
		func_request->temperature = strtod(config_temperature, NULL);
	} else func_request->temperature = OPENAI_REQUEST_TEMPERATURE_NOT_SET;

	char* prompt = strdup("");

	const struct option long_options[] = {
		{"model", required_argument, 0, 'm'},
		{"key", required_argument, 0, 'k'},
		{"instructions", required_argument, 0, 'i'},
		{"temperature", required_argument, 0, 't'},
		{"max-tokens", required_argument, 0, 'T'},
		{"raw", no_argument, 0, 'r'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int opt; // usually a char, the current option. (with arg optarg)
	while ((opt = getopt_long(argc, argv, "m:k:i:t:T:rhv", long_options, NULL)) != -1) {
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
		case 't':
			// If user doesn't provide a valid double, temperature will be set to 0 without warning.
			// Consider this user error.
			func_request->temperature = strtod(optarg, NULL);
			break;
		case 'T':
			// ignored if set to 0
			func_request->max_tokens = strtoul(optarg, NULL, 10);
			break;
		case 'r':
			func_request->raw = true;
			break;
		case 'h':
			openai_request_free(func_request);
			print_help("chatgpt_cli");
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			openai_request_free(func_request);
			printf("%s %s\ncreated by mia <3\n", CHATGPT_CLI_PROGRAM_NAME, CHATGPT_CLI_VERSION);
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

	// 0-2 is temperature interval, as per OpenAI docs (https://platform.openai.com/docs/api-reference/responses/create)
	if (func_request->temperature != OPENAI_REQUEST_TEMPERATURE_NOT_SET &&
		(func_request->temperature < 0 || func_request->temperature > 2)) {
		fprintf(stderr, "Invalid Temperature %f is not in required interval [0,2]", func_request->temperature);
		openai_request_free(func_request);
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
