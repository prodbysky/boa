#ifndef ARENA_H_
#define ARENA_H_
#include <stddef.h>

typedef struct {
    char* buffer;
    size_t used;
    size_t cap;
} Arena;

Arena arena_new(size_t size);
void* arena_alloc(Arena* arena, size_t size);
void arena_free(Arena* arena);

#endif
