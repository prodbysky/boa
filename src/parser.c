#include "parser.h"
#include "lexer.h"
#include "log.h"
#include "util.h"
#include <wchar.h>

Token parser_pop(Parser *parser) {
    ASSERT(!parser_is_empty(parser), "Caller ensures this");
    Token t = parser->tokens.items[0];
    parser->last_token = t;
    parser->tokens.count--;
    parser->tokens.items++;
    return t;
}

bool parser_is_empty(const Parser *parser) { return parser->tokens.count == 0; }
Token parser_peek(const Parser *parser, size_t offset) {
    ASSERT(parser->tokens.count > offset, "Tried to access tokens out of bounds");
    return parser->tokens.items[offset];
}

bool parser_parse_expr(Parser *parser, AstExpression *out) { return parser_parse_term(parser, out); }

bool parser_parse_primary(Parser *parser, AstExpression *out) {
    if (parser_is_empty(parser)) {
        log_diagnostic(LL_ERROR, "Expected an expression to be here");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }
    Token t = parser_pop(parser);
    switch (t.type) {
    case TT_NUMBER: {
        out->type = AET_PRIMARY;
        out->len = t.len;
        out->number = t.number;
        out->begin = t.begin;
        return true;
    }
    default: {
        log_diagnostic(LL_ERROR, "Expected an expression to be here");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }
    }
    return true;
}
bool parser_parse_factor(Parser *parser, AstExpression *out) {
    if (parser_is_empty(parser)) {
        log_diagnostic(LL_ERROR, "Expected an expression to be here");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }

    if (!parser_parse_primary(parser, out)) return false;

    while (!parser_is_empty(parser) && parser_peek(parser, 0).type == TT_OPERATOR &&
           (parser_peek(parser, 0).operator== OT_MULT || parser_peek(parser, 0).operator== OT_DIV)) {

        Token op = parser_pop(parser);

        AstExpression *lhs = arena_alloc(parser->arena, sizeof(AstExpression));
        *lhs = *out;

        AstExpression *rhs = arena_alloc(parser->arena, sizeof(AstExpression));
        if (!parser_parse_primary(parser, rhs)) return false;

        out->type = AET_BINARY;
        out->len = (rhs->begin + rhs->len) - lhs->begin;
        out->bin.op = op.operator;
        out->bin.l = lhs;
        out->bin.r = rhs;
    }
    return true;
}
bool parser_parse_term(Parser *parser, AstExpression *out) {
    if (parser_is_empty(parser)) {
        log_diagnostic(LL_ERROR, "Expected an expression to be here");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }

    if (!parser_parse_factor(parser, out)) return false;

    while (!parser_is_empty(parser) && parser_peek(parser, 0).type == TT_OPERATOR &&
           (parser_peek(parser, 0).operator== OT_PLUS || parser_peek(parser, 0).operator== OT_MINUS)) {

        Token op = parser_pop(parser);

        AstExpression *lhs = arena_alloc(parser->arena, sizeof(AstExpression));
        *lhs = *out;

        AstExpression *rhs = arena_alloc(parser->arena, sizeof(AstExpression));
        if (!parser_parse_factor(parser, rhs)) return false;
        out->type = AET_BINARY;
        out->bin.op = op.operator;
        out->len = (rhs->begin + rhs->len) - lhs->begin;
        out->bin.l = lhs;
        out->bin.r = rhs;
    }
    return true;
}

bool parser_parse_statement(Parser *parser, AstStatement *out) {
    ASSERT(parser_peek(parser, 0).type == TT_KEYWORD, "The caller ensures this condition");

    out->begin = parser->tokens.items[0].begin;

    switch (parser_pop(parser).keyword) {
    case KT_NO: UNREACHABLE("This is an error value for the KeywordType enum");
    case KT_RETURN: {
        if (parser_is_empty(parser)) {
            log_diagnostic(LL_ERROR, "Expected a semicolon or an expression here not EOF");
            report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                         parser->origin.src.items, parser->origin.name);
            return false;
        }
        out->type = AST_RETURN;
        switch (parser_peek(parser, 0).type) {
        case TT_SEMICOLON: {
            out->ret.has_expr = false;
            parser_pop(parser);
            out->len = (parser->last_token.begin + parser->last_token.len) - out->begin;
            return true;
        }
        default: {
            if (!parser_parse_expr(parser, &out->ret.return_expr)) return false;
            out->ret.has_expr = true;
        }
        }
    }
    }

    if (parser_is_empty(parser) || parser_peek(parser, 0).type != TT_SEMICOLON) {
        log_diagnostic(LL_ERROR, "Expected a semicolon here");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }
    parser_pop(parser);
    out->len = (parser->last_token.begin + parser->last_token.len) - out->begin;

    return true;
}
