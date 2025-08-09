#include "lexer.h"
#include "log.h"
#include "sv.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char *program_name);

/*
 * Argument `file_name`: input file name (null-terminated C-str)
 * Argument `s`: output parameter which will be filled out
 * Return: indicates failure (where false is failed read operation)
 */
bool read_file(const char *file_name, String *s);

int main(int argc, char **argv) {
    // TODO: Unhardcode the argument parsing
    char *program_name = argv[0];
    if (argc < 2) {
        usage(program_name);
        return 1;
    }
    char *input_name = argv[1];

    String s = {0};
    if (!read_file(input_name, &s)) return 1;
    Lexer l = {.src = SV(s), .begin_of_src = s.items};
    Tokens tokens = {0};

    if (!lexer_run(&l, &tokens)) {
        log_diagnostic(LL_ERROR, "Failed to lex source code");
        return 1;
    }
}

static void usage(char *program_name) {
    log_message(LL_INFO, "Usage: ");
    log_message(LL_INFO, "  %s <input.boa>", program_name);
}

bool read_file(const char *file_name, String *s) {
    if (file_name == NULL) return false;
    memset(s, 0, sizeof(String));
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        log_message(LL_ERROR, "Failed to open file %s: %s", file_name, strerror(errno));
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    s->count = size;
    s->capacity = size;
    s->items = malloc(sizeof(char) * size + 1);
    fseek(file, 0, SEEK_SET);
    size_t read = fread(s->items, sizeof(char), size, file);
    if (read != size) {
        log_message(LL_ERROR, "Failed to read whole file %s");
        return false;
    }

    return true;
}
