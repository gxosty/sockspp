#include <sockspp/server/server.hpp>
#include <sockspp/core/log.hpp>

static inline void sockspp_print_greeting()
{
    LOGI("%s v%s", SOCKSPP_NAME, SOCKSPP_VERSION);
}

#ifdef __cplusplus
extern "C" {
#endif

int sockspp_main(int argc, char* argv[])
{
    sockspp_print_greeting();
    
    return 0;
}

#ifdef __cplusplus
}
#endif
