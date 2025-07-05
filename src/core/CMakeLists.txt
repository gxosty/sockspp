cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME sockspp-core)

project(${PROJECT_NAME})

list(APPEND SOURCES
    # sockspp
    src/sockspp/core/s5.cxx
    src/sockspp/core/buffer.cxx
    src/sockspp/core/socket.cxx
    src/sockspp/core/ip_address.cxx

    # dnslib
    src/dnslib/buffer.cpp
    src/dnslib/message.cpp
    src/dnslib/qs.cpp
    src/dnslib/rr.cpp
)

if(WIN32)
    list(APPEND SOURCES
        thirdparty/wepoll/wepoll.c
    )
endif()

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
    list(APPEND LIBRARIES
        ws2_32
    )

    target_include_directories(
        ${PROJECT_NAME} PUBLIC
        thirdparty/wepoll
    )
endif()

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    ${LIBRARIES}
)
