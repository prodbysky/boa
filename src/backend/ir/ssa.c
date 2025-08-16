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
// now will try to strip not needed statements
static bool dce(SSAModule *mod);

// does this:
//     where a + b and if a or b is 0 then changes the statement to be just setting to a or b
//     where a * b and if a or b is 0 then changes the statement to be just setting to 0
//     where a / b and if a is 0 then changes the statement to assign 0
static bool algebraic_simp(SSAModule *mod);

static bool constant_folding(SSAModule *mod);

#define MAX_LOOPS 100

bool optimize_ssa_ir(SSAModule *mod) {
    for (int i = 0; i < MAX_LOOPS; i++) {
        bool changed_something = false;
        changed_something |= constant_folding(mod);
        changed_something |= algebraic_simp(mod);
        changed_something |= dce(mod);
        if (!changed_something) break;
    }
    return true;
}

typedef struct {
    TempValueIndex idx;
    uint64_t val;
} KnownValue;

typedef struct {
    KnownValue *items;
    size_t count;
    size_t capacity;
} KnownValues;

static bool get_if_known(const KnownValues *vs, const SSAValue *v, uint64_t *out) {
    if (v->type == SSAVT_CONST) {
        *out = v->constant;
        return true;
    }
    for (size_t i = 0; i < vs->count; i++) {
        if (vs->items[i].idx == v->temp) {
            *out = vs->items[i].val;
            return true;
        }
    }
    return false;
}

static bool constant_folding(SSAModule *mod) {
    bool changed_something = false;
    for (size_t i = 0; i < mod->functions.count; i++) {
        SSAFunction *f = &mod->functions.items[i];
        KnownValues vs = {0};
        for (size_t j = 0; j < f->body.count; j++) {
            SSAStatement *st_old = &f->body.items[j];
            switch (st_old->type) {
            case SSAST_ASSIGN: {
                if (st_old->assign.value.type == SSAVT_CONST) {
                    KnownValue v = {.idx = st_old->assign.place, .val = st_old->assign.value.constant};
                    da_push(&vs, v);
                    break;
                }
                uint64_t const_v = 0;
                if (get_if_known(&vs, &st_old->assign.value, &const_v)) {
                    KnownValue v = {.idx = st_old->assign.place, .val = const_v};
                    da_push(&vs, v);
                    SSAStatement st_new = *st_old;
                    st_new.assign.value = (SSAValue){.type = SSAVT_CONST, .constant = const_v};
                    *st_old = st_new;
                    changed_something = true;
                }
                break;
            }
            case SSAST_MUL: {
                uint64_t l, r;
                if (get_if_known(&vs, &st_old->binop.l, &l) && get_if_known(&vs, &st_old->binop.r, &r)) {
                    KnownValue v = {.idx = st_old->binop.result.temp, .val = l * r};
                    SSAStatement st_new = {.type = SSAST_ASSIGN,
                                           .assign = {.place = st_old->binop.result.temp,
                                                      .value = (SSAValue){.type = SSAVT_CONST, .constant = l * r}}};
                    da_push(&vs, v);
                    *st_old = st_new;
                    changed_something = true;
                }
                break;
            }
            case SSAST_DIV: {
                uint64_t l, r;
                if (get_if_known(&vs, &st_old->binop.l, &l) && get_if_known(&vs, &st_old->binop.r, &r)) {
                    KnownValue v = {.idx = st_old->binop.result.temp, .val = l / r};
                    SSAStatement st_new = {.type = SSAST_ASSIGN,
                                           .assign = {.place = st_old->binop.result.temp,
                                                      .value = (SSAValue){.type = SSAVT_CONST, .constant = l / r}}};
                    da_push(&vs, v);
                    *st_old = st_new;
                    changed_something = true;
                }
                break;
            }
            case SSAST_ADD: {
                uint64_t l, r;
                if (get_if_known(&vs, &st_old->binop.l, &l) && get_if_known(&vs, &st_old->binop.r, &r)) {
                    KnownValue v = {.idx = st_old->binop.result.temp, .val = l + r};
                    SSAStatement st_new = {.type = SSAST_ASSIGN,
                                           .assign = {.place = st_old->binop.result.temp,
                                                      .value = (SSAValue){.type = SSAVT_CONST, .constant = l + r}}};
                    da_push(&vs, v);
                    *st_old = st_new;
                    changed_something = true;
                }
                break;
            }
            case SSAST_SUB: {
                uint64_t l, r;
                if (get_if_known(&vs, &st_old->binop.l, &l) && get_if_known(&vs, &st_old->binop.r, &r)) {
                    KnownValue v = {.idx = st_old->binop.result.temp, .val = l - r};
                    SSAStatement st_new = {.type = SSAST_ASSIGN,
                                           .assign = {.place = st_old->binop.result.temp,
                                                      .value = (SSAValue){.type = SSAVT_CONST, .constant = l - r}}};
                    da_push(&vs, v);
                    *st_old = st_new;
                    changed_something = true;
                }
                break;
            }
            case SSAST_RETURN: {
                uint64_t v = 0;
                if (get_if_known(&vs, &st_old->ret.value, &v)) {
                    st_old->ret.value = (SSAValue){.type = SSAVT_CONST, .constant = v};
                    changed_something = true;
                    break;
                }
                break;
            }
            case SSAST_RETURN_EMPTY: break;
            default:
                log_message(LL_INFO, "%ld", st_old->type);
                TODO();
                break;
            }
        }
        if (vs.items != NULL) free(vs.items);
    }

    return changed_something;
}

static bool dce(SSAModule *mod) {
    bool changed = false;
    // first strip off statements after return
    for (size_t i = 0; i < mod->functions.count; i++) {
        for (size_t j = 0; j < mod->functions.items[i].body.count; j++) {
            if (mod->functions.items[i].body.items[j].type == SSAST_RETURN ||
                mod->functions.items[i].body.items[j].type == SSAST_RETURN_EMPTY) {
                if (mod->functions.items[i].body.capacity > j + 1) {
                    mod->functions.items[i].body.count = j + 1;
                    changed = true;
                    break;
                }
            }
        }
    }

    for (size_t i = 0; i < mod->functions.count; i++) {
        SSAFunction *f = &mod->functions.items[i];
        bool *used = calloc(f->max_temps, sizeof(bool));
        for (size_t j = 0; j < f->body.count; j++) {
            SSAStatement *stmt = &f->body.items[j];
            switch (stmt->type) {
            case SSAST_ADD:
            case SSAST_SUB:
            case SSAST_MUL:
            case SSAST_DIV:
                if (stmt->binop.result.type == SSAVT_TEMP) {
                    if (stmt->binop.l.type == SSAVT_TEMP) used[stmt->binop.l.temp] = true;
                    if (stmt->binop.r.type == SSAVT_TEMP) used[stmt->binop.r.temp] = true;
                }
                break;
            case SSAST_ASSIGN:
                if (stmt->assign.value.type == SSAVT_TEMP) used[stmt->assign.value.temp] = true;
                break;
            case SSAST_RETURN:
                if (stmt->ret.value.type == SSAVT_TEMP) used[stmt->ret.value.temp] = true;
                break;
            case SSAST_RETURN_EMPTY: break;
            }
        }

        size_t write_idx = 0;
        for (size_t read_idx = 0; read_idx < f->body.count; read_idx++) {
            SSAStatement *stmt = &f->body.items[read_idx];
            bool keep = true;

            switch (stmt->type) {
            case SSAST_ADD:
            case SSAST_SUB:
            case SSAST_MUL:
            case SSAST_DIV:
                if (stmt->binop.result.type == SSAVT_TEMP && !used[stmt->binop.result.temp]) keep = false;
                break;
            case SSAST_ASSIGN:
                if (!used[stmt->assign.place]) keep = false;
                break;
            default: break;
            }

            if (keep) {
                f->body.items[write_idx++] = f->body.items[read_idx];
                changed = true;
            }
        }
        f->body.count = write_idx;

        free(used);
    }

    return changed;
}

static bool algebraic_simp(SSAModule *mod) {
    bool changed = false;
    for (size_t i = 0; i < mod->functions.count; i++) {
        for (size_t j = 0; j < mod->functions.items[i].body.count; j++) {
            SSAStatement *s = &mod->functions.items[i].body.items[j];

            switch (s->type) {
            case SSAST_ADD:
            case SSAST_SUB: {
                if (s->binop.r.type == SSAVT_CONST && s->binop.r.constant == 0) {
                    *s = (SSAStatement){.type = SSAST_ASSIGN,
                                        .assign = {.place = s->binop.result.temp, .value = s->binop.l}};
                    changed = true;
                    break;
                }
                if (s->binop.l.type == SSAVT_CONST && s->binop.l.constant == 0) {
                    *s = (SSAStatement){.type = SSAST_ASSIGN,
                                        .assign = {.place = s->binop.result.temp, .value = s->binop.r}};
                    changed = true;
                    break;
                }
                break;
            }
            case SSAST_MUL: {
                if (s->binop.r.type == SSAVT_CONST && s->binop.r.constant == 0) {
                    *s = (SSAStatement){.type = SSAST_ASSIGN,
                                        .assign = {
                                            .place = s->binop.result.temp,
                                            .value = (SSAValue){.type = SSAVT_CONST, .constant = 0},
                                        }};
                    changed = true;
                    break;
                }
                if (s->binop.l.type == SSAVT_CONST && s->binop.l.constant == 0) {
                    *s = (SSAStatement){.type = SSAST_ASSIGN,
                                        .assign = {
                                            .place = s->binop.result.temp,
                                            .value = (SSAValue){.type = SSAVT_CONST, .constant = 0},
                                        }};
                    changed = true;
                    break;
                }
                break;
            }
            case SSAST_DIV: {
                if (s->binop.l.type == SSAVT_CONST && s->binop.l.constant == 0) {
                    *s = (SSAStatement){.type = SSAST_ASSIGN,
                                        .assign = {
                                            .place = s->binop.result.temp,
                                            .value = (SSAValue){.type = SSAVT_CONST, .constant = 0},
                                        }};
                    changed = true;
                    break;
                }

                break;
            }
            case SSAST_ASSIGN:
            case SSAST_RETURN:
            case SSAST_RETURN_EMPTY: break;
            }
        }
    }
    return changed;
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
