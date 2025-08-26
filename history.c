//
// Created by mia on 25/08/2025.
//

#include "history.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"


#ifdef _WIN32
#include <direct.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif

// recursive mkdir
static void r_mkdir(char* path) {
	// mkdir simply fails if directory exists, so go through all subdirs.

	const char* current_ptr = path;
	const size_t path_length = strlen(path);
	while (true) {
		const char* delimiter_ptr = strchr(current_ptr, PATH_SEPARATOR);
		if (delimiter_ptr == NULL) break;
		current_ptr = delimiter_ptr + 1;

		// get char* from path->delimiter_ptr
		const size_t len = delimiter_ptr - path;

		char* path_to_mkdir = malloc(len + 1);
		memcpy(path_to_mkdir, path, len);
		path_to_mkdir[len] = '\0';

		#ifdef _WIN32
		_mkdir(path_to_mkdir);
		#else
		mkdir(path_to_mkdir, 0777);
		#endif

		free(path_to_mkdir);

		// check if current is after the path, this isn't our memory, let's not.
		if (current_ptr - path_length > path) break;
	}

	// in case the path doesn't end with a delimiter
	#ifdef _WIN32
		_mkdir(path);
	#else
		mkdir(path, 0777);
	#endif
}

FILE* chatgpt_cli_history_get_previous_response_id_file(const char* mode) {
	char* app_folder = chatgpt_cli_config_get_app_folder();
	const size_t len = strlen(CHATGPT_CLI_HISTORY_PREVIOUS_NODE_FILE_NAME) + strlen(app_folder) + 2;
	// 2 for path separator and \0

	char* path = malloc(len);
	snprintf(path, len, "%s%c%s", app_folder, PATH_SEPARATOR, CHATGPT_CLI_HISTORY_PREVIOUS_NODE_FILE_NAME);

	FILE* ret_file = fopen(path, mode); // read/write

	if (ret_file == NULL) {
		// create subdirs and file
		r_mkdir(app_folder);

		ret_file = fopen(path, "w+"); // read/write, clears/creates
		if (ret_file == NULL) {
			fprintf(stderr, "\nUnable to open history file!\n");
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
