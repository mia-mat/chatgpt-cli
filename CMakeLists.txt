cmake_minimum_required(VERSION 3.15)

project(chatgpt_cli C)



set(CMAKE_C_STANDARD 17)

find_package(CURL REQUIRED)
find_library(JSONC_LIB json-c REQUIRED)

add_executable(chatgpt_cli main.c
        main.h
        openai-wrapper.c
        openai-wrapper.h
        config.c
        config.h
        version.h
        history.c
        history.h)
target_link_libraries(chatgpt_cli PRIVATE
        CURL::libcurl
        ${JSONC_LIB})
