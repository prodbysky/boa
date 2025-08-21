#include "parser.h"
#include "../log.h"
#include "../util.h"
#include "lexer.h"

bool parser_parse(Parser *parser, AstRoot *out) {
    ASSERT(parser, "Uh oh");
    ASSERT(out, "Uh oh");

    out->source = parser->origin;

    while (!parser_is_empty(parser)) {
        Token t = parser_pop(parser);
        if (t.type == TT_KEYWORD && t.keyword == KT_DEF) {
            StringView name = {0};
            if (!parser_expect_ident(parser, &name)) {
                log_diagnostic(LL_ERROR, "Expected a name for a function to be here");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            AstFunction f = {0};
            f.name = name;
            if (!parser_expect_and_skip(parser, TT_OPEN_PAREN)) {
                log_diagnostic(LL_ERROR, "Expected an `(` symbol here to denote an argument list");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_CLOSE_PAREN) {
                StringView arg_name = {0};
                if (!parser_expect_ident(parser, &arg_name)) {
                    log_diagnostic(LL_ERROR, "Expected an argument name here");
                    report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                                 parser->origin.src.items, parser->origin.name);
                    return false;
                }
                da_push(&f.args, arg_name, parser->arena);
                if (parser_is_empty(parser)) {
                    log_diagnostic(LL_ERROR, "Unexpected end of input in argument list");
                    return false;
                }

                Token next = parser_peek(parser, 0);
                if (next.type == TT_CLOSE_PAREN) {
                    break;
                } else if (next.type == TT_COMMA) {
                    parser_pop(parser);
                    if (parser_is_empty(parser) || parser_peek(parser, 0).type == TT_CLOSE_PAREN) {
                        log_diagnostic(LL_ERROR, "Expected argument name after comma");
                        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                                     parser->origin.src.items, parser->origin.name);
                        return false;
                    }
                } else {
                    log_diagnostic(LL_ERROR, "Expected comma or closing parenthesis in argument list");
                    report_error(next.begin, next.begin + next.len, parser->origin.src.items, parser->origin.name);
                    return false;
                }
            }
            if (!parser_expect_and_skip(parser, TT_CLOSE_PAREN)) {
                log_diagnostic(LL_ERROR, "Argument list wasn't terminated with a `)`");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            if (!parser_parse_block(parser, &f.body)) return false;
            da_push(&out->fs, f, parser->arena);
        }
    }

    return true;
}

bool parser_parse_block(Parser *parser, AstBlock *out) {
    if (!parser_expect_and_skip(parser, TT_OPEN_CURLY)) {
        log_diagnostic(LL_ERROR, "Expected a `{` to begin a block");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }
    while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_CLOSE_CURLY) {
        AstStatement st = {0};
        if (!parser_parse_statement(parser, &st)) return false;
        da_push(out, st, parser->arena);
    }
    if (parser_is_empty(parser)) {
        log_diagnostic(LL_ERROR, "Expected a `}` to end a block");
        report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                     parser->origin.src.items, parser->origin.name);
        return false;
    }
    // we can ignore this return value, since the other only condition has been checked
    parser_expect_and_skip(parser, TT_CLOSE_CURLY);

    return true;
}

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

bool parser_expect_ident(Parser *parser, StringView *out) {
    if (parser_is_empty(parser)) return false;
    if (parser_peek(parser, 0).type == TT_IDENT) {
        *out = parser_pop(parser).identifier;
        return true;
    }
    return false;
}

bool parser_expect_and_skip(Parser *parser, TokenType type) {
    if (parser_is_empty(parser)) return false;

    if (parser_peek(parser, 0).type == type) {
        parser_pop(parser);
        return true;
    }

    return false;
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
    case TT_IDENT: {
        if (!parser_is_empty(parser) && parser_peek(parser, 0).type == TT_OPEN_PAREN) {
            out->type = AET_FUNCTION_CALL;
            parser_pop(parser);
            Token closing_paren = parser_peek(parser, 0);
            while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_CLOSE_PAREN) {
                AstExpression arg = {0};
                if (!parser_parse_expr(parser, &arg)) return false;
                da_push(&out->func_call.args, arg, parser->arena);
                if (!parser_expect_and_skip(parser, TT_COMMA)) break;
            }
            if (!parser_expect_and_skip(parser, TT_CLOSE_PAREN)) {
                log_diagnostic(LL_ERROR, "Argument list wasn't terminated with a `)`");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            out->len = closing_paren.begin + 1 - t.begin;
            out->func_call.name = t.identifier;
            out->begin = t.begin;
            return true;
        }
        out->type = AET_IDENT;
        out->len = t.identifier.count;
        out->ident = t.identifier;
        out->begin = t.begin;
        return true;
    }
    case TT_DOUBLE_QUOTE: {
        while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_DOUBLE_QUOTE) parser_pop(parser);
        if (parser_is_empty(parser)) {
            log_diagnostic(LL_ERROR, "Unterminated string literal");
            report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                         parser->origin.src.items, parser->origin.name);
            return false;
        }
        parser_pop(parser);
        out->type = AET_STRING;
        out->begin = t.begin;
        out->len = parser->last_token.begin - t.begin;
        out->string.items = t.begin + 1;
        out->string.count = (parser->last_token.begin - t.begin) - 1;
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

// statements that require a semicolon break out of the switch
// those that don't return true, but they have to setup the length of themselves
bool parser_parse_statement(Parser *parser, AstStatement *out) {
    out->begin = parser->tokens.items[0].begin;
    Token t = parser_pop(parser);

    if (t.type == TT_KEYWORD) {
        switch (t.keyword) {
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
                break;
            }
            }
            break;
        }
        case KT_LET: {
            if (!parser_expect_ident(parser, &out->let.name)) {
                log_diagnostic(LL_ERROR, "Expected a name for a variable definition here");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            if (!parser_expect_and_skip(parser, TT_ASSIGN)) {
                log_diagnostic(LL_ERROR, "Expected `=` here after the name of the let binding");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }

            if (!parser_parse_expr(parser, &out->let.value)) return false;
            out->type = AST_LET;
            break;
        }
        case KT_IF: {
            AstExpression cond = {0};
            if (!parser_parse_expr(parser, &cond)) return false;
            out->if_st.cond = cond;
            if (!parser_parse_block(parser, &out->if_st.block)) return false;
            out->len = (parser->last_token.begin + parser->last_token.len) - out->begin;
            out->type = AST_IF;
            return true;
        }
        case KT_WHILE: {
            AstExpression cond = {0};
            if (!parser_parse_expr(parser, &cond)) return false;
            out->if_st.cond = cond;
            if (!parser_parse_block(parser, &out->if_st.block)) return false;
            out->len = (parser->last_token.begin + parser->last_token.len) - out->begin;
            out->type = AST_WHILE;
            return true;
        }
        case KT_DEF: {
            log_message(LL_INFO, "Nested functions aren't supported");
            TODO();
            break;
        }
        case KT_ASM: {
            out->type = AST_ASM;
            if (!parser_expect_and_skip(parser, TT_OPEN_PAREN)) {
                log_diagnostic(LL_ERROR, "Expected `(` to denote the beginning of the asm statement");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            const char *begin = parser->last_token.begin + 1;
            while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_CLOSE_PAREN) parser_pop(parser);
            const char *end = parser->last_token.begin + parser->last_token.len;
            out->asm = (StringView){.items = begin, .count = end - begin};
            return true;
        }
        }
    } else if (t.type == TT_IDENT) {
        switch (parser_peek(parser, 0).type) {
        case TT_ASSIGN: {
            out->type = AST_ASSIGN;
            out->assign.name = t.identifier;
            parser_pop(parser);
            if (!parser_parse_expr(parser, &out->assign.value)) return false;
            break;
        }
        case TT_OPEN_PAREN: {
            out->type = AST_CALL;
            out->call.name = t.identifier;
            parser_pop(parser);
            while (!parser_is_empty(parser) && parser_peek(parser, 0).type != TT_CLOSE_PAREN) {
                AstExpression arg = {0};
                if (!parser_parse_expr(parser, &arg)) { return false; }
                da_push(&out->call.args, arg, parser->arena);
                if (!parser_expect_and_skip(parser, TT_COMMA)) { break; }
            }
            if (!parser_expect_and_skip(parser, TT_CLOSE_PAREN)) {
                log_diagnostic(LL_ERROR, "Argument list wasn't terminated with a `)`");
                report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                             parser->origin.src.items, parser->origin.name);
                return false;
            }
            break;
        }
        default: {
            log_diagnostic(LL_ERROR, "Expected `=` after this identifier");
            report_error(parser->last_token.begin, parser->last_token.begin + parser->last_token.len,
                         parser->origin.src.items, parser->origin.name);
            return false;
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
