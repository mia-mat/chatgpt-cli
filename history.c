//
// Created by mia on 25/08/2025.
//

#include "history.h"

#include <stdlib.h>
#include <string.h>

#include "config.h"

FILE* chatgpt_cli_history_get_previous_response_id_file(const char* mode) {
	char* app_folder = chatgpt_cli_config_get_app_folder();
	const size_t len = strlen(CHATGPT_CLI_HISTORY_PREVIOUS_NODE_FILE_NAME) + strlen(app_folder) + 2; // 2 for path separator and \0

	char* path = malloc(len);
	snprintf(path, len, "%s%c%s", app_folder, PATH_SEPARATOR, CHATGPT_CLI_HISTORY_PREVIOUS_NODE_FILE_NAME);

	FILE* ret_file = fopen(path, mode); // read/write

	if (ret_file == NULL) {
		// create file
		ret_file = fopen(path, "w+"); // read/write, clears/creates
		if (ret_file == NULL) {
			fprintf(stderr, "Unable to open history file!");
			exit(1);
		}
	}
	free(path);

	return ret_file;

}

char* chatgpt_cli_history_get_previous_response_id() {
	FILE* file = chatgpt_cli_history_get_previous_response_id_file("r+"); // read/write

	fseek(file, 0, SEEK_END);
	const size_t file_length = ftell(file);
	fseek(file, 0, SEEK_SET); // back to start

	if (file_length == 0) return NULL;

	char* content = malloc(file_length + 1);
	fread(content, sizeof(char), file_length, file);
	content[file_length] = '\0'; // fread doesn't automatically null-terminate

	fclose(file);
	return content;
}

void chatgpt_cli_history_set_previous_response_id(const char* id) {
	// "w+" acts like read/write but automatically empties the file
	FILE* file = chatgpt_cli_history_get_previous_response_id_file("w+");
	fwrite(id, sizeof(char), strlen(id), file);
	fclose(file);
}
