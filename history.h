//
// Created by mia 25/08/2025.
//

#ifndef HISTORY_H
#define HISTORY_H
#include <stdio.h>

// name of the file which specifies the path of the last history node created
#define CHATGPT_CLI_HISTORY_PREVIOUS_NODE_FILE_NAME ".prev"

// if file doesn't exist after trying with the given, mode defaults to w+
FILE* chatgpt_cli_history_get_previous_response_id_file(const char* mode);

char* chatgpt_cli_history_get_previous_response_id();
void chatgpt_cli_history_set_previous_response_id(const char* id);

#endif //HISTORY_H
