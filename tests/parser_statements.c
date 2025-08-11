#include "../src/frontend/lexer.h"
#include "../src/frontend/parser.h"
#include "../src/util.h"

int main() {
    char *src = "return; return 123;";
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
    {
        AstStatement return_no_value = {0};
        ASSERT(parser_parse_statement(&p, &return_no_value), "The source code should be parsible without any errors");

        if (return_no_value.begin != src) return 1;
        if (return_no_value.len != 7) return 1;
        if (return_no_value.type != AST_RETURN) return 1;
        if (return_no_value.ret.has_expr) return 1;
    }

    {
        AstStatement return_value = {0};
        ASSERT(parser_parse_statement(&p, &return_value), "The source code should be parsible without any errors");

        if (return_value.begin != src + 8) return 1;
        if (return_value.len != 11) return 1;
        if (return_value.type != AST_RETURN) return 1;
        if (!return_value.ret.has_expr) return 1;
        if (return_value.ret.return_expr.type != AET_PRIMARY) return 1;
    }

    return 0;
}
