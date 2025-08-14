#include "ir.h"
#include "../util.h"

// TODO: MULTIPLE FUNCTIONS
bool generate_ir_module(const AstTree *ast, IRModule *out) {
    ASSERT(ast, "Sanity check");
    ASSERT(out, "Sanity check");
    IRFunction main_func = {0};
    main_func.name = "main";

    for (size_t i = 0; i < ast->count; i++) {
        if (!generate_ir_statement(ast, &ast->items[i], &main_func)) return false;
    }
    da_push(&out->functions, main_func);

    return true;
}
bool generate_ir_statement(const AstTree *tree, const AstStatement *st, IRFunction *out) {
    ASSERT(st, "Sanity check");
    switch (st->type) {
    case AST_RETURN: {
        if (!st->ret.has_expr) {
            da_push(&out->body, (IRStatement){.type = IRST_RETURN_EMPTY});
            return true;
        } else {
            IRValue value = {0};
            if (!generate_ir_expr(tree, &st->ret.return_expr, &value, out)) return false;
            IRStatement st = (IRStatement){.type = IRST_RETURN, .ret = {value}};
            da_push(&out->body, st);
            return true;
        }
    }
    case AST_LET: {
        IRValue out_value = {0};
        if (!generate_ir_expr(tree, &st->let.value, &out_value, out)) return false;
        ASSERT(out_value.type == IRVT_TEMP, "yk idk how this will behave");
        NameValuePair pair = {.name = st->let.name, .index = out_value.temp};
        da_push(&out->variables, pair);
        return true;
    } break;
    }
    UNREACHABLE("This shouldn't ever be reached, so all statements should early return from their case in the switch "
                "statement");
    return false;
}
bool generate_ir_expr(const AstTree *tree, const AstExpression *expr, IRValue *out_value, IRFunction *out) {
    ASSERT(expr, "Sanity check");
    ASSERT(out_value, "Sanity check");

    switch (expr->type) {
    case AET_PRIMARY: {
        out_value->type = IRVT_TEMP;
        out_value->temp = out->max_temps++;
        IRStatement st = {
            .type = IRST_ASSIGN,
            .assign =
                {
                    .place = out_value->temp,
                    .value =
                        (IRValue){
                            .type = IRVT_CONST,
                            .constant = expr->number,
                        },
                },
        };
        da_push(&out->body, st);
        return true;
    }
    case AET_BINARY: {
        IRValue l = {0};
        IRValue r = {0};
        if (!generate_ir_expr(tree, expr->bin.l, &l, out)) return false;
        if (!generate_ir_expr(tree, expr->bin.r, &r, out)) return false;
        IRValue result = {.type = IRVT_TEMP, .temp = out->max_temps++};
        IRStatement st = {.binop = {.l = l, .r = r, .result = result}};
        switch (expr->bin.op) {
        case OT_PLUS: st.type = IRST_ADD; break;
        case OT_MINUS: st.type = IRST_SUB; break;
        case OT_MULT: st.type = IRST_MUL; break;
        case OT_DIV: st.type = IRST_DIV; break;
        }
        da_push(&out->body, st);
        *out_value = result;

        return true;
    }
    case AET_IDENT: {
        for (size_t i = 0; i < out->variables.count; i++) {
            if (strncmp(out->variables.items[i].name.items, expr->ident.items, expr->ident.count) == 0) {
                out_value->type = IRVT_TEMP;
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
