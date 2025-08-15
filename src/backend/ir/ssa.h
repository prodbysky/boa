#ifndef SSA_H_
#define SSA_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../frontend/parser.h"

typedef enum {
    SSAVT_CONST,
    SSAVT_TEMP,
} SSAValueType;

typedef uint64_t ConstValue;
typedef uint64_t TempValueIndex;

typedef struct {
    SSAValueType type;
    union {
        ConstValue constant;
        TempValueIndex temp;
    };
} SSAValue;

// SSAS lol
typedef enum {
    SSAST_RETURN,
    SSAST_RETURN_EMPTY,
    SSAST_ADD,
    SSAST_SUB,
    SSAST_MUL,
    SSAST_DIV,
    SSAST_ASSIGN,
} SSAStatementType;

typedef struct {
    SSAStatementType type;
    union {
        struct {
            SSAValue value;
        } ret;
        struct {
            SSAValue l;
            SSAValue r;
            SSAValue result;
        } binop;
        struct {
            TempValueIndex place;
            SSAValue value;
        } assign;
    };
} SSAStatement;

typedef struct {
    SSAStatement* items;
    size_t count;
    size_t capacity;
} SSAFunctionBody;

typedef struct {
    StringView name;
    TempValueIndex index;
} NameValuePair;

typedef struct {
    NameValuePair* items;
    size_t count;
    size_t capacity;
} SSANameToValue;

typedef struct {
    const char* name;
    SSAFunctionBody body;
    size_t max_temps;
    SSANameToValue variables;
} SSAFunction;

typedef struct {
    SSAFunction* items;
    size_t count;
    size_t capacity;
} SSAFunctions;

typedef struct {
    SSAFunctions functions;
} SSAModule;

bool generate_ssa_module(const AstTree* ast, SSAModule* out);
bool generate_ssa_statement(const AstTree* tree, const AstStatement* st, SSAFunction* out);
bool generate_ssa_expr(const AstTree* tree, const AstExpression* expr, SSAValue* out_value, SSAFunction* out);

#endif
