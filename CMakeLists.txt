cmake_minimum_required(VERSION 3.15)

# Project variables
set(PROJECT_NAME sockspp)
set(SOCKSPP_VERSION_MAJOR 1)
set(SOCKSPP_VERSION_MINOR 0)
set(SOCKSPP_VERSION_PATCH 0)
set(SOCKSPP_VERSION ${SOCKSPP_VERSION_MAJOR}.${SOCKSPP_VERSION_MINOR}.${SOCKSPP_VERSION_PATCH})
set(CMAKE_CXX_STANDARD 20)

project(
    ${PROJECT_NAME}
    VERSION ${SOCKSPP_VERSION}
)

# Options
option(SOCKSPP_CLIENT "Build socks5 client module" OFF)
option(SOCKSPP_SERVER "Build socks5 server module" ON)
option(SOCKSPP_SERVER_CLI "Build socks5 server CLI (socks5 server module is required)" ON)
option(SOCKSPP_BUILD_SHARED "Build shared lib, otherwise static" OFF)
option(SOCKSPP_ENABLE_LOCATION_LOGS "Enable filename and log location in logs" OFF)
option(SOCKSPP_DISABLE_LOGS "Disable logs" OFF)

if(NOT SOCKSPP_CLIENT AND NOT SOCKSPP_SERVER)
    message(SEND_ERROR "At least one module has to be enabled (client or server)")
endif()

if(NOT SOCKSPP_SERVER AND SOCKSPP_SERVER_CLI)
    message(WARNING "Server CLI is enabled but server module is disabled, ignoring cli build...")
    set(SOCKSPP_SERVER_CLI OFF)
endif()

# Global Definitions
add_compile_definitions(
    -DSOCKSPP_NAME="${PROJECT_NAME}"
    -DSOCKSPP_VERSION_MAJOR=${SOCKSPP_VERSION_MAJOR}
    -DSOCKSPP_VERSION_MINOR=${SOCKSPP_VERSION_MINOR}
    -DSOCKSPP_VERSION_PATCH=${SOCKSPP_VERSION_PATCH}
    -DSOCKSPP_VERSION="${SOCKSPP_VERSION}"
    -DSOCKSPP_ENABLE_LOCATION_LOGS=$<BOOL:${SOCKSPP_ENABLE_LOCATION_LOGS}>
    -DSOCKSPP_DISABLE_LOGS=$<BOOL:${SOCKSPP_DISABLE_LOGS}>
)

add_subdirectory(src/core)

if(SOCKSPP_CLIENT)
    add_subdirectory(src/client)
endif()

if(SOCKSPP_SERVER)
    add_subdirectory(src/server)

    if(SOCKSPP_SERVER_CLI)
        add_subdirectory(src/server_cli)
    endif()
endif()
