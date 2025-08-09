#include "../src/lexer.h"
#include "../src/util.h"
#include <string.h>

int main() {
    char *src = "+ -/*";
    Lexer l = {.begin_of_src = src, .input_name = "CONST", .src = SV_FROM_CSTR(src)};
    Tokens out = {0};
    ASSERT(lexer_run(&l, &out), "The source code should be lexible without any errors");


    Token expected[] = {
        (Token){.type = TT_OPERATOR, .begin = src, .len = 1, .operator = OT_PLUS},
        (Token){.type = TT_OPERATOR, .begin = src + 2, .len = 1, .operator = OT_MINUS},
        (Token){.type = TT_OPERATOR, .begin = src + 3, .len = 1, .operator = OT_DIV},
        (Token){.type = TT_OPERATOR, .begin = src + 4, .len = 1, .operator = OT_MULT},
    };

    if (out.count < 4) return 1;

    for (size_t i = 0; i < out.count; i++) {
        if (memcmp(&expected[i], &out.items[i], sizeof(Token)) != 0) return 1;
    }
}
