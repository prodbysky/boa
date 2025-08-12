#include "fasm_x86_64_linux.h"
#include "../../util.h"
#include <stdio.h>

static bool generate_function(FILE *out, const IRFunction *f);
static bool generate_statement(FILE *out, const IRStatement *st);

bool generate_fasm_x86_64_linux(FILE *out, const IRModule *mod) {
    ASSERT(out, "Sanity check");
    ASSERT(mod, "Sanity check");

    for (size_t i = 0; i < mod->functions.count; i++)
        if (!generate_function(out, &mod->functions.items[i])) return false;

    return true;
}

static bool generate_function(FILE *out, const IRFunction *f) {
    ASSERT(f, "Sanity check");

    fprintf(out, "format elf64 executable 3\n");
    fprintf(out, "segment readable executable\n");



    fprintf(out, "entry _start\n");
    fprintf(out, "_start:\n");
    fprintf(out, "    call main\n");
    fprintf(out, "    mov rsi, rax\n");
    fprintf(out, "    mov rax, 60\n");
    fprintf(out, "    mov rdi, rsi\n");
    fprintf(out, "    syscall\n");


    fprintf(out, "%s:\n", f->name);

    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    sub rsp, %lu\n", f->max_temps * 8);


    for (size_t i = 0; i < f->body.count; i++) {
        if (!generate_statement(out, &f->body.items[i])) return 1;
    }


    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");

    return true;
}

static bool generate_statement(FILE *out, const IRStatement *st) {
    switch (st->type) {
        case IRST_RETURN: {
            if (st->ret.value.type == IRVT_CONST) {
                fprintf(out, "mov rax, %ld\n", st->ret.value.constant);
            } else {
                fprintf(out, "mov rax, [rbp - %ld]\n", st->ret.value.temp * 8);
            }
            fprintf(out, "leave\n");
            fprintf(out, "ret\n");
            return true;
        }
        default: TODO();
    }
}
