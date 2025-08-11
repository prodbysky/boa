#include "../src/frontend/lexer.h"
#include "../src/frontend/parser.h"
#include "../src/util.h"

int main() {
    char *src = "1 + 1 * 3";
    Lexer l = {.begin_of_src = src, .file = {.name = "CONST", .src = SV_FROM_CSTR(src)}};
    Tokens ts = {0};
    ASSERT(lexer_run(&l, &ts), "The source code should be lexible without any errors");

    Arena arena = arena_new(1024);
    Parser p = {
        .arena = &arena,
        .last_token = {0},
        .tokens = {.items = ts.items, .count = ts.count},
        .origin =
            {
                .src = SV_FROM_CSTR(src),
                .name = "CONST",
            },
    };
    AstExpression expr = {0};
    ASSERT(parser_parse_expr(&p, &expr), "The source code should be parsible without any errors");

    if (expr.begin != src) return 1;
    if (expr.len != 9) return 1;
    if (expr.type != AET_BINARY) return 1;
    if (expr.bin.op != OT_PLUS) return 1;

    if (expr.bin.r->bin.op != OT_MULT) return 1;
    if (expr.bin.r->bin.l->number != 1) return 1;
    if (expr.bin.r->bin.r->number != 3) return 1;


    return 0;
}
