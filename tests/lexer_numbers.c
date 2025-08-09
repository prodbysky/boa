#include "../src/lexer.h"
#include "../src/util.h"
#include <string.h>

int main() {
    char *src = "123 69\n 1";
    Lexer l = {.begin_of_src = src, .file = {.name = "CONST", .src = SV_FROM_CSTR(src)}};
    Tokens out = {0};
    ASSERT(lexer_run(&l, &out), "The source code should be lexible without any errors");


    Token expected[] = {
        (Token){.type = TT_NUMBER, .begin = src, .len = 3, .number = 123},
        (Token){.type = TT_NUMBER, .begin = src + 4, .len = 2, .number = 69},
        (Token){.type = TT_NUMBER, .begin = src + 8, .len = 1, .number = 1},
    };

    if (out.count < 3) return 1;

    for (size_t i = 0; i < out.count; i++) {
        if (memcmp(&expected[i], &out.items[i], sizeof(Token)) != 0) return 1;
    }
}
