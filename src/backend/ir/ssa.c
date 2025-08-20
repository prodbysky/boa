#include "ssa.h"
#include "../../util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool generate_ssa_module(const AstRoot *ast, SSAModule *out) {
    ASSERT(ast, "Sanity check");
    ASSERT(out, "Sanity check");
    for (size_t i = 0; i < ast->fs.count; i++) {
        AstFunction *f = &ast->fs.items[i];
        SSAFunction func = {0};
        func.arg_count = f->args.count;
        func.max_temps = f->args.count;
        da_push(&func.scopes, (SSANameToValue){});
        for (size_t i = 0; i < f->args.count; i++) {
            add_variable(&func.scopes, (NameValuePair){.name = f->args.items[i], .index = i});
        }
        func.name = ast->fs.items[i].name;
        for (size_t i = 0; i < f->body.count; i++) {
            if (!generate_ssa_statement(ast, &f->body.items[i], &func)) return false;
        }
        da_push(&out->functions, func);
    }

    return true;
}

bool get_if_known_variable(SSABadBoyStack *vals, StringView view, NameValuePair **out) {
    ASSERT(vals->count > 0,
           "There should always be a scope in this struct if not that means someone popped an extra stack");
    for (size_t i = vals->count; i != 0; i--) {
        SSANameToValue *current_stack = &vals->items[i - 1];
        for (size_t j = 0; j < current_stack->count; j++) {
            if (strncmp(view.items, current_stack->items[j].name.items, view.count) == 0) {
                *out = &current_stack->items[j];
                return true;
            }
        }
    }

    return false;
}

bool add_variable(SSABadBoyStack *stack, NameValuePair pair) {
    ASSERT(stack->count > 0,
           "There should always be a scope in this struct if not that means someone popped an extra stack");
    SSANameToValue *scope = &stack->items[stack->count - 1];
    da_push(scope, pair);
    return true;
}

bool generate_ssa_statement(const AstRoot *tree, const AstStatement *st, SSAFunction *out) {
    ASSERT(st, "Sanity check");
    switch (st->type) {
    case AST_RETURN: {
        if (!st->ret.has_expr) {
            da_push(&out->body, (SSAStatement){.type = SSAST_RETURN_EMPTY});
            return true;
        } else {
            SSAValue value = {0};
            if (!generate_ssa_expr(tree, &st->ret.return_expr, &value, out)) return false;
            SSAStatement st = (SSAStatement){.type = SSAST_RETURN, .ret = {value}};
            da_push(&out->body, st);
            return true;
        }
    }
    case AST_LET: {
        SSAValue variable_value = {0};
        if (!generate_ssa_expr(tree, &st->let.value, &variable_value, out)) return false;
        TempValueIndex place = out->max_temps++;
        NameValuePair pair = {.name = st->let.name, .index = place};
        SSAStatement st = {
            .type = SSAST_ASSIGN,
            .assign = {.place = place, .value = variable_value},
        };
        da_push(&out->body, st);
        add_variable(&out->scopes, pair);
        return true;
    }
    case AST_ASSIGN: {
        SSAValue variable_value_new = {0};
        if (!generate_ssa_expr(tree, &st->assign.value, &variable_value_new, out)) return false;
        NameValuePair *p;
        if (!get_if_known_variable(&out->scopes, st->assign.name, &p)) {
            log_diagnostic(LL_ERROR, "Tried to reassign an unknown variable");
            report_error(st->begin, st->begin + st->len, tree->source.src.items, tree->source.name);
            return false;
        }
        SSAStatement st = {
            .type = SSAST_ASSIGN,
            .assign = {.place = p->index, .value = variable_value_new},
        };
        da_push(&out->body, st);
        return true;
    }
    case AST_CALL: {
        SSAInputArgs args = {0};
        for (size_t i = 0; i < st->call.args.count; i++) {
            SSAValue v = {0};
            if (!generate_ssa_expr(tree, &st->call.args.items[i], &v, out)) return false;
            da_push(&args, v);
        }
        SSAStatement call_st = {.type = SSAST_CALL, .call = {.name = st->call.name, .args = args}};
        da_push(&out->body, call_st);
        return true;
    }
    case AST_IF: {
        SSAValue v = {0};
        if (!generate_ssa_expr(tree, &st->if_st.cond, &v, out)) return false;
        uint64_t jump_over = out->label_count++;
        SSAStatement jump_st = {.type = SSAST_JZ, .jz = {.cond = v, .to = jump_over}};
        da_push(&out->body, jump_st);
        da_push(&out->scopes, (SSANameToValue){});
        for (size_t i = 0; i < st->if_st.block.count; i++) {
            if (!generate_ssa_statement(tree, &st->if_st.block.items[i], out)) return false;
        }
        SSAStatement label_st = {
            .type = SSAST_LABEL,
            .label = jump_over,
        };
        da_push(&out->body, label_st);
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
        da_push(&out->body, header_st);
        SSAValue v = {0};
        if (!generate_ssa_expr(tree, &st->while_st.cond, &v, out)) return false;
        SSAStatement jump_st = {.type = SSAST_JZ, .jz = {.cond = v, .to = over}};
        da_push(&out->body, jump_st);
        da_push(&out->scopes, (SSANameToValue){});
        for (size_t i = 0; i < st->while_st.block.count; i++) {
            if (!generate_ssa_statement(tree, &st->while_st.block.items[i], out)) return false;
        }
        SSAStatement jump_back_st = {.type = SSAST_JMP, .jmp = header};
        da_push(&out->body, jump_back_st);
        da_push(&out->body, over_st);
        out->scopes.count--;
        return true;
    }
    case AST_ASM: {
        SSAStatement s = {
            .type = SSAST_ASM,
            .asm = st->asm
        };
        da_push(&out->body, s);
        return true;
    }
    }
    UNREACHABLE("This shouldn't ever be reached, so all statements should early return from their case in "
                "the switch "
                "statement");
    return false;
}
bool generate_ssa_expr(const AstRoot *tree, const AstExpression *expr, SSAValue *out_value, SSAFunction *out) {
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
        if (!generate_ssa_expr(tree, expr->bin.l, &l, out)) return false;
        if (!generate_ssa_expr(tree, expr->bin.r, &r, out)) return false;
        SSAValue result = {.type = SSAVT_TEMP, .temp = out->max_temps++};
        SSAStatement st = {.binop = {.l = l, .r = r, .result = result}};
        switch (expr->bin.op) {
        case OT_PLUS: st.type = SSAST_ADD; break;
        case OT_MINUS: st.type = SSAST_SUB; break;
        case OT_MULT: st.type = SSAST_MUL; break;
        case OT_DIV: st.type = SSAST_DIV; break;
        }
        da_push(&out->body, st);
        *out_value = result;

        return true;
    }
    case AET_IDENT: {
        NameValuePair *p = NULL;
        if (!get_if_known_variable(&out->scopes, expr->ident, &p)) {
            log_diagnostic(LL_ERROR, "Found an unknown identifier in place of a expression");
            report_error(expr->begin, expr->begin + expr->len, tree->source.src.items, tree->source.name);
            return false;
        }
        out_value->type = SSAVT_TEMP;
        out_value->temp = p->index;
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
            if (!generate_ssa_expr(tree, &expr->func_call.args.items[i], &v, out)) return false;
            da_push(&args, v);
        }
        SSAStatement st = {.type = SSAST_CALL,
                           .call = {
                               .returns = true,
                               .return_v = result_value,
                               .name = expr->func_call.name,
                               .args = args,
                           }};
        da_push(&out->body, st);
        *out_value = result_value;
        return true;
    }
    }
    UNREACHABLE("Again this shouldn't ever be reached")

    return false;
}
