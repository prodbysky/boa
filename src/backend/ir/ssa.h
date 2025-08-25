#ifndef SSA_H_
#define SSA_H_

#include "../../frontend/parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VT_CONST,
    VT_TEMP,
    VT_ARG,
    VT_STRING,
} ValueType;

typedef uint64_t ConstValue;
typedef uint64_t TempValueIndex;

typedef struct {
    ValueType type;
    union {
        ConstValue constant;
        TempValueIndex temp;
        // strings will be in the data section like this
        // str%zu... db ..., 0
        uint64_t string_index, arg_index;
    };
} Value;

typedef enum {
    ST_RETURN,
    ST_RETURN_EMPTY,
    ST_ADD,
    ST_SUB,
    ST_MUL,
    ST_DIV,
    ST_CMP_MT,
    ST_CMP_LT,
    ST_CMP_MTE,
    ST_CMP_LTE,
    ST_CMP_EQ,
    ST_CMP_NEQ,
    ST_ASSIGN,
    ST_CALL,
    ST_LABEL,
    ST_JZ,
    ST_JMP,
    ST_ASM,
} StatementType;

typedef struct {
    Value *items;
    size_t count;
    size_t capacity;
} InputArgs;

typedef struct {
    StatementType type;
    union {
        struct {
            Value value;
        } ret;
        struct {
            Value l;
            Value r;
            Value result;
        } binop;
        struct {
            Value place;
            Value value;
        } assign;
        struct {
            StringView name;
            bool returns;
            Value return_v;
            InputArgs args;
        } call;
        struct {
            Value cond;
            uint64_t to;
        } jz;
        uint64_t jmp, label;
        StringView asm;
    };
} Statement;

typedef struct {
    Statement *items;
    size_t count;
    size_t capacity;
} FunctionBody;

typedef struct {
    StringView name;
    Value value;
} Sym;

typedef struct {
    Sym *items;
    size_t count;
    size_t capacity;
} SymTable;

typedef struct {
    SymTable *items;
    size_t count;
    size_t capacity;
} ScopeStack;

void push_scope(ScopeStack *stack, Arena *arena);
void pop_scope(ScopeStack *stack);
bool define_sym(ScopeStack *stack, StringView name, Value value, Arena *arena);
bool lookup_sym(ScopeStack *stack, StringView name, Sym **out_value);

typedef struct {
    StringView name;
    size_t arg_count;
    FunctionBody body;
    ScopeStack scopes;
    size_t max_temps;
    uint64_t label_count;
} Function;

typedef struct {
    Function *items;
    size_t count;
    size_t capacity;
} Functions;

typedef struct {
    StringView *items;
    size_t count;
    size_t capacity;
} StringPool;

typedef struct {
    Functions functions;
    StringPool strings;
} Module;

void dump_ir(const Module *mod);

bool generate_module(const AstRoot *ast, Module *out, Arena *arena);
bool generate_function(Function *out, const AstFunction *ast_func, Arena *arena, StringPool *strs,
                           const AstRoot *tree);
bool generate_statement(const AstRoot *tree, const AstStatement *st, Function *out, StringPool *strs, Arena *arena);
bool generate_expr(const AstRoot *tree, const AstExpression *expr, Value *out_value, Function *out,
                       StringPool *strs, Arena *arena);

#endif
