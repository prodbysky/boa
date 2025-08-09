#include "lexer.h"
#include "log.h"
#include "util.h"
#include <ctype.h>
#include <stdlib.h>

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

        switch (lexer_peek(lexer, 0)) {
            case 0: UNREACHABLE("The lexer can't be empty in here");
            case '+': case '-': case '*': case '/': {
                Token t = {0};
                t.type = TT_OPERATOR;
                t.len = 1;
                t.begin = lexer->src.items;
                t.operator = char_to_op[(size_t)lexer_peek(lexer, 0)];
                da_push(out, t);
                lexer_consume(lexer);
                continue;
            }
        }

        TODO();
    }

    return true;
}

bool lexer_lex_number(Lexer *lexer, Token *out) {
    ASSERT(!lexer_is_empty(lexer), "The callee ensures this condition");
    ASSERT(isdigit(lexer_peek(lexer, 0)), "The callee ensures this condition");
    const char *begin = lexer->src.items;
    while (!lexer_is_empty(lexer) && isdigit(lexer_peek(lexer, 0))) lexer_consume(lexer);
    const char *end = lexer->src.items;
    if (isalpha(*end)) {
        log_diagnostic(LL_ERROR, "You can't have numeric literal next to "
                                 "any alphabetic characters");
        report_error(begin, end, lexer->begin_of_src, lexer->input_name);
        return false;
    }
    out->type = TT_NUMBER;
    out->begin = begin;
    out->len = end - begin;
    char* got_end = NULL;
    out->number = strtoull(begin, &got_end, 10);
    ASSERT(got_end == end, "LEXER BUG");
    return true;
}

bool lexer_is_empty(const Lexer *lexer) { return lexer->src.count == 0; }

char lexer_peek(const Lexer *lexer, size_t offset) {
    if (lexer->src.count <= offset) {
        log_diagnostic(LL_ERROR, "Tried to access string outside of the valid range");
        return 0;
    }
    return lexer->src.items[offset];
}

char lexer_consume(Lexer *lexer) {
    if (lexer->src.count == 0) return 0;
    char c = lexer->src.items[0];
    lexer->src.items++;
    lexer->src.count--;
    return c;
}

void lexer_skip_ws(Lexer *lexer) {
    while (!lexer_is_empty(lexer) && isspace((lexer_peek(lexer, 0)))) lexer_consume(lexer);
}

void report_error(const char *begin, const char *end, const char *src,
                  const char *name) {
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
