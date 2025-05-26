cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME sockspp-core)

project(${PROJECT_NAME})

list(APPEND SOURCES
    src/sockspp/core/buffer.cxx
    src/sockspp/core/socket.cxx
)

if(SOCKSPP_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCES})
endif()

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    src
)

if(WIN32)
    target_link_libraries(
        ${PROJECT_NAME} PUBLIC
        ws2_32
    )
endif()
