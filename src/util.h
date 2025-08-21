#ifndef UTIL_H_
#define UTIL_H_

#include "../src/log.h"
#include "arena.h"
#include "sv.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

#define da_push(arr, item, arena)                                                                                      \
    if ((arr)->capacity == 0) {                                                                                        \
        (arr)->items = arena_alloc(arena, sizeof(*(arr)->items) * 16);                                                 \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        (arr)->count = 0;                                                                                              \
        (arr)->capacity = 16;                                                                                          \
    }                                                                                                                  \
    if ((arr)->capacity <= (arr)->count) {                                                                             \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        void *new_items = arena_alloc(arena, sizeof(*(arr)->items) * (arr)->capacity * 1.5);                           \
        memmove(new_items, (arr)->items, sizeof(*(arr)->items) * (arr)->capacity); \
        (arr)->items = new_items;                           \
        ASSERT((arr)->items, "Buy more RAM LOLOL");                                                                    \
        (arr)->capacity *= 1.5;                                                                                        \
    }                                                                                                                  \
    (arr)->items[(arr)->count++] = item;


#define da_remove(arr, index)                                                                                          \
    ASSERT((arr)->count >= index && index >= 0, "Tried to remove invalid index from array");                           \
    if (index < (arr)->count - 1) {                                                                                    \
        memmove(&(arr)->items[index], &(arr)->items[index + 1], ((arr)->count - index - 1) * sizeof(*(arr)->items));   \
    }                                                                                                                  \
    (arr)->count--;

typedef struct {
    String path;
} Path;

Path path_from_cstr(char *cstr, Arena *arena);
void path_add_ext(Path *path, const char *ext, Arena* arena);

char *path_to_cstr(Path *path, Arena *arena);

/*
 * Argument `file_name`: input file name (null-terminated C-str)
 * Argument `s`: output parameter which will be filled out
 * Return: indicates failure (where false is failed read operation)
 */
bool read_file(const char *file_name, String *s, Arena *arena);
bool read_source_file(const char *file_name, SourceFile *out, Arena *arena);

int run_program(const char *prog, int argc, char *argv[]);

#endif
