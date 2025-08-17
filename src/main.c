#include "arena.h"
#include "log.h"
#include "sv.h"
#include "util.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "backend/codegen/nasm_x86_64_linux.h"
#include "backend/ir/ssa.h"
#include "config.h"
#include "target.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __unix__
const TargetKind default_target = TK_Linux_x86_64_NASM;
#elifdef _WIN32
const TargetKind default_target = TK_Win_x86_64_MINGW;
#endif

int main(int argc, char **argv) {
    int result = 0;
    Config c = {0};

    if (!parse_config(&c, argc, argv)) {
        usage(c.exe_name);
        result = 1;
        goto defer;
    }

    Target *t = NULL;
    if (c.target == NULL) {
        const char *target_name = target_enum_to_str(default_target);
        t = find_target(target_name);
    } else {
        t = find_target(c.target);
    }

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

    t->generate(c.output_name, &mod);
    t->assemble(c.output_name);
    t->link(c.output_name);
    if (!c.keep_build_artifacts) t->cleanup(c.output_name);

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
    if (c.should_free_output_name) free(c.output_name);
    return result;
}
