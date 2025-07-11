#pragma once

// number of incoming tcp connections in the accept queue
#define SOCKSPP_SERVER_LISTEN 128

// initial available space for server poll result
#define SOCKSPP_SERVER_INITIAL_POLL_RESULT_SIZE 128

// buffer size on stack for each session (doubles for udp)
#define SOCKSPP_SESSION_SOCKET_BUFFER_SIZE 16384

// max number of dns queries allowed at the same time per session
#define SOCKSPP_SESSION_MAX_DNS_SOCKETS 5
