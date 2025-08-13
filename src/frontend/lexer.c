#include "lexer.h"
#include "../log.h"
#include "../util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const OperatorType char_to_op[] = {
    ['+'] = OT_PLUS,
    ['-'] = OT_MINUS,
    ['*'] = OT_MULT,
    ['/'] = OT_DIV,
};

bool lexer_run(Lexer *lexer, Tokens *out) {
    ASSERT(lexer, "Passed null lexer");
    ASSERT(out, "Passed null tokens out array");
    while (!lexer_is_empty(lexer)) {
        lexer_skip_ws(lexer);
        if (lexer_is_empty(lexer)) return true;

        if (isdigit(lexer_peek(lexer, 0))) {
            Token t = {0};
            if (!lexer_lex_number(lexer, &t)) return false;
            da_push(out, t);
            continue;
        }

        if (lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0))) {
            Token t = {0};
            if (!lexer_lex_ident_or_keyword(lexer, &t)) return false;
            da_push(out, t);
            continue;
        }

        switch (lexer_peek(lexer, 0)) {
        case 0: UNREACHABLE("The lexer can't be empty in here");
        case '+':
        case '-':
        case '*':
        case '/': {
            Token t = {0};
            t.type = TT_OPERATOR;
            t.len = 1;
            t.begin = lexer->file.src.items;
            t.operator= char_to_op[(size_t)lexer_peek(lexer, 0)];
            da_push(out, t);
            lexer_consume(lexer);
            continue;
        }
        case ';': {
            Token t = {0};
            t.type = TT_SEMICOLON;
            t.len = 1;
            t.begin = lexer->file.src.items;
            da_push(out, t);
            lexer_consume(lexer);
            continue;
        }
        case '=': {
            Token t = {0};
            t.type = TT_ASSIGN;
            t.len = 1;
            t.begin = lexer->file.src.items;
            da_push(out, t);
            lexer_consume(lexer);
            continue;
        }
        }

        TODO();
    }

    return true;
}

bool lexer_lex_ident_or_keyword(Lexer *lexer, Token *out) {
    ASSERT(!lexer_is_empty(lexer), "The caller ensures this condition");
    ASSERT(lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0)), "The caller ensures this condition");
    const char *begin = lexer->file.src.items;

    while (!lexer_is_empty(lexer) && (lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0))))
        lexer_consume(lexer);

    const char *end = lexer->file.src.items;

    if (lexer_ident_is_keyword(begin, end - begin)) {
        out->type = TT_KEYWORD;
        out->begin = begin;
        out->len = end - begin;
        out->keyword = lexer_keyword(begin, end - begin);
    } else {
        out->type = TT_IDENT;
        out->begin = begin;
        out->len = end - begin;
        out->identifier = (StringView){
            .items = begin,
            .count = out->len,
        };
    }

    return true;
}

// TODO: Make this "better"
bool lexer_ident_is_keyword(const char *begin, ptrdiff_t len) {
    if (strncmp(begin, "return", len) == 0) return true;
    if (strncmp(begin, "let", len) == 0) return true;
    return false;
}

KeywordType lexer_keyword(const char *begin, ptrdiff_t len) {
    if (strncmp(begin, "return", len) == 0) return KT_RETURN;
    if (strncmp(begin, "let", len) == 0) return KT_LET;
    return KT_NO;
}

bool lexer_lex_number(Lexer *lexer, Token *out) {
    ASSERT(!lexer_is_empty(lexer), "The caller ensures this condition");
    ASSERT(isdigit(lexer_peek(lexer, 0)), "The caller ensures this condition");
    const char *begin = lexer->file.src.items;
    while (!lexer_is_empty(lexer) && isdigit(lexer_peek(lexer, 0))) lexer_consume(lexer);
    const char *end = lexer->file.src.items;
    if (isalpha(*end)) {
        log_diagnostic(LL_ERROR, "You can't have numeric literal next to "
                                 "any alphabetic characters");
        report_error(begin, end, lexer->begin_of_src, lexer->file.name);
        return false;
    }
    out->type = TT_NUMBER;
    out->begin = begin;
    out->len = end - begin;
    char *got_end = NULL;
    out->number = strtoull(begin, &got_end, 10);
    ASSERT(got_end == end, "LEXER BUG");
    return true;
}

bool lexer_is_empty(const Lexer *lexer) { return lexer->file.src.count == 0; }

char lexer_peek(const Lexer *lexer, size_t offset) {
    if (lexer->file.src.count <= offset) {
        log_message(LL_ERROR, "Tried to access string outside of the valid range");
        return 0;
    }
    return lexer->file.src.items[offset];
}

char lexer_consume(Lexer *lexer) {
    if (lexer->file.src.count == 0) return 0;
    char c = lexer->file.src.items[0];
    lexer->file.src.items++;
    lexer->file.src.count--;
    return c;
}

void lexer_skip_ws(Lexer *lexer) {
    while (!lexer_is_empty(lexer) && isspace((lexer_peek(lexer, 0)))) lexer_consume(lexer);
}

void report_error(const char *begin, const char *end, const char *src, const char *name) {
    int row = 1, col = 1; // x, y;
    ptrdiff_t offset = end - begin;
    for (int i = 0; i < offset; i++) {
        if (src[i] == '\n') {
            col++;
            row = 1;
        } else {
            row++;
        }
    }
    log_diagnostic(LL_ERROR, "%s:%d:%d", name, col, row);
}
