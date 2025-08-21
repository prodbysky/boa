#include "arena.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

Arena arena_new(size_t size) {
    char *buffer = malloc(sizeof(char) * size);
    ASSERT(buffer, "Buy more RAM LOLOLOL");
    return (Arena){.buffer = buffer, .cap = size, .used = 0};
}
void *arena_alloc(Arena *arena, size_t size) {
    ASSERT(arena, "House keeping");
    ASSERT(arena->buffer, "House keeping");
    ASSERT(arena->used + size < arena->cap, "Arena overflow");
    void* buf = &arena->buffer[arena->used];
    arena->used += size;
    return buf;
}
void arena_free(Arena *arena) {
    ASSERT(arena, "House keeping");
    ASSERT(arena->buffer, "House keeping");
    free(arena->buffer);
    memset(arena, 0, sizeof(Arena));
}
