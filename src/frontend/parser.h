#ifndef PARSER_H_
#define PARSER_H_
#include "../arena.h"
#include "lexer.h"
#include <stddef.h>

typedef struct {
    TokensSlice tokens;
    Token last_token;

    SourceFileView origin;

    Arena *arena;
} Parser;

typedef enum { AET_PRIMARY, AET_BINARY, AET_IDENT, } AstExpressionType;

typedef struct AstExpression {
    AstExpressionType type;
    const char *begin;
    ptrdiff_t len;
    union {
        uint64_t number;
        StringView ident;
        struct {
            struct AstExpression *l;
            OperatorType op;
            struct AstExpression *r;
        } bin;
    };
} AstExpression;

typedef enum { AST_RETURN, AST_LET, AST_ASSIGN } AstStatementType;

typedef struct {
    AstStatementType type;
    const char *begin;
    ptrdiff_t len;
    union {
        struct {
            bool has_expr;
            AstExpression return_expr;
        } ret;
        struct {
            StringView name;
            AstExpression value;
        } let, assign;
    };
} AstStatement;

typedef struct {
    AstStatement *items;
    SourceFileView source;
    size_t count;
    size_t capacity;
} AstTree;

bool parser_parse(Parser *parser, AstTree *out);

bool parser_is_empty(const Parser *parser);
Token parser_pop(Parser *parser);
Token parser_peek(const Parser *parser, size_t offset);

bool parser_expect_ident(Parser* parser, StringView* out);
bool parser_expect_and_skip(Parser* parser, TokenType type);

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
