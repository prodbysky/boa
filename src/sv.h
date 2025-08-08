#ifndef SV_H_
#define SV_H_
#include <stddef.h>

#define STR_FMT "%.*s"
#define STR_ARG(s) (int)(s).count, (s).items

/// Takes a `String` and makes a `StringView`
#define SV(s) (StringView) {.items = (s).items, .count = (s).count}

typedef struct {
    char* items;
    size_t count;
    size_t capacity;
} String;

typedef struct {
    char const* items;
    size_t count;
} StringView;
#endif 
