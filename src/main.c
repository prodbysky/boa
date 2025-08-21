#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/ir/ssa.h"
#include "config.h"
#include "target.h"
#include <stdlib.h>

#ifdef __unix__
const TargetKind default_target = TK_Linux_x86_64_NASM;
#elif defined(_WIN32)
const TargetKind default_target = TK_Win_x86_64_MINGW;
#endif

int main(int argc, char **argv) {
    int result = 0;
    Arena arena = arena_new(1024 * 1024 * 10);
    Config c = {0};

    if (!parse_config(&c, argc, argv, &arena)) {
        usage(c.exe_name);
        result = 1;
        goto defer;
    }

    Target *t = NULL;
    const char *target_name = (c.target == NULL) ? target_enum_to_str(default_target) : c.target;
    if (!find_target(&t, target_name)) {
        log_diagnostic(LL_ERROR, "Unknown target %s", c.target ? c.target : "(default)");
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
        .origin = l.file,
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

    t->generate(c.output_name, &mod, &arena);
    t->assemble(c.output_name, &arena);
    t->link(c.output_name, &arena);
    if (!c.keep_build_artifacts) t->cleanup(c.output_name, &arena);

defer:
    arena_free(&arena);
    return result;
}
