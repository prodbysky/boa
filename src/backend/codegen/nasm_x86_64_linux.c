#include "nasm_x86_64_linux.h"
#include "../../util.h"


static bool generate_function(FILE* sink, const IRFunction* func);

bool nasm_x86_64_linux_generate_file(FILE* sink, const IRModule* mod) {
    ASSERT(mod->functions.count == 1, "expect only one function for now");
    ASSERT(strcmp(mod->functions.items[0].name, "main") == 0, "expect only a main function");
    // prelude of some sorts
    fprintf(sink, "section .text\n");
    fprintf(sink, "global _start\n");
    fprintf(sink, "_start:\n");
    fprintf(sink, "  call main\n");
    fprintf(sink, "  mov rax, 60\n");
    fprintf(sink, "  mov rdi, 0\n");
    fprintf(sink, "  syscall\n");

    generate_function(sink, &mod->functions.items[0]);

    return true;
}

static bool generate_function(FILE* sink, const IRFunction* func) {
    fprintf(sink, "%s:\n", func->name);
    if (func->max_temps != 0) {
        fprintf(sink, "  enter %ld, 0\n", func->max_temps * 8);
    }

    fprintf(sink, "ret:\n");
    fprintf(sink, "  leave\n");
    fprintf(sink, "  ret\n");

    return true;
}

