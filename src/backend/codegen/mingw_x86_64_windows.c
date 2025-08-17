#include "mingw_x86_64_windows.h"
#include "../../util.h"
#include <stdio.h>

bool mingw_x86_64_windows_generate_file(FILE* sink, const SSAModule* mod) {
    ASSERT(mod->functions.count == 1, "expect only one function for now");
    ASSERT(strcmp(mod->functions.items[0].name, "main") == 0, "expect only a main function");
    
    fprintf(sink, ".intel_syntax noprefix\n");
    fprintf(sink, ".extern ExitProcess\n");
    fprintf(sink, ".global _start\n");
    fprintf(sink, ".section .text\n");
    fprintf(sink, "_start:\n");
    fprintf(sink, "   mov rcx, 69\n");
    fprintf(sink, "   sub rsp, 8\n");
    fprintf(sink, "   call ExitProcess\n");
    return true;
}
