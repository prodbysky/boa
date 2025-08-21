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

static void lex_single_char(Lexer *lexer, Tokens *out, TokenType new) {
    Token t = {.len = 1, .begin = lexer->file.src.items, .type = new};
    da_push(out, t, lexer->arena);
    lexer_consume(lexer);
}

bool lexer_run(Lexer *lexer, Tokens *out) {
    ASSERT(lexer, "Passed null lexer");
    ASSERT(out, "Passed null tokens out array");
    while (!lexer_is_empty(lexer)) {
        lexer_skip_ws(lexer);
        if (lexer_is_empty(lexer)) return true;

        if (isdigit(lexer_peek(lexer, 0))) {
            Token t = {0};
            if (!lexer_lex_number(lexer, &t)) return false;
            da_push(out, t, lexer->arena);
            continue;
        }

        if (lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0))) {
            Token t = {0};
            if (!lexer_lex_ident_or_keyword(lexer, &t)) return false;
            da_push(out, t, lexer->arena);
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
            da_push(out, t, lexer->arena);
            lexer_consume(lexer);
            continue;
        }
        case ';': lex_single_char(lexer, out, TT_SEMICOLON); continue;
        case '=': lex_single_char(lexer, out, TT_ASSIGN); continue;
        case '{': lex_single_char(lexer, out, TT_OPEN_CURLY); continue;
        case '}': lex_single_char(lexer, out, TT_CLOSE_CURLY); continue;
        case '(': lex_single_char(lexer, out, TT_OPEN_PAREN); continue;
        case ')': lex_single_char(lexer, out, TT_CLOSE_PAREN); continue;
        case ',': lex_single_char(lexer, out, TT_COMMA); continue;
        case '"': lex_single_char(lexer, out, TT_DOUBLE_QUOTE); continue;
        default: {
            log_diagnostic(LL_INFO, "Don't know some letter skipping for sake of asm");
            lexer_consume(lexer);
            continue;
        }
        }
    }

    return true;
}

bool lexer_lex_ident_or_keyword(Lexer *lexer, Token *out) {
    ASSERT(!lexer_is_empty(lexer), "The caller ensures this condition");
    ASSERT(lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0)), "The caller ensures this condition");
    const char *begin = lexer->file.src.items;

    while (!lexer_is_empty(lexer) &&
           (lexer_peek(lexer, 0) == '_' || isalpha(lexer_peek(lexer, 0)) || isdigit(lexer_peek(lexer, 0))))
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

bool lexer_ident_is_keyword(const char *begin, ptrdiff_t len) { return lexer_keyword(begin, len) != KT_NO; }

KeywordType lexer_keyword(const char *begin, ptrdiff_t len) {
    if (len == 6 && strncmp(begin, "return", len) == 0) return KT_RETURN;
    if (len == 3 && strncmp(begin, "let", len) == 0) return KT_LET;
    if (len == 3 && strncmp(begin, "def", len) == 0) return KT_DEF;
    if (len == 2 && strncmp(begin, "if", len) == 0) return KT_IF;
    if (len == 5 && strncmp(begin, "while", len) == 0) return KT_WHILE;
    if (len == 7 && strncmp(begin, "__asm__", len) == 0) return KT_ASM;
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
