#include "ssa.h"
#include "../../util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate_return_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                               Arena *arena);
static bool generate_let_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs, Arena *arena);
static bool generate_assign_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                               Arena *arena);
static bool generate_call_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                             Arena *arena);
static bool generate_if_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs, Arena *arena);
static bool generate_while_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                              Arena *arena);
static bool generate_asm_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs, Arena *arena);

bool generate_module(const AstRoot *ast, Module *out, Arena *arena) {
    ASSERT(ast, "Sanity check");
    ASSERT(out, "Sanity check");

    for (size_t i = 0; i < ast->fs.count; i++) {
        AstFunction *f = &ast->fs.items[i];
        Function func = {0};
        if (!generate_function(&func, f, arena, &out->strings, ast)) return false;
        da_push(&out->functions, func, arena);
    }

    return true;
}

bool generate_function(Function *out, const AstFunction *ast_func, Arena *arena, StringPool *strs,
                       const AstRoot *tree) {

    out->arg_count = ast_func->args.count;
    push_scope(&out->scopes, arena);
    for (size_t i = 0; i < ast_func->args.count; i++) {
        define_sym(&out->scopes, ast_func->args.items[i], (Value){.type = VT_ARG, .arg_index = i}, arena);
    }
    out->name = ast_func->name;
    for (size_t i = 0; i < ast_func->body.count; i++) {
        if (!generate_statement(tree, &ast_func->body.items[i], out, strs, arena)) return false;
    }
    return true;
}

void push_scope(ScopeStack *stack, Arena *arena) { da_push(stack, (SymTable){}, arena); }
void pop_scope(ScopeStack *stack) {
    ASSERT(stack->count > 0, "this should never NEVER be true");
    stack->count--;
}

bool define_sym(ScopeStack *stack, StringView name, Value value, Arena *arena) {
    SymTable *table = &stack->items[stack->count - 1];
    Sym s = {.name = name, .value = value};
    da_push(table, s, arena);
    return true;
}

bool lookup_sym(ScopeStack *stack, StringView name, Sym **out_value) {
    ASSERT(stack->count > 0,
           "There should always be a scope in this struct if not that means someone popped an extra stack");
    for (size_t i = stack->count; i != 0; i--) {
        SymTable *current_stack = &stack->items[i - 1];
        for (size_t j = 0; j < current_stack->count; j++) {
            const StringView *candidate = &current_stack->items[j].name;
            if (candidate->count == name.count && strncmp(name.items, candidate->items, name.count) == 0) {
                *out_value = &current_stack->items[j];
                return true;
            }
        }
    }

    return false;
}

bool generate_statement(const AstRoot *tree, const AstStatement *st, Function *out, StringPool *strs, Arena *arena) {
    ASSERT(st, "Sanity check");
    switch (st->type) {
    case AST_RETURN: return generate_return_st(tree, out, st, strs, arena);
    case AST_LET: return generate_let_st(tree, out, st, strs, arena);
    case AST_ASSIGN: return generate_assign_st(tree, out, st, strs, arena);
    case AST_CALL: return generate_call_st(tree, out, st, strs, arena);
    case AST_IF: return generate_if_st(tree, out, st, strs, arena);
    case AST_WHILE: return generate_while_st(tree, out, st, strs, arena);
    case AST_ASM: return generate_asm_st(tree, out, st, strs, arena);
    }
    UNREACHABLE("This shouldn't ever be reached, so all statements should early return from their case in "
                "the switch "
                "statement");
    return false;
}
bool generate_expr(const AstRoot *tree, const AstExpression *expr, Value *out_value, Function *out, StringPool *strs,
                   Arena *arena) {
    ASSERT(expr, "Sanity check");
    ASSERT(out_value, "Sanity check");

    switch (expr->type) {
    case AET_PRIMARY: {
        out_value->type = VT_CONST;
        out_value->constant = expr->number;
        return true;
    }
    case AET_BINARY: {
        Value l = {0};
        Value r = {0};
        if (!generate_expr(tree, expr->bin.l, &l, out, strs, arena)) return false;
        if (!generate_expr(tree, expr->bin.r, &r, out, strs, arena)) return false;
        Value result = {.type = VT_TEMP, .temp = out->max_temps++};
        Statement st = {.binop = {.l = l, .r = r, .result = result}};
        switch (expr->bin.op) {
        case OT_PLUS: st.type = ST_ADD; break;
        case OT_MINUS: st.type = ST_SUB; break;
        case OT_MULT: st.type = ST_MUL; break;
        case OT_DIV: st.type = ST_DIV; break;
        case OT_LT: st.type = ST_CMP_LT; break;
        case OT_MT: st.type = ST_CMP_MT; break;
        }
        da_push(&out->body, st, arena);
        *out_value = result;

        return true;
    }
    case AET_IDENT: {
        Sym *p = NULL;
        if (!lookup_sym(&out->scopes, expr->ident, &p)) {
            log_diagnostic(LL_ERROR, "Found an unknown identifier in place of a expression");
            report_error(expr->begin, tree->source.src.items, tree->source.name);
            return false;
        }
        *out_value = p->value;
        return true;
    }
    case AET_FUNCTION_CALL: {
        // TODO: Check for undefined functions
        // To avoid header files and such, we should go through each of the function definitions first
        // and push them into the module before generating
        // for now just yolo
        // also all functions just return a 64bit number for now
        Value result_value = {.type = VT_TEMP, .temp = out->max_temps++};

        InputArgs args = {0};
        for (size_t i = 0; i < expr->func_call.args.count; i++) {
            Value v = {0};
            if (!generate_expr(tree, &expr->func_call.args.items[i], &v, out, strs, arena)) return false;
            da_push(&args, v, arena);
        }
        Statement st = {.type = ST_CALL,
                        .call = {
                            .returns = true,
                            .return_v = result_value,
                            .name = expr->func_call.name,
                            .args = args,
                        }};
        da_push(&out->body, st, arena);
        *out_value = result_value;
        return true;
    }
    case AET_STRING: {
        da_push(strs, expr->string, arena);
        out_value->type = VT_STRING;
        out_value->string_index = strs->count - 1;
        return true;
    }
    }
    UNREACHABLE("Again this shouldn't ever be reached")

    return false;
}

void ir_value_repr(const Value *v) {
    switch (v->type) {
    case VT_CONST: {
        printf("%zu", v->constant);
        return;
    }
    case VT_TEMP: {
        printf("$%zu", v->temp);
        return;
    }
    case VT_STRING: {
        printf("^%zu", v->string_index);
        return;
    }
    case VT_ARG: {
        printf("#%zu", v->arg_index);
        return;
    }
    }
}

void dump_ir(const Module *mod) {
    for (size_t i = 0; i < mod->functions.count; i++) {
        const Function *f = &mod->functions.items[i];
        printf("function " STR_FMT "(", STR_ARG(f->name));
        for (size_t j = 0; j < f->arg_count; j++) { printf("#%zu ", j); }
        printf(") {\n");
        for (size_t j = 0; j < f->body.count; j++) {
            const Statement *st = &f->body.items[j];
            printf("  ");
            switch (st->type) {
            case ST_ADD: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" + ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case ST_SUB: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" - ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case ST_MUL: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" * ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case ST_DIV: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" / ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case ST_ASM: {
                printf("asm(");
                printf(STR_FMT, STR_ARG(st->asm));
                printf("\n  )");
                break;
            }
            case ST_ASSIGN: {
                ir_value_repr(&st->assign.place);
                printf(" <- ");
                ir_value_repr(&st->assign.value);
                break;
            }
            case ST_RETURN: {
                printf("ret <- ");
                ir_value_repr(&st->ret.value);
                break;
            }
            case ST_RETURN_EMPTY: {
                printf("ret");
                break;
            }
            case ST_JMP: {
                printf("jump @%zu", st->jmp);
                break;
            }
            case ST_JZ: {
                printf("jz ");
                ir_value_repr(&st->jz.cond);
                printf(" @%zu", st->jz.to);
                break;
            }
            case ST_CALL: {
                if (st->call.returns) {
                    ir_value_repr(&st->call.return_v);
                    printf(" <- ");
                }
                printf("call " STR_FMT "(", STR_ARG(st->call.name));
                for (size_t k = 0; k < st->call.args.count; k++) {
                    ir_value_repr(&st->call.args.items[k]);
                    if (k + 1 < st->call.args.count) printf(", ");
                }
                printf(")");
                break;
            }
            case ST_LABEL: {
                printf("@%zu:", st->label);
                break;
            }
            }
            printf("\n");
        }

        printf("}\n");
    }
}

static bool generate_return_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                               Arena *arena) {
    if (!st->ret.has_expr) {
        da_push(&out->body, (Statement){.type = ST_RETURN_EMPTY}, arena);
        return true;
    } else {
        Value value = {0};
        if (!generate_expr(tree, &st->ret.return_expr, &value, out, strs, arena)) return false;
        Statement st = (Statement){.type = ST_RETURN, .ret = {value}};
        da_push(&out->body, st, arena);
        return true;
    }
}
static bool generate_let_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                            Arena *arena) {
    Value variable_value = {0};
    if (!generate_expr(tree, &st->let.value, &variable_value, out, strs, arena)) return false;
    TempValueIndex place = out->max_temps++;
    define_sym(&out->scopes, st->let.name, (Value){.type = VT_TEMP, .temp = place}, arena);
    Statement ir_st = {
        .type = ST_ASSIGN,
        .assign = {.place = (Value){.type = VT_TEMP, .temp = place}, .value = variable_value},
    };
    da_push(&out->body, ir_st, arena);
    return true;
}
static bool generate_assign_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                               Arena *arena) {
    Value variable_value_new = {0};
    if (!generate_expr(tree, &st->assign.value, &variable_value_new, out, strs, arena)) return false;
    Sym *p;
    if (!lookup_sym(&out->scopes, st->assign.name, &p)) {
        log_diagnostic(LL_ERROR, "Tried to reassign an unknown variable");
        report_error(st->begin, tree->source.src.items, tree->source.name);
        return false;
    }
    Statement ir_st = {
        .type = ST_ASSIGN,
        .assign = {.place = p->value, .value = variable_value_new},
    };
    da_push(&out->body, ir_st, arena);
    return true;
}
static bool generate_call_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                             Arena *arena) {
    InputArgs args = {0};
    for (size_t i = 0; i < st->call.args.count; i++) {
        Value v = {0};
        if (!generate_expr(tree, &st->call.args.items[i], &v, out, strs, arena)) return false;
        da_push(&args, v, arena);
    }
    Statement call_st = {.type = ST_CALL, .call = {.name = st->call.name, .args = args}};
    da_push(&out->body, call_st, arena);
    return true;
}
static bool generate_if_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs, Arena *arena) {
    Value v = {0};
    if (!generate_expr(tree, &st->if_st.cond, &v, out, strs, arena)) return false;
    uint64_t jump_over = out->label_count++;
    Statement jump_st = {.type = ST_JZ, .jz = {.cond = v, .to = jump_over}};
    da_push(&out->body, jump_st, arena);
    push_scope(&out->scopes, arena);
    for (size_t i = 0; i < st->if_st.block.count; i++) {
        if (!generate_statement(tree, &st->if_st.block.items[i], out, strs, arena)) return false;
    }
    Statement label_st = {
        .type = ST_LABEL,
        .label = jump_over,
    };
    da_push(&out->body, label_st, arena);
    pop_scope(&out->scopes);
    return true;
}
static bool generate_while_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                              Arena *arena) {
    uint64_t header = out->label_count++;
    uint64_t over = out->label_count++;
    Statement header_st = {
        .type = ST_LABEL,
        .label = header,
    };
    Statement over_st = {
        .type = ST_LABEL,
        .label = over,
    };
    da_push(&out->body, header_st, arena);
    Value v = {0};
    if (!generate_expr(tree, &st->while_st.cond, &v, out, strs, arena)) return false;
    Statement jump_st = {.type = ST_JZ, .jz = {.cond = v, .to = over}};
    da_push(&out->body, jump_st, arena);
    push_scope(&out->scopes, arena);
    for (size_t i = 0; i < st->while_st.block.count; i++) {
        if (!generate_statement(tree, &st->while_st.block.items[i], out, strs, arena)) return false;
    }
    Statement jump_back_st = {.type = ST_JMP, .jmp = header};
    da_push(&out->body, jump_back_st, arena);
    da_push(&out->body, over_st, arena);
    pop_scope(&out->scopes);
    return true;
}
static bool generate_asm_st(const AstRoot *tree, Function *out, const AstStatement *st, StringPool *strs,
                            Arena *arena) {
    (void) strs;
    (void) tree;
    Statement s = {.type = ST_ASM, .asm = st->asm};
    da_push(&out->body, s, arena);
    return true;
}
