#ifndef UTIL_H_
#define UTIL_H_

#include "../src/log.h"
#include "sv.h"
#include <stdbool.h>
#include <stdlib.h>

#define TODO()                                                                                                         \
    log_message(LL_ERROR, "Not implemented yet");                                                                      \
    exit(1);

#define ASSERT(cond, msg)                                                                                              \
    if (!(cond)) {                                                                                                     \
        log_message(LL_ERROR, "Assert Failed: %s", msg);                                                               \
        exit(1);                                                                                                       \
    }

#define UNREACHABLE(msg)                                                                                               \
    log_message(LL_ERROR, "Unreachable reached: %s", msg);                                                             \
    exit(1);

#define da_push(arr, item)                                                                                             \
    if ((arr)->capacity == 0) {                                                                                        \
        (arr)->items = malloc(sizeof(*(arr)->items) * 16);                                                             \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        (arr)->count = 0;                                                                                              \
        (arr)->capacity = 16;                                                                                          \
    }                                                                                                                  \
    if ((arr)->capacity <= (arr)->count) {                                                                             \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        (arr)->items = realloc((arr)->items, sizeof(*(arr)->items) * (arr)->capacity * 1.5);                           \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        (arr)->capacity *= 1.5;                                                                                        \
    }                                                                                                                  \
    (arr)->items[(arr)->count++] = item;

#define da_remove(arr, index)                                                                                          \
    ASSERT((arr)->count >= index && index >= 0, "Tried to remove invalid index from array");                           \
    if (index < (arr)->count - 1) {                                                                                    \
        memmove(&(arr)->items[index], &(arr)->items[index + 1], ((arr)->size - index - 1) * sizeof(*(arr)->items));    \
    }                                                                                                                  \
    (arr)->count--;

typedef struct {
    String path;
} Path;

Path path_from_cstr(char *cstr);
void path_add_ext(Path *path, const char *ext);

char *path_to_cstr(Path *path);

/*
 * Argument `file_name`: input file name (null-terminated C-str)
 * Argument `s`: output parameter which will be filled out
 * Return: indicates failure (where false is failed read operation)
 */
bool read_file(const char *file_name, String *s);
bool read_source_file(const char *file_name, SourceFile *out);

int run_program(const char *prog, char *args[]);

#endif
