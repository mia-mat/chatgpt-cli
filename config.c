//
// Created by mia on 19/08/2025.
//

#include "config.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* chatgpt_cli_config_get_app_folder() {
	char* path = NULL;

#ifdef _WIN32
	char* user = getenv("USERPROFILE");
	if (!user) {
		fprintf(stderr, "Could not find save location");
		exit(1);
	}

	const char* subdir = "\\AppData\\Local\\chatgpt-cli";
	size_t len = strlen(user) + strlen(subdir) + 1;
	path = malloc(len);
	snprintf(path, len, "%s%s", user, subdir);

#else
	char* home = getenv("HOME");
	if (!home) {
		fprintf(stderr, "Could not find save location");
		exit(1);
	}

	const char* subdir = "/.chatgpt-cli";
	size_t len = strlen(home) + strlen(subdir) + 1;
	path = malloc(len);
	snprintf(path, len, "%s%s", home, subdir);

#endif

	return path;
}

char* chatgpt_cli_config_get_config_path() {
	char* app_path = chatgpt_cli_config_get_app_folder();
	const char* config_file_name = ".config";
	const size_t len = strlen(app_path) + strlen(config_file_name) + 2; // 2 for path separator and \0

	char* path = malloc(len);
	snprintf(path, len, "%s%c%s", app_path, PATH_SEPARATOR, config_file_name);
	free(app_path);
	return path;
}

char* chatgpt_cli_config_read_value(const char* key) {
	char* config_path = chatgpt_cli_config_get_config_path();

	FILE* config_file = fopen(config_path, "rb");
	free(config_path);

	if (!config_file) {
		return NULL;
	}

	// formatted as KEY=VALUE pairs on lines, with | to go onto next line
	// if a line starts with '#' and isn't inside a value, consider it as a comment.

	fseek(config_file, 0, SEEK_END);
	size_t file_length = ftell(config_file);
	fseek(config_file, 0, SEEK_SET); // back to start

	char* config_content = malloc(file_length + 1);
	fread(config_content, sizeof(char), file_length, config_file);

	fclose(config_file);

	if (!config_content) {
		return NULL;
	}

	config_content[file_length] = '\0'; // fread doesn't automatically null-terminate

	// reset per-line and depending on if we're reading the key or value.
	char* current_part = malloc(1);
	current_part[0] = '\0';
	size_t current_part_length = 0;

	bool equals_seen = false; // has an = been seen on the current line (are we writing to key or value)
	bool key_found = false; // if the current value is being assigned to the key which we want to read from
	bool skip_line = false; // if the line is a comment line

	for (int i = 0; i < file_length; i++) {
		const char current_char = config_content[i];
		if (current_char == '\r') continue;

		if (current_char == '\n') {
			if (key_found) break;

			// reset current part
			free(current_part);
			current_part = strdup("");
			current_part_length = 0;

			equals_seen = false;
			skip_line = false;

			if (file_length > i+1 && config_content[i+1] == '#') {
				skip_line = true;
			}

			continue;
		}

		if (skip_line) {
			continue;
		}

		if (current_char == '=' && !equals_seen) {
			equals_seen = true;

			// check for match
			if(strcmp(current_part, key) == 0) {
				key_found = true;
			}

			// reset current part
			free(current_part);
			current_part = strdup("");
			current_part_length = 0;

			continue;
		}

		// input is going onto a new line, continue
		if (current_char == '|' && equals_seen &&
			((file_length > i+1 && config_content[i+1] == '\n')
			|| (file_length > i+2 && config_content[i+1] == '\r' && config_content[i+2] == '\n'))) {
			i++; // skip newline character
			if (config_content[i] == '\r') i++; // extra skip for windows because \r\n
			continue;
		}


		if (!key_found && equals_seen) {
			// writing to this value would be pointless
			continue;
		}

		char* new_current_part = realloc(current_part, current_part_length+2);
		if (new_current_part == NULL) {
			fprintf(stderr, "Memory allocation failed!\n");
			free(current_part);
			exit(EXIT_FAILURE);
		}
		current_part = new_current_part;
		current_part[current_part_length] = config_content[i];
		current_part[current_part_length+1] = '\0';
		current_part_length++;

	}

	free(config_content);

	if (!key_found) {
		free(current_part); // contains some un-needed key or value
		return NULL;
	}

	return current_part;
}