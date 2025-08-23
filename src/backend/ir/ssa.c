#include "ssa.h"
#include "../../util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool generate_ssa_module(const AstRoot *ast, SSAModule *out, Arena *arena) {
    ASSERT(ast, "Sanity check");
    ASSERT(out, "Sanity check");

    for (size_t i = 0; i < ast->fs.count; i++) {
        AstFunction *f = &ast->fs.items[i];
        SSAFunction func = {0};
        if (!generate_ssa_function(&func, f, arena, &out->strings, ast)) return false;
        da_push(&out->functions, func, arena);
    }

    return true;
}

bool generate_ssa_function(SSAFunction *out, const AstFunction *ast_func, Arena *arena, SSAStrings *strs,
                           const AstRoot *tree) {

    out->arg_count = ast_func->args.count;
    da_push(&out->scopes, (SSANameToValue){}, arena);
    for (size_t i = 0; i < ast_func->args.count; i++) {
        add_variable(
            &out->scopes,
            (NameValuePair){.name = ast_func->args.items[i], .value = (SSAValue){.type = SSAVT_ARG, .arg_index = i}},
            arena);
    }
    out->name = ast_func->name;
    for (size_t i = 0; i < ast_func->body.count; i++) {
        if (!generate_ssa_statement(tree, &ast_func->body.items[i], out, strs, arena)) return false;
    }
    return true;
}

bool get_if_known_variable(SSABadBoyStack *vals, StringView view, NameValuePair **out) {
    ASSERT(vals->count > 0,
           "There should always be a scope in this struct if not that means someone popped an extra stack");
    for (size_t i = vals->count; i != 0; i--) {
        SSANameToValue *current_stack = &vals->items[i - 1];
        for (size_t j = 0; j < current_stack->count; j++) {
            const StringView *candidate = &current_stack->items[j].name;
            if (candidate->count == view.count && strncmp(view.items, candidate->items, view.count) == 0) {
                *out = &current_stack->items[j];
                return true;
            }
        }
    }

    return false;
}

bool add_variable(SSABadBoyStack *stack, NameValuePair pair, Arena *arena) {
    ASSERT(stack->count > 0,
           "There should always be a scope in this struct if not that means someone popped an extra stack");
    SSANameToValue *scope = &stack->items[stack->count - 1];
    da_push(scope, pair, arena);
    return true;
}

bool generate_ssa_statement(const AstRoot *tree, const AstStatement *st, SSAFunction *out, SSAStrings *strs,
                            Arena *arena) {
    ASSERT(st, "Sanity check");
    switch (st->type) {
    case AST_RETURN: {
        if (!st->ret.has_expr) {
            da_push(&out->body, (SSAStatement){.type = SSAST_RETURN_EMPTY}, arena);
            return true;
        } else {
            SSAValue value = {0};
            if (!generate_ssa_expr(tree, &st->ret.return_expr, &value, out, strs, arena)) return false;
            SSAStatement st = (SSAStatement){.type = SSAST_RETURN, .ret = {value}};
            da_push(&out->body, st, arena);
            return true;
        }
    }
    case AST_LET: {
        SSAValue variable_value = {0};
        if (!generate_ssa_expr(tree, &st->let.value, &variable_value, out, strs, arena)) return false;
        TempValueIndex place = out->max_temps++;
        NameValuePair pair = {.name = st->let.name, .value = {.type = SSAVT_TEMP, .temp = place}};
        SSAStatement st = {
            .type = SSAST_ASSIGN,
            .assign = {.place = (SSAValue){.type = SSAVT_TEMP, .temp = place}, .value = variable_value},
        };
        da_push(&out->body, st, arena);
        add_variable(&out->scopes, pair, arena);
        return true;
    }
    case AST_ASSIGN: {
        SSAValue variable_value_new = {0};
        if (!generate_ssa_expr(tree, &st->assign.value, &variable_value_new, out, strs, arena)) return false;
        NameValuePair *p;
        if (!get_if_known_variable(&out->scopes, st->assign.name, &p)) {
            log_diagnostic(LL_ERROR, "Tried to reassign an unknown variable");
            report_error(st->begin, tree->source.src.items, tree->source.name);
            return false;
        }
        SSAStatement st = {
            .type = SSAST_ASSIGN,
            .assign = {.place = p->value, .value = variable_value_new},
        };
        da_push(&out->body, st, arena);
        return true;
    }
    case AST_CALL: {
        SSAInputArgs args = {0};
        for (size_t i = 0; i < st->call.args.count; i++) {
            SSAValue v = {0};
            if (!generate_ssa_expr(tree, &st->call.args.items[i], &v, out, strs, arena)) return false;
            da_push(&args, v, arena);
        }
        SSAStatement call_st = {.type = SSAST_CALL, .call = {.name = st->call.name, .args = args}};
        da_push(&out->body, call_st, arena);
        return true;
    }
    case AST_IF: {
        SSAValue v = {0};
        if (!generate_ssa_expr(tree, &st->if_st.cond, &v, out, strs, arena)) return false;
        uint64_t jump_over = out->label_count++;
        SSAStatement jump_st = {.type = SSAST_JZ, .jz = {.cond = v, .to = jump_over}};
        da_push(&out->body, jump_st, arena);
        da_push(&out->scopes, (SSANameToValue){}, arena);
        for (size_t i = 0; i < st->if_st.block.count; i++) {
            if (!generate_ssa_statement(tree, &st->if_st.block.items[i], out, strs, arena)) return false;
        }
        SSAStatement label_st = {
            .type = SSAST_LABEL,
            .label = jump_over,
        };
        da_push(&out->body, label_st, arena);
        out->scopes.count--;
        return true;
    }
    case AST_WHILE: {
        uint64_t header = out->label_count++;
        uint64_t over = out->label_count++;
        SSAStatement header_st = {
            .type = SSAST_LABEL,
            .label = header,
        };
        SSAStatement over_st = {
            .type = SSAST_LABEL,
            .label = over,
        };
        da_push(&out->body, header_st, arena);
        SSAValue v = {0};
        if (!generate_ssa_expr(tree, &st->while_st.cond, &v, out, strs, arena)) return false;
        SSAStatement jump_st = {.type = SSAST_JZ, .jz = {.cond = v, .to = over}};
        da_push(&out->body, jump_st, arena);
        da_push(&out->scopes, (SSANameToValue){}, arena);
        for (size_t i = 0; i < st->while_st.block.count; i++) {
            if (!generate_ssa_statement(tree, &st->while_st.block.items[i], out, strs, arena)) return false;
        }
        SSAStatement jump_back_st = {.type = SSAST_JMP, .jmp = header};
        da_push(&out->body, jump_back_st, arena);
        da_push(&out->body, over_st, arena);
        out->scopes.count--;
        return true;
    }
    case AST_ASM: {
        SSAStatement s = {.type = SSAST_ASM, .asm = st->asm};
        da_push(&out->body, s, arena);
        return true;
    }
    }
    UNREACHABLE("This shouldn't ever be reached, so all statements should early return from their case in "
                "the switch "
                "statement");
    return false;
}
bool generate_ssa_expr(const AstRoot *tree, const AstExpression *expr, SSAValue *out_value, SSAFunction *out,
                       SSAStrings *strs, Arena *arena) {
    ASSERT(expr, "Sanity check");
    ASSERT(out_value, "Sanity check");

    switch (expr->type) {
    case AET_PRIMARY: {
        out_value->type = SSAVT_CONST;
        out_value->constant = expr->number;
        return true;
    }
    case AET_BINARY: {
        SSAValue l = {0};
        SSAValue r = {0};
        if (!generate_ssa_expr(tree, expr->bin.l, &l, out, strs, arena)) return false;
        if (!generate_ssa_expr(tree, expr->bin.r, &r, out, strs, arena)) return false;
        SSAValue result = {.type = SSAVT_TEMP, .temp = out->max_temps++};
        SSAStatement st = {.binop = {.l = l, .r = r, .result = result}};
        switch (expr->bin.op) {
        case OT_PLUS: st.type = SSAST_ADD; break;
        case OT_MINUS: st.type = SSAST_SUB; break;
        case OT_MULT: st.type = SSAST_MUL; break;
        case OT_DIV: st.type = SSAST_DIV; break;
        }
        da_push(&out->body, st, arena);
        *out_value = result;

        return true;
    }
    case AET_IDENT: {
        NameValuePair *p = NULL;
        if (!get_if_known_variable(&out->scopes, expr->ident, &p)) {
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
        SSAValue result_value = {.type = SSAVT_TEMP, .temp = out->max_temps++};

        SSAInputArgs args = {0};
        for (size_t i = 0; i < expr->func_call.args.count; i++) {
            SSAValue v = {0};
            if (!generate_ssa_expr(tree, &expr->func_call.args.items[i], &v, out, strs, arena)) return false;
            da_push(&args, v, arena);
        }
        SSAStatement st = {.type = SSAST_CALL,
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
        out_value->type = SSAVT_STRING;
        out_value->string_index = strs->count - 1;
        return true;
    }
    }
    UNREACHABLE("Again this shouldn't ever be reached")

    return false;
}

void ir_value_repr(const SSAValue *v) {
    switch (v->type) {
    case SSAVT_CONST: {
        printf("%zu", v->constant);
        return;
    }
    case SSAVT_TEMP: {
        printf("$%zu", v->temp);
        return;
    }
    case SSAVT_STRING: {
        printf("^%zu", v->string_index);
        return;
    }
    case SSAVT_ARG: {
        printf("#%zu", v->arg_index);
        return;
    }
    }
}

void dump_ir(const SSAModule *mod) {
    for (size_t i = 0; i < mod->functions.count; i++) {
        const SSAFunction *f = &mod->functions.items[i];
        printf("function " STR_FMT "(", STR_ARG(f->name));
        for (size_t j = 0; j < f->arg_count; j++) { printf("#%zu ", j); }
        printf(") {\n");
        for (size_t j = 0; j < f->body.count; j++) {
            const SSAStatement *st = &f->body.items[j];
            printf("  ");
            switch (st->type) {
            case SSAST_ADD: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" + ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case SSAST_SUB: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" - ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case SSAST_MUL: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" * ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case SSAST_DIV: {
                ir_value_repr(&st->binop.result);
                printf(" <- ");
                ir_value_repr(&st->binop.l);
                printf(" / ");
                ir_value_repr(&st->binop.r);
                break;
            }
            case SSAST_ASM: {
                printf("asm(");
                printf(STR_FMT, STR_ARG(st->asm));
                printf("\n  )");
                break;
            }
            case SSAST_ASSIGN: {
                ir_value_repr(&st->assign.place);
                printf(" <- ");
                ir_value_repr(&st->assign.value);
                break;
            }
            case SSAST_RETURN: {
                printf("ret <- ");
                ir_value_repr(&st->ret.value);
                break;
            }
            case SSAST_RETURN_EMPTY: {
                printf("ret");
                break;
            }
            case SSAST_JMP: {
                printf("jump @%zu", st->jmp);
                break;
            }
            case SSAST_JZ: {
                printf("jz ");
                ir_value_repr(&st->jz.cond);
                printf(" @%zu", st->jz.to);
                break;
            }
            case SSAST_CALL: {
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
            case SSAST_LABEL: {
                printf("@%zu:", st->label);
                break;
            }
            }
            printf("\n");
        }

        printf("}\n");
    }
}
