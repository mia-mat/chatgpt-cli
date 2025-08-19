//
// Created by mia on 19/08/2025.
//

#ifndef CONFIG_H
#define CONFIG_H

char* chatgpt_cli_config_get_path();

// returns the value or NULL if not found or config file doesn't exist
char* chatgpt_cli_config_read_value(const char* key);

#endif //CONFIG_H
