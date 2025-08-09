#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include "../src/log.h"

#define TODO()                                                                                                         \
    do {                                                                                                               \
        log_message(LL_ERROR, "Not implemented yet");                                                                  \
        exit(1);                                                                                                       \
    } while (false)

#define ASSERT(cond, msg)                                                                                              \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            log_message(LL_ERROR, "Assert Failed: %s", msg);                                                           \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (false)

#define UNREACHABLE(msg)                                                                                               \
    log_message(LL_ERROR, "Unreachable reached: %s", msg);                                                             \
    exit(1);

#define da_push(arr, item)                                                                                             \
    do {                                                                                                               \
        if ((arr)->capacity == 0) {                                                                                    \
            (arr)->items = malloc(sizeof(*(arr)->items) * 16);                                                         \
            ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                \
            (arr)->count = 0;                                                                                          \
            (arr)->capacity = 16;                                                                                      \
        }                                                                                                              \
        if ((arr)->capacity <= (arr)->count) {                                                                         \
            ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                \
            (arr)->items = realloc((arr)->items, sizeof(*(arr)->items) * (arr)->capacity * 1.5);                       \
            ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                \
            (arr)->capacity *= 1.5;                                                                                    \
        }                                                                                                              \
        (arr)->items[(arr)->count++] = item;                                                                           \
    } while (false)

#endif
