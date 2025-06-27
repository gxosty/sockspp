cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME sockspp-server)

project(${PROJECT_NAME})

list(APPEND SOURCES
    src/sockspp/server/server.cxx
    src/sockspp/server/client_socket.cxx
    src/sockspp/server/remote_socket.cxx
    src/sockspp/server/udp_socket.cxx
    src/sockspp/server/session.cxx
)

if(SOCKSPP_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCES})
endif()

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    src
    $<TARGET_PROPERTY:sockspp-core,INCLUDE_DIRECTORIES>
)

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    sockspp-core
)
