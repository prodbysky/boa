#ifndef SSA_H_
#define SSA_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../frontend/parser.h"

typedef enum {
    SSAVT_CONST,
    SSAVT_TEMP,
    SSAVT_ARG,
    SSAVT_STRING,
} SSAValueType;

typedef uint64_t ConstValue;
typedef uint64_t TempValueIndex;

typedef struct {
    SSAValueType type;
    union {
        ConstValue constant;
        TempValueIndex temp;
        // strings will be in the data section like this
        // str%zu... db ..., 0
        uint64_t string_index, arg_index;
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
    SSAST_JMP,
    SSAST_ASM,
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
            SSAValue place;
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
        uint64_t jmp, label;
        StringView asm;
    };
} SSAStatement;

typedef struct {
    SSAStatement* items;
    size_t count;
    size_t capacity;
} SSAFunctionBody;

typedef struct {
    StringView name;
    SSAValue value;
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
bool add_variable(SSABadBoyStack* stack, NameValuePair pair, Arena* arena);

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
    StringView* items;
    size_t count;
    size_t capacity;
} SSAStrings;

typedef struct {
    SSAFunctions functions;
    SSAStrings strings;
} SSAModule;

void dump_ir(const SSAModule* mod);

bool generate_ssa_module(const AstRoot *ast, SSAModule *out, Arena* arena);
bool generate_ssa_function(SSAFunction* out, const AstFunction* ast_func, Arena* arena, SSAStrings* strs, const AstRoot* tree);
bool generate_ssa_statement(const AstRoot *tree, const AstStatement *st, SSAFunction *out, SSAStrings* strs, Arena* arena);
bool generate_ssa_expr(const AstRoot *tree, const AstExpression *expr, SSAValue *out_value, SSAFunction *out, SSAStrings* strs, Arena* arena);

#endif
