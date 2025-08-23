cmake_minimum_required(VERSION 3.15)

project(chatgpt_cli
        VERSION 1.2.2
        LANGUAGES C)

configure_file(
        "${PROJECT_SOURCE_DIR}/version.h.in"
        "${PROJECT_BINARY_DIR}/version.h"
        @ONLY
)


set(CMAKE_C_STANDARD 17)

find_package(CURL REQUIRED)
find_library(JSONC_LIB json-c REQUIRED)

add_executable(chatgpt_cli main.c
        main.h
        openai-wrapper.c
        openai-wrapper.h
        config.c
        config.h)
target_link_libraries(chatgpt_cli PRIVATE
        CURL::libcurl
        ${JSONC_LIB})

target_include_directories(chatgpt_cli PRIVATE ${CMAKE_BINARY_DIR}) # so version.h is found
