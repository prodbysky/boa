#include "../src/frontend/lexer.h"
#include "../src/frontend/parser.h"
#include "../src/util.h"

int main() {
    char *src = "return; return 123; let number = 123; if a {} while b {} c();";
    Arena arena = arena_new(1024 * 1024);
    Lexer l = {.begin_of_src = src, .file = {.name = "CONST", .src = SV_FROM_CSTR(src)}, .arena = &arena};
    Tokens ts = {0};
    ASSERT(lexer_run(&l, &ts), "The source code should be lexible without any errors");

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
    {
        AstStatement let_statement = {0};
        ASSERT(parser_parse_statement(&p, &let_statement), "The source code should be parsible without any errors");

        if (let_statement.begin != src + 20) return 1;
        if (let_statement.len != 17) return 1;
        if (let_statement.type != AST_LET) return 1;
        if (strncmp((char *)let_statement.let.name.items, "number", 6) != 0) return 1;
    }
    {
        AstStatement if_statement = {0};
        ASSERT(parser_parse_statement(&p, &if_statement), "The source code should be parsible without any errors");

        if (if_statement.begin != src + 38) return 1;
        if (if_statement.len != 7) return 1;
        if (if_statement.type != AST_IF) return 1;
        if (strncmp((char *)if_statement.if_st.cond.ident.items, "a", 1) != 0) return 1;
    }
    {
        AstStatement while_statement = {0};
        ASSERT(parser_parse_statement(&p, &while_statement), "The source code should be parsible without any errors");

        if (while_statement.begin != src + 46) return 1;
        if (while_statement.len != 10) return 1;
        if (while_statement.type != AST_WHILE) return 1;
        if (strncmp((char *)while_statement.while_st.cond.ident.items, "b", 1) != 0) return 1;
    }
    {
        AstStatement call_statement = {0};
        ASSERT(parser_parse_statement(&p, &call_statement), "The source code should be parsible without any errors");

        if (call_statement.begin != src + 57) return 1;
        if (call_statement.len != 4) return 1;
        if (call_statement.type != AST_CALL) return 1;
        if (strncmp((char *)call_statement.call.name.items, "c", 1) != 0) return 1;
    }
    return 0;
}
