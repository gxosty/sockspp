#pragma once

#if !SOCKSPP_DISABLE_LOGS

#include <stdio.h>
#include <time.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

enum class LogLevel
{
    Off,
    Error,
    Warning,
    Info,
    Debug
};

inline LogLevel _loglevel = LogLevel::Off;

#if SOCKSPP_ENABLE_LOCATION_LOGS
    #define _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) fprintf(stderr, "[%s][%s] %s:%d | ", _tag, _time_buffer, _filename, _line);
#else
    #define _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) fprintf(stderr, "[%s][%s]: ", _tag, _time_buffer);
#endif // SOCKSPP_DISABLE_LOCATION_LOGS

#define _SOCKSPP_TTFL_LOG(_tag, _current_time, _filename, _line, ...) \
    char _time_buffer[24]; strftime(_time_buffer, 24, "%H:%M:%S", _current_time); \
    _SOCKSPP_IN_TTFL_LOG(_tag, _time_buffer, _filename, _line) \
    fprintf(stderr, __VA_ARGS__); \
    fputs("\n", stderr); \

#define _SOCKSPP_LOG(_tag, _level, ...) \
if (_level <= _loglevel) { \
    time_t _timer = time(NULL); \
    struct tm* _current_time = localtime(&_timer); \
    _SOCKSPP_TTFL_LOG(_tag, _current_time, __FILENAME__, __LINE__, __VA_ARGS__) \
}0

#define LOGI(...) _SOCKSPP_LOG("I", LogLevel::Info, __VA_ARGS__)
#define LOGD(...) _SOCKSPP_LOG("D", LogLevel::Debug, __VA_ARGS__)
#define LOGW(...) _SOCKSPP_LOG("W", LogLevel::Warning, __VA_ARGS__)
#define LOGE(...) _SOCKSPP_LOG("E", LogLevel::Error, __VA_ARGS__)
#define SET_LOG_LEVEL(_level) (_loglevel = _level)
#define LOG_SCOPE(_level) if (_level <= _loglevel)

#else

#define LOGI(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)
#define SET_LOG_LEVEL(_level)
#define LOG_SCOPE(_level)

#endif // SOCKSPP_DISABLE_LOGS
