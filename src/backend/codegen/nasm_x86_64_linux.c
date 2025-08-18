#include "nasm_x86_64_linux.h"
#include "../../util.h"
#include <stdio.h>

static bool generate_function(FILE *sink, const SSAFunction *func);
static bool generate_statement(FILE *sink, const SSAStatement *st);

static void emit_return_some(FILE *sink, const SSAStatement *ret);
static void emit_return_none(FILE *sink, const SSAStatement *ret_none);

static void emit_add(FILE *sink, const SSAStatement *st);
static void emit_sub(FILE *sink, const SSAStatement *st);
static void emit_imul(FILE *sink, const SSAStatement *st);
static void emit_div(FILE *sink, const SSAStatement *st);
static void emit_assign(FILE *sink, const SSAStatement *st);
static void emit_call(FILE *sink, const SSAStatement *st);

static void emit_add_reg_value(FILE *sink, const char *reg, const SSAValue *value);
static void emit_sub_reg_value(FILE *sink, const char *reg, const SSAValue *value);
static void emit_imul_reg_value(FILE *sink, const char *reg, const SSAValue *value);
static void move_value_into_register(FILE *sink, const char *reg, const SSAValue *value);
static void move_value_into_value(FILE *sink, const SSAValue *from, const SSAValue *into);

static void value_asm_repr(FILE *sink, const SSAValue *value);

static size_t f_count = 0;

bool nasm_x86_64_linux_generate_file(FILE *sink, const SSAModule *mod) {
    // prelude of some sorts
    fprintf(sink, "section .text\n");
    fprintf(sink, "global _start\n");
    fprintf(sink, "_start:\n");
    fprintf(sink, "  call main\n");
    fprintf(sink, "  mov rsi, rax\n");
    fprintf(sink, "  mov rax, 60\n");
    fprintf(sink, "  mov rdi, rsi\n");
    fprintf(sink, "  syscall\n");

    for (size_t i = 0; i < mod->functions.count; i++) { generate_function(sink, &mod->functions.items[i]); }

    return true;
}

static bool generate_function(FILE *sink, const SSAFunction *func) {
    fprintf(sink, STR_FMT ":\n", STR_ARG(func->name));
    fprintf(sink, "  enter %ld, 0\n", func->max_temps * 8);

    for (size_t i = 0; i < func->body.count; i++) { generate_statement(sink, &func->body.items[i]); }

    fprintf(sink, "ret%ld:\n", f_count);
    fprintf(sink, "  leave\n");
    fprintf(sink, "  ret\n");
    f_count++;

    return true;
}

static bool generate_statement(FILE *sink, const SSAStatement *st) {
    switch (st->type) {
    case SSAST_RETURN: {
        emit_return_some(sink, st);
        return true;
    }
    case SSAST_RETURN_EMPTY: {
        emit_return_none(sink, st);
        return true;
    }
    case SSAST_ADD: {
        emit_add(sink, st);
        return true;
    }
    case SSAST_SUB: {
        emit_sub(sink, st);
        return true;
    }
    case SSAST_MUL: {
        emit_imul(sink, st);
        return true;
    }
    case SSAST_DIV: {
        emit_div(sink, st);
        return true;
    }
    case SSAST_ASSIGN: {
        emit_assign(sink, st);
        return true;
    }
    case SSAST_CALL: {
        emit_call(sink, st);
        return true;
    }
    default: TODO();
    }
}

static void emit_return_some(FILE *sink, const SSAStatement *ret) {
    ASSERT(ret->type == SSAST_RETURN,
           "This function should only be called when the type of the statement is SSAST_RETURN");
    move_value_into_register(sink, "rax", &ret->ret.value);
    fprintf(sink, "  jmp ret%ld\n", f_count);
}

static void emit_return_none(FILE *sink, const SSAStatement *ret_none) {
    ASSERT(ret_none->type == SSAST_RETURN_EMPTY,
           "This function should only be called when the type of the statement is SSAST_RETURN_EMPTY");
    fprintf(sink, "  jmp ret%ld\n", f_count);
}

static void emit_add(FILE *sink, const SSAStatement *st) {
    ASSERT(st->type == SSAST_ADD, "This function should only be called when the type of the statement is SSAST_ADD");
    ASSERT(st->binop.result.type == SSAVT_TEMP, "we can't add to a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_add_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_sub(FILE *sink, const SSAStatement *st) {
    ASSERT(st->type == SSAST_SUB, "This function should only be called when the type of the statement is SSAST_SUB");
    ASSERT(st->binop.result.type == SSAVT_TEMP, "we can't sub a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_sub_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_imul(FILE *sink, const SSAStatement *st) {
    ASSERT(st->type == SSAST_MUL, "This function should only be called when the type of the statement is SSAST_MUL");
    ASSERT(st->binop.result.type == SSAVT_TEMP, "we can't mul a constant");
    move_value_into_register(sink, "rax", &st->binop.l);

    emit_imul_reg_value(sink, "rax", &st->binop.r);
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_div(FILE *sink, const SSAStatement *st) {
    // ugh x86_64 is so weird
    // rax low bits
    // rdi high bits
    ASSERT(st->type == SSAST_DIV, "This function should only be called when the type of the statement is SSAST_DIV");
    ASSERT(st->binop.result.type == SSAVT_TEMP, "we can't div a constant");
    move_value_into_register(sink, "rax", &st->binop.l);
    fprintf(sink, "  xor rdx, rdx\n");
    move_value_into_register(sink, "rcx", &st->binop.r);
    fprintf(sink, "  div rcx\n");
    fprintf(sink, "  mov qword [rbp - %ld], rax\n", (st->binop.result.temp + 1) * 8);
}

static void emit_assign(FILE *sink, const SSAStatement *st) {
    ASSERT(st->type == SSAST_ASSIGN,
           "This function should only be called when the type of the statement is SSAST_ASSIGN");

    SSAValue temp_value = {.temp = st->assign.place, .type = SSAVT_TEMP};
    move_value_into_value(sink, &st->assign.value, &temp_value);
}

static void emit_call(FILE *sink, const SSAStatement *st) {
    ASSERT(st->type == SSAST_CALL, "This function should only be called when the type of the statement is SSAST_CALL");
    fprintf(sink, "  call "STR_FMT"\n", STR_ARG(st->call.name));
    if (st->call.returns) {
        fprintf(sink, "  mov ");
        value_asm_repr(sink, &st->call.return_v);
        fprintf(sink, ", rax\n");
    }
}

static void move_value_into_value(FILE *sink, const SSAValue *from, const SSAValue *into) {
    move_value_into_register(sink, "rax", from);

    fprintf(sink, "  mov ");
    value_asm_repr(sink, into);
    fprintf(sink, ", rax\n");
}

static void emit_add_reg_value(FILE *sink, const char *reg, const SSAValue *value) {
    fprintf(sink, "  add %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void emit_sub_reg_value(FILE *sink, const char *reg, const SSAValue *value) {
    fprintf(sink, "  sub %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void emit_imul_reg_value(FILE *sink, const char *reg, const SSAValue *value) {
    fprintf(sink, "  imul %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void move_value_into_register(FILE *sink, const char *reg, const SSAValue *value) {
    fprintf(sink, "  mov %s, ", reg);
    value_asm_repr(sink, value);
    fprintf(sink, "\n");
}

static void value_asm_repr(FILE *sink, const SSAValue *value) {
    switch (value->type) {
    case SSAVT_CONST: {
        fprintf(sink, "%ld", value->constant);
        break;
    }
    case SSAVT_TEMP: {
        fprintf(sink, "qword [rbp - %ld]", (value->temp + 1) * 8);
        break;
    }
    }
}
