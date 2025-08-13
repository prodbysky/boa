#include "nasm_x86_64_linux.h"
#include "../../util.h"
#include <stdio.h>

static bool generate_function(FILE *sink, const IRFunction *func);
static bool generate_statement(FILE *sink, const IRStatement *st);

bool nasm_x86_64_linux_generate_file(FILE *sink, const IRModule *mod) {
    ASSERT(mod->functions.count == 1, "expect only one function for now");
    ASSERT(strcmp(mod->functions.items[0].name, "main") == 0, "expect only a main function");
    // prelude of some sorts
    fprintf(sink, "section .text\n");
    fprintf(sink, "global _start\n");
    fprintf(sink, "_start:\n");
    fprintf(sink, "  call main\n");
    fprintf(sink, "  mov rsi, rax\n");
    fprintf(sink, "  mov rax, 60\n");
    fprintf(sink, "  mov rdi, rsi\n");
    fprintf(sink, "  syscall\n");

    generate_function(sink, &mod->functions.items[0]);

    return true;
}

static bool generate_function(FILE *sink, const IRFunction *func) {
    fprintf(sink, "%s:\n", func->name);
    fprintf(sink, "  enter %ld, 0\n", func->max_temps * 8);

    for (size_t i = 0; i < func->body.count; i++) { generate_statement(sink, &func->body.items[i]); }

    fprintf(sink, "ret:\n");
    fprintf(sink, "  leave\n");
    fprintf(sink, "  ret\n");

    return true;
}

static bool generate_statement(FILE *sink, const IRStatement *st) {
    switch (st->type) {
    case IRST_RETURN: {
        switch (st->ret.value.type) {
        case IRVT_CONST: {
            fprintf(sink, "  mov %s, %ld\n", "rax", st->ret.value.constant);
            break;
        }
        case IRVT_TEMP: {
            fprintf(sink, "  mov %s, [rbp - %ld]\n", "rax", (st->ret.value.temp + 1) * 8);
            break;
        }
        }
        fprintf(sink, "  jmp ret\n");
        return true;
    }
    case IRST_ADD: {
        ASSERT(st->binop.result.type == IRVT_TEMP, "we can't add to a constant");
        switch (st->binop.l.type) {
        case IRVT_CONST: {
            fprintf(sink, "  mov %s, %ld\n", "rax", st->binop.l.constant);
            break;
        }
        case IRVT_TEMP: {
            fprintf(sink, "  mov %s, [rbp - %ld]\n", "rax", (st->binop.l.temp + 1) * 8);
            break;
        }
        }

        fprintf(sink, "  add rax, ");
        switch (st->binop.r.type) {
        case IRVT_CONST: {
            fprintf(sink, "%ld\n", st->binop.r.constant);
            break;
        }
        case IRVT_TEMP: {
            fprintf(sink, "QWORD PTR [rbp - %ld]\n", (st->binop.r.temp + 1) * 8);
            break;
        }
        }
        fprintf(sink, "  mov [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
        return true;
    }
    default: TODO();
    }
}

