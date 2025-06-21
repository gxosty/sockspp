#include "sockspp/core/exceptions.hpp"
#ifdef _WIN32
    #include <winsock2.h>
#endif // _WIN32

#include <sockspp/server/server_params.hpp>
#include <sockspp/server/server.hpp>
#include <sockspp/core/log.hpp>

#include <csignal>

namespace
{

static sockspp::Server* g_server = nullptr;

}

static inline void sockspp_print_greeting()
{
    LOGI("%s v%s", SOCKSPP_NAME, SOCKSPP_VERSION);
}

static inline void initialize()
{
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

static void sigint_handler(int sig)
{
    if (g_server)
    {
        LOGI("Interrupt");
        g_server->stop();
    }
}

static inline void prepare_sigint()
{
    signal(SIGINT, sigint_handler);
}

#ifdef __cplusplus
extern "C" {
#endif

int sockspp_main(int argc, char* argv[])
{
    sockspp_print_greeting();
    initialize();

    sockspp::ServerParams params;

    params.listen_ip = "192.168.1.104";
    params.listen_port = 2080;

    g_server = new sockspp::Server(std::move(params));
    prepare_sigint();
    g_server->serve();
    LOGI("Server stopped");

    return 0;
}

#ifdef __cplusplus
}
#endif
