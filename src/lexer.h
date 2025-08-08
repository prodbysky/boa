#ifndef LEXER_H_
#define LEXER_H_
#include "sv.h"
#include <stdbool.h>

typedef struct {
    StringView src;
    const char* begin_of_src;
} Lexer;

bool lexer_is_empty(const Lexer* lexer);

/// NOTE: Returns 0 if the offset would cause a buffer overflow or the `src` is empty (src.count == 0)
char lexer_peek(const Lexer* lexer, size_t offset);
char lexer_consume(Lexer* lexer);

#endif 
