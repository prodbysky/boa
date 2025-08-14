#ifndef IR_H_
#define IR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../frontend/parser.h"

typedef enum {
    IRVT_CONST,
    IRVT_TEMP,
} IRValueType;

typedef uint64_t ConstValue;
typedef uint64_t TempValueIndex;

typedef struct {
    IRValueType type;
    union {
        ConstValue constant;
        TempValueIndex temp;
    };
} IRValue;

// IRS lol
typedef enum {
    IRST_RETURN,
    IRST_RETURN_EMPTY,
    IRST_ADD,
    IRST_SUB,
    IRST_MUL,
    IRST_DIV,
    IRST_ASSIGN,
} IRStatementType;

typedef struct {
    IRStatementType type;
    union {
        struct {
            IRValue value;
        } ret;
        struct {
            IRValue l;
            IRValue r;
            IRValue result;
        } binop;
        struct {
            TempValueIndex place;
            IRValue value;
        } assign;
    };
} IRStatement;

typedef struct {
    IRStatement* items;
    size_t count;
    size_t capacity;
} IRFunctionBody;

typedef struct {
    StringView name;
    TempValueIndex index;
} NameValuePair;

typedef struct {
    NameValuePair* items;
    size_t count;
    size_t capacity;
} IRNameToValue;

typedef struct {
    const char* name;
    IRFunctionBody body;
    size_t max_temps;
    IRNameToValue variables;
} IRFunction;

typedef struct {
    IRFunction* items;
    size_t count;
    size_t capacity;
} IRFunctions;

typedef struct {
    IRFunctions functions;
} IRModule;

bool generate_ir_module(const AstTree* ast, IRModule* out);
bool generate_ir_statement(const AstTree* tree, const AstStatement* st, IRFunction* out);
bool generate_ir_expr(const AstTree* tree, const AstExpression* expr, IRValue* out_value, IRFunction* out);

#endif
