#ifndef PARSER_H_
#define PARSER_H_
#include "arena.h"
#include "lexer.h"
#include <stddef.h>

typedef struct {
    TokensSlice tokens;
    Token last_token;

    SourceFileView origin;

    Arena *arena;
} Parser;

typedef enum { AET_PRIMARY, AET_BINARY } AstExpressionType;

typedef struct AstExpression {
    AstExpressionType type;
    const char *begin;
    ptrdiff_t len;
    union {
        uint64_t number;
        struct {
            struct AstExpression *l;
            OperatorType op;
            struct AstExpression *r;
        } bin;
    };
} AstExpression;

typedef enum { AST_RETURN } AstStatementType;

typedef struct {
    AstStatementType type;
    const char *begin;
    ptrdiff_t len;
    union {
        struct {
            bool has_expr;
            AstExpression return_expr;
        } ret;
    };
} AstStatement;

bool parser_is_empty(const Parser *parser);
Token parser_pop(Parser *parser);
Token parser_peek(const Parser *parser, size_t offset);

/*
    https://timothya.com/pdfs/crafting-interpreters.pdf
    expression → equality ;
    equality → comparison ( ( "!=" | "==" ) comparison )* ;
    comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    term → factor ( ( "-" | "+" ) factor )* ;
    factor → unary ( ( "/" | "*" ) unary )* ;
    unary → ( "!" | "-" ) unary
    | primary ;
    primary → NUMBER | STRING | "true" | "false" | "nil"
    | "(" expression ")" ;
*/
bool parser_parse_expr(Parser *parser, AstExpression *out);
bool parser_parse_factor(Parser *parser, AstExpression *out);
bool parser_parse_term(Parser *parser, AstExpression *out);
bool parser_parse_primary(Parser *parser, AstExpression *out);

bool parser_parse_statement(Parser *parser, AstStatement *out);

#endif
