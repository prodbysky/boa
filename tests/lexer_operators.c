#include "../src/frontend/lexer.h"
#include "../src/util.h"
#include <string.h>

int main() {
    char *src = "+ -/* != == >= <= < >";
    Arena arena = arena_new(1024);
    Lexer l = {.begin_of_src = src, .file = {.name = "CONST", .src = SV_FROM_CSTR(src)}, .arena = &arena};
    Tokens out = {0};
    ASSERT(lexer_run(&l, &out), "The source code should be lexible without any errors");


    Token expected[] = {
        (Token){.type = TT_OPERATOR, .begin = src, .len = 1, .operator = OT_PLUS},
        (Token){.type = TT_OPERATOR, .begin = src + 2, .len = 1, .operator = OT_MINUS},
        (Token){.type = TT_OPERATOR, .begin = src + 3, .len = 1, .operator = OT_DIV},
        (Token){.type = TT_OPERATOR, .begin = src + 4, .len = 1, .operator = OT_MULT},
        (Token){.type = TT_OPERATOR, .begin = src + 6, .len = 2, .operator = OT_NEQ},
        (Token){.type = TT_OPERATOR, .begin = src + 9, .len = 2, .operator = OT_EQ},
        (Token){.type = TT_OPERATOR, .begin = src + 12, .len = 2, .operator = OT_MTE},
        (Token){.type = TT_OPERATOR, .begin = src + 15, .len = 2, .operator = OT_LTE},
        (Token){.type = TT_OPERATOR, .begin = src + 18, .len = 1, .operator = OT_LT},
        (Token){.type = TT_OPERATOR, .begin = src + 20, .len = 1, .operator = OT_MT},
    };

    if (out.count != 10) return 1;


    for (size_t i = 0; i < out.count; i++) {
        if (memcmp(&expected[i], &out.items[i], sizeof(Token)) != 0) return 1;
    }
}
