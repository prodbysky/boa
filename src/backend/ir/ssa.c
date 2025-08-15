#include "ssa.h"
#include "../../util.h"
#include <stdint.h>
#include <stdlib.h>

// TODO: MULTIPLE FUNCTIONS
bool generate_ssa_module(const AstTree *ast, SSAModule *out) {
    ASSERT(ast, "Sanity check");
    ASSERT(out, "Sanity check");
    SSAFunction main_func = {0};
    main_func.name = "main";

    for (size_t i = 0; i < ast->count; i++) {
        if (!generate_ssa_statement(ast, &ast->items[i], &main_func)) return false;
    }
    da_push(&out->functions, main_func);

    return true;
}

// so far will only do dce for instructions after the return statement
static bool dce(SSAModule *mod);

bool optimize_ssa_ir(SSAModule *mod) {
    if (!dce(mod)) return false;
    return true;
}
static bool dce(SSAModule *mod) {
    for (size_t i = 0; i < mod->functions.count; i++) {
        for (size_t j = 0; j < mod->functions.items[i].body.count; j++) {
            if (mod->functions.items[i].body.items[j].type == SSAST_RETURN ||
                mod->functions.items[i].body.items[j].type == SSAST_RETURN_EMPTY) {
                if (mod->functions.items[i].body.capacity > j + 1) { mod->functions.items[i].body.count = j + 1; }
            }
        }
    }
    return true;
}

bool generate_ssa_statement(const AstTree *tree, const AstStatement *st, SSAFunction *out) {
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
        da_push(&out->variables, pair);
        return true;
    } break;
    }
    UNREACHABLE("This shouldn't ever be reached, so all statements should early return from their case in "
                "the switch "
                "statement");
    return false;
}
bool generate_ssa_expr(const AstTree *tree, const AstExpression *expr, SSAValue *out_value, SSAFunction *out) {
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
        for (size_t i = 0; i < out->variables.count; i++) {
            if (strncmp(out->variables.items[i].name.items, expr->ident.items, expr->ident.count) == 0) {
                out_value->type = SSAVT_TEMP;
                out_value->temp = out->variables.items[i].index;
                return true;
            }
        }
        log_diagnostic(LL_ERROR, "Found an unknown identifier in place of a expression");
        report_error(expr->begin, expr->begin + expr->len, tree->source.src.items, tree->source.name);
        return false;
    }
    }
    UNREACHABLE("Again this shouldn't ever be reached")

    return false;
}
