#include "lexer.h"
#include "log.h"

bool lexer_is_empty(const Lexer* lexer) {
    return lexer->src.count == 0;
}

char lexer_peek(const Lexer* lexer, size_t offset) {
    if (lexer->src.count <= offset) {
        log_message(LL_ERROR, "Tried to access string outside of the valid range");
        return 0;
    }
    return lexer->src.items[offset];
}

char lexer_consume(Lexer* lexer) {
    if (lexer->src.count == 0) return 0;
    char c = lexer->src.items[0];
    lexer->src.items++;
    lexer->src.count--;
    return c;
}
