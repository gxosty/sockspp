#pragma once

#define COLOR_BLACK   "\x1b[30m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"

#define COLOR_LIGHT_BLACK   "\x1b[90m" // Often appears as Dark Gray
#define COLOR_LIGHT_RED     "\x1b[91m"
#define COLOR_LIGHT_GREEN   "\x1b[92m"
#define COLOR_LIGHT_YELLOW  "\x1b[93m"
#define COLOR_LIGHT_BLUE    "\x1b[94m"
#define COLOR_LIGHT_MAGENTA "\x1b[95m"
#define COLOR_LIGHT_CYAN    "\x1b[96m"
#define COLOR_LIGHT_WHITE   "\x1b[97m" // Often just Bright White or slightly off-white

#define COLOR_RESET   "\x1b[0m"

enum class LogLevel
{
    Off,
    Error,
    Warning,
    Info,
    Debug
};

#define LOG_LEVEL_OFF LogLevel::Off
#define LOG_LEVEL_ERROR LogLevel::Error
#define LOG_LEVEL_WARNING LogLevel::Warning
#define LOG_LEVEL_INFO LogLevel::Info
#define LOG_LEVEL_DEBUG LogLevel::Debug

#if !SOCKSPP_DISABLE_LOGS

#include <stdio.h>
#include <time.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))

inline LogLevel _loglevel = LogLevel::Off;

#if SOCKSPP_ENABLE_LOCATION_LOGS
    #define _SOCKSPP_IN_TTFL_LOG(_color, _tag, _time_buffer, _filename, _line) \
        fprintf( \
            stderr, \
            "%s[%s] %s | %s:%d > %s", \
            _color, \
            _tag, \
            _time_buffer, \
            _filename, \
            _line, \
            COLOR_RESET \
        );
#else
    #define _SOCKSPP_IN_TTFL_LOG(_color, _tag, _time_buffer, _filename, _line) \
        fprintf( \
            stderr, \
            "%s[%s] %s > %s", \
            _color, \
            _tag, \
            _time_buffer, \
            COLOR_RESET \
        );
#endif // SOCKSPP_DISABLE_LOCATION_LOGS

#define _SOCKSPP_TTFL_LOG(_color, _tag, _current_time, _filename, _line, ...) \
    char _time_buffer[24]; strftime(_time_buffer, 24, "%H:%M:%S", _current_time); \
    _SOCKSPP_IN_TTFL_LOG(_color, _tag, _time_buffer, _filename, _line) \
    fprintf(stderr, __VA_ARGS__); \
    fputs("\n", stderr); \

#define _SOCKSPP_LOG(_tag, _level, ...) \
if (_level <= _loglevel) { \
    time_t _timer = time(NULL); \
    struct tm* _current_time = localtime(&_timer); \
    const char* _color = NULL; \
    switch (_level) { \
        case LOG_LEVEL_OFF: _color = COLOR_RESET; break; \
        case LOG_LEVEL_ERROR: _color = COLOR_RED; break; \
        case LOG_LEVEL_WARNING: _color = COLOR_YELLOW; break; \
        case LOG_LEVEL_INFO: _color = COLOR_LIGHT_BLUE; break; \
        case LOG_LEVEL_DEBUG: _color = COLOR_LIGHT_BLACK; break; \
    } \
    _SOCKSPP_TTFL_LOG(_color, _tag, _current_time, __FILENAME__, __LINE__, __VA_ARGS__) \
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
