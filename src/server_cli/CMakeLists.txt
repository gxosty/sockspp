cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME sockspp-server-cli)

project(${PROJECT_NAME})

list(APPEND SOURCES
    src/main.cxx
    src/cli.cxx
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:sockspp-server,INCLUDE_DIRECTORIES>
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    sockspp-server
)
