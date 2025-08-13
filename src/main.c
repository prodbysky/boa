#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/codegen/nasm_x86_64_linux.h"
#include "backend/ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char *program_name);

typedef struct {
    char *exe_name;
    char *input_name;
    char *output_name;
    bool keep_build_artifacts;
} Config;

bool parse_config(Config *conf, int argc, char **argv);

/**
 * Program entry point and single-file compiler driver.
 *
 * Parses command-line arguments, reads and lexes the input source, parses
 * to an AST, lowers to an IR module, emits NASM x86-64 assembly, assembles
 * with `nasm`, links with `ld` to produce the final executable, and
 * optionally removes intermediate artifacts.
 *
 * Side effects:
 *  - Reads the input file specified on the command line.
 *  - Writes "<output_name>.asm" and "<output_name>.o" and the final executable.
 *  - Invokes external programs: "nasm", "ld" and "rm" (for cleanup).
 *  - Logs diagnostics on failure.
 *
 * Return:
 *  - 0 on success.
 *  - 1 on any error (argument parsing, file I/O, lexing, parsing, IR gen,
 *    assembly or linking).
 */
int main(int argc, char **argv) {
    Config c = {0};
    if (!parse_config(&c, argc, argv)) {
        usage(c.exe_name);
        return 1;
    }

    SourceFile file = {0};
    if (!read_source_file(c.input_name, &file)) return 1;
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
    size_t output_name_len = strlen(c.output_name);
    size_t output_asm_name_len = output_name_len + 4;
    char *out_asm_name = malloc((output_asm_name_len + 1) * sizeof(char));
    out_asm_name[output_asm_name_len] = 0;
    sprintf(out_asm_name, "%s.asm", c.output_name);

    size_t output_o_name_len = output_name_len + 4;
    char *out_o_name = malloc((output_o_name_len + 1) * sizeof(char));
    out_o_name[output_o_name_len] = 0;
    sprintf(out_o_name, "%s.o", c.output_name);

    FILE *asm_file = fopen(out_asm_name, "wb");
    nasm_x86_64_linux_generate_file(asm_file, &mod);
    fclose(asm_file);

    if (run_program("nasm", (char *[]){"nasm", out_asm_name, "-f", "elf64", "-o", out_o_name, NULL}) != 0) {
        log_diagnostic(LL_ERROR, "nasm failed");
        return 1;
    }
    if (run_program("ld", (char *[]){"ld", out_o_name, "-o", c.output_name, NULL}) != 0) {
        if (c.keep_build_artifacts) { run_program("rm", (char *[]){"rm", out_o_name, out_asm_name, NULL}); }
        log_diagnostic(LL_ERROR, "ld failed");
        return 1;
    }
    if (c.keep_build_artifacts) { run_program("rm", (char *[]){"rm", out_o_name, out_asm_name, NULL}); }
}

/**
 * Parse command-line arguments into a Config structure.
 *
 * Processes argv to populate conf->exe_name, conf->input_name, conf->output_name,
 * and conf->keep_build_artifacts. Recognized flags:
 *  - "-o <name>"           : set custom output base name
 *  - "-keep-artifacts"     : preserve intermediate build artifacts
 *  - "-help"               : print usage and exit(0)
 *
 * If -o is not provided, output_name is derived from the input file name by
 * truncating the final three characters (allocates a new string). On error
 * (missing input, duplicate flags, unknown flag, or malformed -o usage) a
 * diagnostic is logged and the function returns false.
 *
 * @param conf Pointer to the Config to fill (must be non-NULL). Fields may be
 *             modified or allocated by this function.
 * @param argc Argument count (as passed to main).
 * @param argv Argument vector (as passed to main).
 * @return true if arguments were parsed successfully and conf is populated;
 *         false on parse error (caller should handle diagnostics and exit).
 */
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
        } else if (strcmp(*argv, "-help")) {
            usage(conf->exe_name);
            exit(0);
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
        snprintf(conf->output_name, len, "%s", conf->input_name);
    }

    return true;
}

/**
 * Print program usage and available command-line flags to the diagnostics log.
 *
 * Prints a short synopsis that uses `program_name` (typically `argv[0]`) and
 * enumerates supported flags: `-help`, `-o <name>`, and `-keep-artifacts`.
 *
 * @param program_name Program invocation name used in the usage synopsis.
 */
static void usage(char *program_name) {
    log_diagnostic(LL_INFO, "Usage: ");
    log_diagnostic(LL_INFO, "  %s <input.boa> [FLAGS]", program_name);
    log_diagnostic(LL_INFO, "  [FLAGS]:");
    log_diagnostic(LL_INFO, "    -help : show this help message");
    log_diagnostic(LL_INFO, "    -o : Customize the output file name (format: -o <name>)");
    log_diagnostic(LL_INFO, "    -keep-artifacts : Keep the build artifacts (.asm, .o files)");
}
