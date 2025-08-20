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

typedef enum {
    SSAST_RETURN,
    SSAST_RETURN_EMPTY,
    SSAST_ADD,
    SSAST_SUB,
    SSAST_MUL,
    SSAST_DIV,
    SSAST_ASSIGN,
    SSAST_CALL,
    SSAST_LABEL,
    SSAST_JZ,
} SSAStatementType;

typedef struct {
    SSAValue* items;
    size_t count;
    size_t capacity;
} SSAInputArgs;

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
        struct {
            StringView name;
            bool returns;
            SSAValue return_v;
            SSAInputArgs args;
        } call;
        struct {
            SSAValue cond;
            uint64_t to;
        } jz;
        uint64_t label;
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
    SSANameToValue* items;
    size_t count;
    size_t capacity;
} SSABadBoyStack;

bool get_if_known_variable(SSABadBoyStack *vals, StringView view, NameValuePair **out);
bool add_variable(SSABadBoyStack* stack, NameValuePair pair);

typedef struct {
    StringView name;
    SSAFunctionBody body;
    size_t max_temps;
    size_t arg_count;
    SSABadBoyStack scopes;
    uint64_t label_count;
} SSAFunction;

typedef struct {
    SSAFunction* items;
    size_t count;
    size_t capacity;
} SSAFunctions;

typedef struct {
    SSAFunctions functions;
} SSAModule;

bool generate_ssa_module(const AstRoot* ast, SSAModule* out);
bool generate_ssa_statement(const AstRoot* ast, const AstStatement* st, SSAFunction* out);
bool generate_ssa_expr(const AstRoot* ast, const AstExpression* expr, SSAValue* out_value, SSAFunction* out);

#endif
