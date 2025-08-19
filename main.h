//
// Created by mia on 8/15/25.
//

#ifndef CHATGPT_CLI_MAIN_H
#define CHATGPT_CLI_MAIN_H
#include "openai-wrapper.h"

openai_request *generateRequestFromOptions(int argc, char *argv[]);
#endif //CHATGPT_CLI_MAIN_H