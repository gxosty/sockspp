#ifdef _WIN32
    #include <winsock2.h>
#endif // _WIN32

#include <argparse/argparse.hpp>

#include <sockspp/server/server_params.hpp>
#include <sockspp/server/server.hpp>
#include <sockspp/core/log.hpp>

#include <csignal>
#include <cstdint>
#include <iostream>
#include <cstdlib>

namespace
{

static sockspp::server::Server* g_server = nullptr;

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

    // Enable vt sequence processing in Windows
    HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (out_handle != INVALID_HANDLE_VALUE)
    {
        DWORD mode = 0;

        if (!GetConsoleMode(out_handle, &mode))
        {
            return;
        }

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(out_handle, mode))
        {
            return;
        }
    }
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

static inline sockspp::server::ServerParams parse_params(int argc, char* argv[])
{
    argparse::ArgumentParser parser(SOCKSPP_NAME, SOCKSPP_VERSION);    
    
    parser.add_argument("--listen-ip")
        .help("listen IP address")
        .default_value("0.0.0.0")
        .nargs(1);

    parser.add_argument("--listen-port")
        .help("listen port")
        .default_value((uint16_t)1080)
        .scan<'d', uint16_t>()
        .nargs(1);

    parser.add_argument("--username")
        .help("authentication username")
        .default_value("")
        .nargs(1);

    parser.add_argument("--password")
        .help("authentication password")
        .default_value("")
        .nargs(1);

    parser.add_argument("--dns-ip")
        .help(
            "dns server ip\n"
            "\"x.x.x.x\" = custom dns server\n"
            "\"none\" = disable dns resolution\n"
            "\"auto\" = default OS dns server address")
        .default_value("auto")
        .implicit_value("none")
        .nargs(1);

    parser.add_argument("--dns-port")
        .help("dns server port")
        .default_value((uint16_t)53)
        .scan<'d', uint16_t>()
        .nargs(1);

    parser.add_argument("--client-tcp-nodelay")
        .help("enable tcp nodelay for client socket")
        .flag();

    parser.add_argument("--client-tcp-keepalive")
        .help("enable tcp keepalive for client socket")
        .flag();

    parser.add_argument("--remote-tcp-nodelay")
        .help("enable tcp nodelay for remote socket")
        .flag();

    parser.add_argument("--remote-tcp-keepalive")
        .help("enable tcp keepalive for remote socket")
        .flag();

#if !SOCKSPP_DISABLE_LOGS
    parser.add_argument("--log-level")
        .help(
            "log/verbosity level\n"
            "{off, error, warning, info, debug}")
        .default_value("off")
        .choices("off", "error", "warning", "info", "debug")
        .nargs(1);
#endif

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        std::cerr << parser;
        std::exit(-1);
    }

    std::string listen_ip = parser.get<std::string>("--listen-ip");
    uint16_t listen_port = parser.get<uint16_t>("--listen-port");
    std::string username = parser.get<std::string>("--username");
    std::string password = parser.get<std::string>("--password");
    std::string dns_ip = parser.get<std::string>("--dns-ip");
    uint16_t dns_port = parser.get<uint16_t>("--dns-port");
    bool client_tcp_nodelay = parser.get<bool>("--client-tcp-nodelay");
    bool client_tcp_keepalive = parser.get<bool>("--client-tcp-keepalive");
    bool remote_tcp_nodelay = parser.get<bool>("--remote-tcp-nodelay");
    bool remote_tcp_keepalive = parser.get<bool>("--remote-tcp-keepalive");

#if !SOCKSPP_DISABLE_LOGS
    std::string log_level_str = parser.get<std::string>("--log-level");

    LogLevel log_level = LogLevel::Off;

    if (log_level_str == "error")
    {
        log_level = LogLevel::Error;
    }
    else if (log_level_str == "warning")
    {
        log_level = LogLevel::Warning;
    }
    else if (log_level_str == "info")
    {
        log_level = LogLevel::Info;
    }
    else if (log_level_str == "debug")
    {
        log_level = LogLevel::Debug;
    }

    SET_LOG_LEVEL(log_level);
#endif

    return sockspp::server::ServerParams{
        .listen_ip = listen_ip,
        .listen_port = listen_port,
        .username = username,
        .password = password,
        .dns_ip = dns_ip,
        .dns_port = dns_port,
        .client_tcp_nodelay = client_tcp_nodelay,
        .client_tcp_keepalive = client_tcp_keepalive,
        .remote_tcp_nodelay = remote_tcp_nodelay,
        .remote_tcp_keepalive = remote_tcp_keepalive
    };
}

#ifdef __cplusplus
extern "C" {
#endif

int sockspp_main(int argc, char* argv[])
{
    initialize();
    sockspp::server::ServerParams params = parse_params(argc, argv);

    sockspp_print_greeting();

    g_server = new sockspp::server::Server(std::move(params));
    prepare_sigint();
    g_server->serve();
    LOGI("Server stopped");

    return 0;
}

#ifdef __cplusplus
}
#endif
