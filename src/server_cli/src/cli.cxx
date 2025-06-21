#ifdef _WIN32
    #include <winsock2.h>
#endif // _WIN32

#include <sockspp/server/server.hpp>
#include <sockspp/core/log.hpp>

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

#ifdef __cplusplus
extern "C" {
#endif

int sockspp_main(int argc, char* argv[])
{
    sockspp_print_greeting();
    initialize();

    return 0;
}

#ifdef __cplusplus
}
#endif
