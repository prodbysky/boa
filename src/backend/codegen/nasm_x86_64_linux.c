#include "nasm_x86_64_linux.h"
#include "../../util.h"
bool nasm_x86_64_linux_generate_file(FILE* sink, const IRModule* mod) {
    ASSERT(mod->functions.count == 1, "expect only one function for now");
    // prelude of some sorts
    fprintf(sink, "section .text\n");
    fprintf(sink, "global _start\n");
    fprintf(sink, "_start:\n");
    fprintf(sink, "  mov rax, 60\n");
    fprintf(sink, "  mov rdi, 0\n");
    fprintf(sink, "  syscall\n");
    return true;
}
