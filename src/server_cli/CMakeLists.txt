cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME sockspp-server-cli)

project(${PROJECT_NAME})

list(APPEND SOURCES
    src/main.cxx
    src/cli.cxx
)

add_executable(${PROJECT_NAME} ${SOURCES})
