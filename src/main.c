#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/codegen/nasm_x86_64_linux.h"
#include "backend/ir/ssa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char *program_name);

typedef struct {
    char *exe_name;
    char *input_name;
    char *output_name;
    bool should_free_output_name;
    bool keep_build_artifacts;
    bool dont_optimize;
} Config;

bool parse_config(Config *conf, int argc, char **argv);

int main(int argc, char **argv) {
    int result = 0;
    Config c = {0};

    char *asm_path_c = NULL;
    char *o_path_c = NULL;

    if (!parse_config(&c, argc, argv)) {
        usage(c.exe_name);
        result = 1;
        goto defer;
    }

    Path asm_path = path_from_cstr(c.output_name);
    path_add_ext(&asm_path, "asm");

    Path o_path = path_from_cstr(c.output_name);
    path_add_ext(&o_path, "o");

    asm_path_c = path_to_cstr(&asm_path);
    o_path_c = path_to_cstr(&o_path);

    free(asm_path.path.items);
    free(o_path.path.items);

    SourceFile file = {0};
    if (!read_source_file(c.input_name, &file)) {
        result = 1;
        goto defer;
    }
    Lexer l = {.begin_of_src = file.src.items, .file = FILE_VIEW_FROM_FILE(file)};
    Tokens tokens = {0};

    if (!lexer_run(&l, &tokens)) {
        log_diagnostic(LL_ERROR, "Failed to lex source code");
        result = 1;
        goto defer;
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
    if (!parser_parse(&p, &tree)) {
        result = 1;
        goto defer;
    }

    SSAModule mod = {0};
    if (!generate_ssa_module(&tree, &mod)) {
        result = 1;
        goto defer;
    }
    if (!c.dont_optimize) {
        if (!optimize_ssa_ir(&mod)) {
            result = 1;
            goto defer;
        }
    }

    FILE *asm_file = fopen(asm_path_c, "wb");
    if (!nasm_x86_64_linux_generate_file(asm_file, &mod)) {
        result = 1;
        goto defer;
    }
    fclose(asm_file);

    if (run_program("nasm", (char *[]){"nasm", asm_path_c, "-f", "elf64", "-o", o_path_c, NULL}) != 0) {
        log_diagnostic(LL_ERROR, "nasm failed");
        result = 1;
        goto defer;
    }
    if (run_program("ld", (char *[]){"ld", o_path_c, "-o", c.output_name, NULL}) != 0) {
        if (!c.keep_build_artifacts) { run_program("rm", (char *[]){"rm", o_path_c, asm_path_c, NULL}); }
        log_diagnostic(LL_ERROR, "ld failed");
        result = 1;
        goto defer;
    }
    if (!c.keep_build_artifacts) { run_program("rm", (char *[]){"rm", o_path_c, asm_path_c, NULL}); }
defer:
    if (tree.items != NULL) free(tree.items);
    if (mod.functions.items != NULL) {
        for (size_t i = 0; i < mod.functions.count; i++) {
            if (mod.functions.items[i].body.items != NULL) free(mod.functions.items[i].body.items);
            if (mod.functions.items[i].variables.items != NULL) free(mod.functions.items[i].variables.items);
        }
        free(mod.functions.items);
    }
    arena_free(&arena);
    if (tokens.items != NULL) free(tokens.items);
    if (file.src.items != NULL) free(file.src.items);
    free(asm_path_c);
    free(o_path_c);
    if (c.should_free_output_name) free(c.output_name);
    return result;
}

bool parse_config(Config *conf, int argc, char **argv) {
    conf->exe_name = *argv;
    argc--;
    argv++;

    while (argc > 0) {
        if (strcmp(*argv, "-o") == 0) {
            if (conf->output_name != NULL) {
                log_diagnostic(LL_ERROR, "Found a duplicate output file name flag");
                return false;
            }
            argc--;
            argv++;
            if (argc >= 0) {
                log_diagnostic(LL_ERROR, "Expected a file name to be here to set a custom output file");
                return false;
            }
            conf->output_name = *argv;
            argc--;
            argv++;
        } else if (strcmp(*argv, "-keep-artifacts") == 0) {
            conf->keep_build_artifacts = true;
            argc--;
            argv++;
        } else if (strcmp(*argv, "-help") == 0) {
            usage(conf->exe_name);
            exit(0);
        } else if (strcmp(*argv, "-no-opt") == 0) {
            conf->dont_optimize = true;
        } else {
            if (**argv == '-') {
                log_diagnostic(LL_ERROR, "Unknown flag supplied");
                return false;
            }
            if (conf->input_name != NULL) {
                log_diagnostic(LL_ERROR, "Compiling multiple files at the same time is not supported");
                return false;
            }
            conf->input_name = *argv;
            argc--;
            argv++;
        }
    }

    if (conf->input_name == NULL) {
        log_diagnostic(LL_ERROR, "No input name is provided");
        return false;
    }

    if (conf->output_name == NULL) {
        size_t len = strlen(conf->input_name) - 3;
        conf->output_name = malloc(sizeof(char) * (len + 1));
        conf->output_name[len] = 0;
        conf->should_free_output_name = true;
        snprintf(conf->output_name, len, "%s", conf->input_name);
    }

    return true;
}

static void usage(char *program_name) {
    log_diagnostic(LL_INFO, "Usage: ");
    log_diagnostic(LL_INFO, "  %s <input.boa> [FLAGS]", program_name);
    log_diagnostic(LL_INFO, "  [FLAGS]:");
    log_diagnostic(LL_INFO, "    -help          : Show this help message");
    log_diagnostic(LL_INFO, "    -o             : Customize the output file name (format: -o <name>)");
    log_diagnostic(LL_INFO, "    -keep-artifacts: Keep the build artifacts (.asm, .o files)");
    log_diagnostic(LL_INFO, "    -no-opt        : Dont optimize the code");
}
