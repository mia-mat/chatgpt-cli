//
// Created by mia on 19/08/2025.
//

#ifndef CONFIG_H
#define CONFIG_H

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

char* chatgpt_cli_config_get_app_folder();

char* chatgpt_cli_config_get_config_path();

// returns the value or NULL if not found or config file doesn't exist
char* chatgpt_cli_config_read_value(const char* key);

#endif //CONFIG_H
