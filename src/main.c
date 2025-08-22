#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/ir/ssa.h"
#include "config.h"
#include "target.h"


int main(int argc, char **argv) {
    int result = 0;
    Arena arena = arena_new(1024 * 1024);
    Config c = {0};

    if (!parse_config(&c, argc, argv, &arena)) {
        usage(c.exe_name);
        result = 1;
        goto defer;
    }

    SourceFile file = {0};
    if (!read_source_file(c.input_name, &file, &arena)) {
        result = 1;
        goto defer;
    }
    Lexer l = {.begin_of_src = file.src.items, .file = FILE_VIEW_FROM_FILE(file), .arena = &arena};
    Tokens tokens = {0};

    if (!lexer_run(&l, &tokens)) {
        log_diagnostic(LL_ERROR, "Failed to lex source code");
        result = 1;
        goto defer;
    }

    Parser p = {
        .arena = &arena,
        .origin = FILE_VIEW_FROM_FILE(file),
        .last_token = {0},
        .tokens =
            {
                .count = tokens.count,
                .items = tokens.items,
            },
    };
    AstRoot root = {0};
    if (!parser_parse(&p, &root)) {
        result = 1;
        goto defer;
    }

    SSAModule mod = {0};
    if (!generate_ssa_module(&root, &mod, &arena)) {
        result = 1;
        goto defer;
    }

    c.target->generate(c.output_name, &mod, &arena);
    c.target->assemble(c.output_name, &arena);
    c.target->link(c.output_name, &arena);
    if (!c.keep_build_artifacts) c.target->cleanup(c.output_name, &arena);

defer:
    arena_free(&arena);
    return result;
}
