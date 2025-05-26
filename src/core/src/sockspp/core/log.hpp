#pragma once

#if !SOCKSPP_DISABLE_LOGS

#include <stdio.h>
#include <time.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

#if SOCKSPP_ENABLE_LOCATION_LOGS
    #define _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) printf("[%s][%s] %s:%d | ", _tag, _time_buffer, _filename, _line);
#else
    #define _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) printf("[%s][%s]: ", _tag, _time_buffer);
#endif // SOCKSPP_DISABLE_LOCATION_LOGS

#define _SOCKSPP_TTFL_LOG(_tag, _current_time, _filename, _line, ...) \
{ \
    char _time_buffer[24]; strftime(_time_buffer, 24, "%H:%M:%S", _current_time); \
    _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) \
    printf(__VA_ARGS__); \
    puts("\n"); \
}

#define _SOCKSPP_LOG(_tag, ...) \
{ \
    time_t _timer = time(NULL); \
    struct tm* _current_time = localtime(&_timer); \
    _SOCKSPP_TTFL_LOG(_tag, _current_time, __FILENAME__, __LINE__, __VA_ARGS__) \
}

#define LOGI(...) _SOCKSPP_LOG("I", __VA_ARGS__)
#define LOGD(...) _SOCKSPP_LOG("D", __VA_ARGS__)
#define LOGW(...) _SOCKSPP_LOG("W", __VA_ARGS__)
#define LOGE(...) _SOCKSPP_LOG("E", __VA_ARGS__)

#else

#define LOGI(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)

#endif // SOCKSPP_DISABLE_LOGS
