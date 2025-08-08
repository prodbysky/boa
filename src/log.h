#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>

typedef enum {
    LL_INFO,
    LL_ERROR,
    LL_WARN,
} LogLevel;

// https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
#define ANSI_RED "\033[1;31m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BLUE "\033[1;34m"
#define ANSI_GRAY "\033[1;90m"

#define ANSI_RESET "\033[0m"

#define log_message(level, fmt, ...) \
    log_message_(level, fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

void log_message_(LogLevel level,
                  const char *format,
                  const char *function,
                  const char *file,
                  size_t line,
                  ...);
#endif
