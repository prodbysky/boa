#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/ir.h"
#include "backend/codegen/nasm_x86_64_linux.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void usage(char *program_name);

/*
 * Argument `file_name`: input file name (null-terminated C-str)
 * Argument `s`: output parameter which will be filled out
 * Return: indicates failure (where false is failed read operation)
 */
bool read_file(const char *file_name, String *s);
bool read_source_file(const char *file_name, SourceFile *out);

int run_program(const char *prog, char *args[]);

int main(int argc, char **argv) {
    // TODO: Unhardcode the argument parsing
    char *program_name = argv[0];
    if (argc < 3) {
        usage(program_name);
        return 1;
    }
    char *input_name = argv[1];
    char *output_name = argv[2];

    SourceFile file = {0};
    if (!read_source_file(input_name, &file)) return 1;
    Lexer l = {.begin_of_src = file.src.items, .file = FILE_VIEW_FROM_FILE(file)};
    Tokens tokens = {0};

    if (!lexer_run(&l, &tokens)) {
        log_diagnostic(LL_ERROR, "Failed to lex source code");
        return 1;
    }

    Arena arena = arena_new(1024);

    Parser p = {
        .arena = &arena,
        .origin = l.file,
        .last_token = {0},
        .tokens =
            {
                .count = tokens.count,
                .items = tokens.items,
            },
    };
    AstTree tree = {0};
    if (!parser_parse(&p, &tree)) return 1;

    IRModule mod = {0};
    if (!generate_ir_module(&tree, &mod)) return 1;

    // TODO: Do something to this mess
    size_t output_name_len = strlen(output_name);
    size_t output_asm_name_len = output_name_len + 4;
    char* out_asm_name = malloc((output_asm_name_len + 1) * sizeof(char));
    out_asm_name[output_asm_name_len] = 0;
    sprintf(out_asm_name, "%s.asm", output_name);

    size_t output_o_name_len = output_name_len + 4;
    char* out_o_name = malloc((output_o_name_len + 1) * sizeof(char));
    out_o_name[output_o_name_len] = 0;
    sprintf(out_o_name, "%s.o", output_name);

    FILE* asm_file = fopen(out_asm_name, "wb");
    nasm_x86_64_linux_generate_file(asm_file, &mod);
    fclose(asm_file);

    run_program("nasm", (char*[]){"nasm", out_asm_name, "-f", "elf64", "-o", out_o_name, NULL});
    run_program("ld", (char*[]){"ld", out_o_name, "-o", output_name, NULL});
}

int run_program(const char *prog, char *args[]) {
    pid_t pi = fork();

    if (pi == -1) {
        log_message(LL_ERROR, "Failed to fork :(");
        return -1;
    }
    if (pi == 0) {
        if (execvp(prog, args) == -1) {
            log_message(LL_ERROR, "Failed to execute %s: %s", prog, strerror(errno));
            exit(-1);
        }
    } else {
        for (;;) {
            int status = 0;
            if (waitpid(pi, &status, 0) == -1) {
                log_message(LL_ERROR, "Failed to wait on %d: %s", pi, strerror(errno));
                return -1;
            }
            if (WIFEXITED(status)) { return WEXITSTATUS(status); }
        }
    }
    return 0;
}

static void usage(char *program_name) {
    log_message(LL_INFO, "Usage: ");
    log_message(LL_INFO, "  %s <input.boa> <output-name>", program_name);
}
bool read_source_file(const char *file_name, SourceFile *out) {
    ASSERT(file_name, "Passed in NULL for the file name");
    ASSERT(out, "Passed in NULL for the out source file parameter");

    if (!read_file(file_name, &out->src)) return false;
    out->name = file_name;
    return true;
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
