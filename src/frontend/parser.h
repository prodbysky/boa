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

typedef enum { AET_PRIMARY, AET_BINARY, AET_IDENT, AET_FUNCTION_CALL } AstExpressionType;

typedef struct AstExpression AstExpression;
typedef struct AstStatement AstStatement;

typedef struct {
    AstStatement *items;
    size_t count;
    size_t capacity;
} AstBlock;

// Where a function is called
typedef struct {
    AstExpression *items;
    size_t count;
    size_t capacity;
} FunctionArgsIn;

// Where a function is defined
// TODO: Types (since all things are just u64 now)
typedef struct {
    StringView *items;
    size_t count;
    size_t capacity;
} FunctionArgsOut;

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
        struct {
            StringView name;
            FunctionArgsIn args;
        } func_call;
    };
} AstExpression;

typedef enum {
    AST_RETURN,
    AST_LET,
    AST_ASSIGN,
    AST_CALL,
    AST_IF,
    AST_WHILE,
} AstStatementType;

struct AstStatement{
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
        struct {
            StringView name;
            FunctionArgsIn args;
        } call;
        struct {
            AstExpression cond;
            AstBlock block;
        } if_st;
        struct {
            AstExpression cond;
            AstBlock block;
        } while_st;
    };
};


typedef struct {
    AstStatement *items;
    SourceFileView source;
    size_t count;
    size_t capacity;
} AstTree;


typedef struct {
    StringView name;
    AstBlock body;
    FunctionArgsOut args;
} AstFunction;

typedef struct {
    AstFunction *items;
    size_t count;
    size_t capacity;
} AstFunctions;

typedef struct {
    AstFunctions fs;
    SourceFileView source;
} AstRoot;

bool parser_parse(Parser *parser, AstRoot *out);

bool parser_is_empty(const Parser *parser);
Token parser_pop(Parser *parser);
Token parser_peek(const Parser *parser, size_t offset);

bool parser_expect_ident(Parser *parser, StringView *out);
bool parser_expect_and_skip(Parser *parser, TokenType type);

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
bool parser_parse_block(Parser* parser, AstBlock* out);

#endif
