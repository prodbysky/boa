#ifndef LEXER_H_
#define LEXER_H_
#include "sv.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    StringView src;
    const char* begin_of_src;
    const char* input_name;
} Lexer;

typedef enum {
    TT_NUMBER = 0,
    TT_OPERATOR = 1,
} TokenType;

typedef enum {
    OT_PLUS,
    OT_MINUS,
    OT_MULT,
    OT_DIV,
} OperatorType;

typedef struct {
    TokenType type;
    const char* begin;
    ptrdiff_t len;
    union {
        uint64_t number;
        OperatorType operator;
    };
} Token;

typedef struct {
    Token* items;
    size_t count;
    size_t capacity;
} Tokens;

bool lexer_run(Lexer* lexer, Tokens* out);
bool lexer_lex_number(Lexer* lexer, Token* out);

bool lexer_is_empty(const Lexer* lexer);

/// NOTE: Returns 0 if the offset would cause a buffer overflow or the `src` is empty (src.count == 0)
char lexer_peek(const Lexer* lexer, size_t offset);
char lexer_consume(Lexer* lexer);

void lexer_skip_ws(Lexer* lexer);

void report_error(const char *begin, const char *end, const char *src,
                  const char *name);

#endif 
