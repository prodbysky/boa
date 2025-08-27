#include "../src/frontend/lexer.h"
#include "../src/util.h"
#include <string.h>

int main() {
    char *src = "def if while let return __asm__";
    Arena arena = arena_new(1024);
    Lexer l = {.begin_of_src = src, .file = {.name = "CONST", .src = SV_FROM_CSTR(src)}, .arena = &arena};
    Tokens out = {0};
    ASSERT(lexer_run(&l, &out), "The source code should be lexible without any errors");

    Token expected[] = {
        (Token){.type = TT_KEYWORD, .begin = src, .len = 3, .keyword = KT_DEF},
        (Token){.type = TT_KEYWORD, .begin = src + 4, .len = 2, .keyword = KT_IF},
        (Token){.type = TT_KEYWORD, .begin = src + 7, .len = 5, .keyword = KT_WHILE},
        (Token){.type = TT_KEYWORD, .begin = src + 13, .len = 3, .keyword = KT_LET},
        (Token){.type = TT_KEYWORD, .begin = src + 17, .len = 6, .keyword = KT_RETURN},
        (Token){.type = TT_KEYWORD, .begin = src + 24, .len = 7, .keyword = KT_ASM},
    };

    if (out.count < 6) return 1;

    for (size_t i = 0; i < out.count; i++) {
        if (memcmp(&expected[i], &out.items[i], sizeof(Token)) != 0) return 1;
    }
}
