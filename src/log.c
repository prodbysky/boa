#include "log.h"


void log_message_(LogLevel level,
                  const char *format,
                  const char *file,
                  size_t line,
                  ...) {
    char *log_level = ANSI_GRAY"UNKNOWN"ANSI_RESET;
    switch (level) {
        case LL_INFO: log_level = ANSI_BLUE"INFO"ANSI_RESET; break;
        case LL_WARN: log_level = ANSI_YELLOW"WARN"ANSI_RESET; break;
        case LL_ERROR: log_level = ANSI_RED"ERROR"ANSI_RESET; break;
    }

    fprintf(stderr, "%s:%lu:1 [%s]: ", file, line, log_level);
    va_list ap;
    va_start(ap, line);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void log_diagnostic_(LogLevel level, const char *format, ...) {
    char *log_level = ANSI_GRAY"UNKNOWN"ANSI_RESET;
    switch (level) {
        case LL_INFO: log_level = ANSI_BLUE"INFO"ANSI_RESET; break;
        case LL_WARN: log_level = ANSI_YELLOW"WARN"ANSI_RESET; break;
        case LL_ERROR: log_level = ANSI_RED"ERROR"ANSI_RESET; break;
    }

    fprintf(stderr, "[%s]: ", log_level);
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
}
