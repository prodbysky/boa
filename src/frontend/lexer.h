#ifndef LEXER_H_
#define LEXER_H_
#include "../sv.h"
#include "../arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    SourceFileView file;
    const char *begin_of_src;
    Arena* arena;
} Lexer;

typedef enum {
    TT_NUMBER = 0,
    TT_OPERATOR,
    TT_KEYWORD,
    TT_IDENT,
    TT_SEMICOLON,
    TT_ASSIGN,
    TT_OPEN_CURLY,
    TT_CLOSE_CURLY,
    TT_OPEN_PAREN,
    TT_CLOSE_PAREN,
    TT_DOUBLE_QUOTE,
    TT_COMMA,
} TokenType;

typedef enum {
    OT_PLUS,
    OT_MINUS,
    OT_MULT,
    OT_DIV,
    OT_LT,
    OT_MT,
} OperatorType;

typedef enum {
    KT_NO = 0,
    KT_RETURN,
    KT_LET,
    KT_DEF,
    KT_IF,
    KT_WHILE,
    KT_ASM,
} KeywordType;

typedef struct {
    TokenType type;
    const char *begin;
    ptrdiff_t len;
    union {
        uint64_t number;
        OperatorType operator;
        KeywordType keyword;
        StringView identifier;
    };
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} Tokens;

typedef struct {
    Token *items;
    size_t count;
} TokensSlice;

bool lexer_run(Lexer *lexer, Tokens *out);
bool lexer_lex_number(Lexer *lexer, Token *out);
bool lexer_lex_ident_or_keyword(Lexer *lexer, Token *out);
bool lexer_ident_is_keyword(const char *begin, ptrdiff_t len);
KeywordType lexer_keyword(const char *begin, ptrdiff_t len);

bool lexer_is_empty(const Lexer *lexer);

/// NOTE: Returns 0 if the offset would cause a buffer overflow or the `src` is empty (src.count == 0)
char lexer_peek(const Lexer *lexer, size_t offset);
char lexer_consume(Lexer *lexer);

void lexer_skip_ws(Lexer *lexer);

void report_error(const char *begin, const char *src, const char *name);

#endif
