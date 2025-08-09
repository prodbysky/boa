#ifndef SV_H_
#define SV_H_
#include <stddef.h>
#include <string.h>

#define STR_FMT "%.*s"
#define STR_ARG(s) (int)(s).count, (s).items

/// Takes a `String` and makes a `StringView`
#define SV(s)                                                                                                          \
    (StringView) { .items = (s).items, .count = (s).count }

/// Takes a C string and makes a `StringView`
#define SV_FROM_CSTR(s)                                                                                                \
    (StringView) { .items = (s), .count = strlen((s)) }

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String;

typedef struct {
    char const *items;
    size_t count;
} StringView;

typedef struct {
    String src;
    const char *name;
} SourceFile;

typedef struct {
    StringView src;
    const char *name;
} SourceFileView;

#define FILE_VIEW_FROM_FILE(sf)                                                                                        \
    (SourceFileView) { .src = SV((sf).src), .name = (sf).name }

#endif
