#include "nasm_x86_64_linux.h"
#include "../../util.h"
#include <stdio.h>

static bool generate_function(FILE *sink, const IRFunction *func);
static bool generate_statement(FILE *sink, const IRStatement *st);

static void emit_return_some(FILE *sink, const IRStatement *ret);
static void emit_return_none(FILE *sink, const IRStatement *ret_none);

static void emit_add(FILE *sink, const IRStatement *st);
static void emit_sub(FILE *sink, const IRStatement *st);
static void emit_imul(FILE *sink, const IRStatement *st);
static void emit_div(FILE *sink, const IRStatement *st);
static void emit_assign(FILE *sink, const IRStatement *st);

static void emit_add_reg_value(FILE *sink, const char *reg, const IRValue *value);
static void emit_sub_reg_value(FILE *sink, const char *reg, const IRValue *value);
static void emit_imul_reg_value(FILE *sink, const char *reg, const IRValue *value);
static void move_value_into_register(FILE *sink, const char *reg, const IRValue *value);
static void move_value_into_value(FILE *sink, const IRValue *from, const IRValue *into);

static void value_asm_repr(FILE *sink, const IRValue *value);

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
        emit_return_some(sink, st);
        return true;
    }
    case IRST_RETURN_EMPTY: {
        emit_return_none(sink, st);
        return true;
    }
    case IRST_ADD: {
        emit_add(sink, st);
        return true;
    }
    case IRST_SUB: {
        emit_sub(sink, st);
        return true;
    }
    case IRST_MUL: {
        emit_imul(sink, st);
        return true;
    }
    case IRST_DIV: {
        emit_div(sink, st);
        return true;
    }
    case IRST_ASSIGN: {
        emit_assign(sink, st);
        return true;
    }
    default: TODO();
    }
}

static void emit_return_some(FILE *sink, const IRStatement *ret) {
    ASSERT(ret->type == IRST_RETURN,
           "This function should only be called when the type of the statement is IRST_RETURN");
    move_value_into_register(sink, "rax", &ret->ret.value);
    fprintf(sink, "  jmp ret\n");
}

static void emit_return_none(FILE *sink, const IRStatement *ret_none) {
    ASSERT(ret_none->type == IRST_RETURN_EMPTY,
           "This function should only be called when the type of the statement is IRST_RETURN_EMPTY");
    fprintf(sink, "  jmp ret\n");
}

static void emit_add(FILE *sink, const IRStatement *st) {
    ASSERT(st->type == IRST_ADD, "This function should only be called when the type of the statement is IRST_ADD");
    ASSERT(st->binop.result.type == IRVT_TEMP, "we can't add to a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_add_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_sub(FILE *sink, const IRStatement *st) {
    ASSERT(st->type == IRST_SUB, "This function should only be called when the type of the statement is IRST_SUB");
    ASSERT(st->binop.result.type == IRVT_TEMP, "we can't sub a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_sub_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_imul(FILE *sink, const IRStatement *st) {
    ASSERT(st->type == IRST_MUL, "This function should only be called when the type of the statement is IRST_MUL");
    ASSERT(st->binop.result.type == IRVT_TEMP, "we can't mul a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_imul_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_div(FILE *sink, const IRStatement *st) {
    // ugh x86_64 is so weird
    // rax low bits
    // rdi high bits
    ASSERT(st->type == IRST_DIV, "This function should only be called when the type of the statement is IRST_DIV");
    ASSERT(st->binop.result.type == IRVT_TEMP, "we can't div a constant");
    move_value_into_register(sink, "rax", &st->binop.l);
    fprintf(sink, "  xor rdx, rdx\n");
    move_value_into_register(sink, "rcx", &st->binop.r);
    fprintf(sink, "  div rcx\n");
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_assign(FILE *sink, const IRStatement *st) {
    ASSERT(st->type == IRST_ASSIGN,
           "This function should only be called when the type of the statement is IRST_ASSIGN");

    IRValue temp_value = {.temp = st->assign.place, .type = IRVT_TEMP};
    move_value_into_value(sink, &st->assign.value, &temp_value);
}

static void move_value_into_value(FILE *sink, const IRValue *from, const IRValue *into) {
    // Load source into a register
    move_value_into_register(sink, "rax", from);

    // Store register into destination
    fprintf(sink, "  mov ");
    value_asm_repr(sink, into);
    fprintf(sink, ", rax\n");}

static void emit_add_reg_value(FILE *sink, const char *reg, const IRValue *value) {
    fprintf(sink, "  add %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void emit_sub_reg_value(FILE *sink, const char *reg, const IRValue *value) {
    fprintf(sink, "  sub %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void emit_imul_reg_value(FILE *sink, const char *reg, const IRValue *value) {
    fprintf(sink, "  imul %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void move_value_into_register(FILE *sink, const char *reg, const IRValue *value) {
    fprintf(sink, "  mov %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void value_asm_repr(FILE *sink, const IRValue *value) {
    switch (value->type) {
    case IRVT_CONST: {
        fprintf(sink, "%ld", value->constant);
        break;
    }
    case IRVT_TEMP: {
        fprintf(sink, "qword [rbp - %ld]", (value->temp + 1) * 8);
        break;
    }
    }
}
