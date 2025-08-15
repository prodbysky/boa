#include "pseudo_reg.h"
#include "../../util.h"
#include "ssa.h"

static bool pseudo_reg_convert_function(const SSAFunction *func, PseudoRegFunction *out);
static bool pseudo_reg_convert_statement(const SSAStatement *st, PseudoRegFunction *out);

static PseudoRegInstructionType ssa_to_ir_binop(const SSAStatementType type);

bool pseudo_reg_convert_module(const SSAModule *ssa_mod, PseudoRegModule *out) {
    for (size_t i = 0; i < ssa_mod->functions.count; i++) {
        PseudoRegFunction func = {0};
        if (!pseudo_reg_convert_function(&ssa_mod->functions.items[i], &func)) return false;
        da_push(&out->fs, func);
    }
    return false;
}

static bool pseudo_reg_convert_function(const SSAFunction *func, PseudoRegFunction *out) {
    (void)func;
    (void)out;
    for (size_t i = 0; i < func->body.count; i++) {
        if (!pseudo_reg_convert_statement(&func->body.items[i], out)) return false;
    }
    return false;
}

static bool pseudo_reg_convert_statement(const SSAStatement *st, PseudoRegFunction *out) {
    switch (st->type) {
    case SSAST_ADD:
    case SSAST_SUB:
    case SSAST_MUL:
    case SSAST_DIV: {
        PseudoRegInstruction instr = {.type = ssa_to_ir_binop(st->type),
                                      .add = {.dest = st->binop.result, .src_1 = st->binop.l, .src_2 = st->binop.r}};
        da_push(&out->body, instr);
        return true;
    }
    case SSAST_RETURN: {
        PseudoRegInstruction instr = {.type = PRT_RET, .ret = {.with_value = true, .value = st->ret.value}};
        da_push(&out->body, instr);
        return true;
    }
    case SSAST_RETURN_EMPTY: {
        PseudoRegInstruction instr = {.type = PRT_RET, .ret = {.with_value = false}};
        da_push(&out->body, instr);
        return true;
    }
    case SSAST_ASSIGN: {
        PseudoRegInstruction instr = {.type = PRT_MOV,
                                      .mov = {
                                          .dest = st->assign.place,
                                          .src = st->assign.value,
                                      }};
        da_push(&out->body, instr);
        return true;
    }
    }

    return false;
}

static PseudoRegInstructionType ssa_to_ir_binop(const SSAStatementType type) {
    switch (type) {
    case SSAST_ADD: return PRT_ADD;
    case SSAST_SUB: return PRT_SUB;
    case SSAST_MUL: return PRT_MUL;
    case SSAST_DIV: return PRT_DIV;
    default: UNREACHABLE("What did you pass here :sus:");
    }
}
