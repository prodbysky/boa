#ifndef PSEUDO_REG_H_
#define PSEUDO_REG_H_

#include "ssa.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PRT_MOV,
    PRT_ADD,
    PRT_SUB,
    PRT_MUL,
    PRT_DIV,
    PRT_RET,
} PseudoRegInstructionType;

typedef struct {
    PseudoRegInstructionType type;
    union {
        struct {
            TempValueIndex dest;
            SSAValue src;
        } mov;
        struct {
            SSAValue dest;
            SSAValue src_1;
            SSAValue src_2;
        } add, sub, mul, div;
        struct {
            bool with_value;
            SSAValue value;
        } ret;
    };
} PseudoRegInstruction;

typedef struct {
    PseudoRegInstruction* items;
    size_t count;
    size_t capacity;
} PseudoRegFunctionBody;

typedef struct {
    PseudoRegFunctionBody body;
    const char* name;
} PseudoRegFunction;

typedef struct {
    PseudoRegFunction* items;
    size_t count;
    size_t capacity;
} PseudoRegFunctions;

typedef struct {
    PseudoRegFunctions fs;
} PseudoRegModule;

bool pseudo_reg_convert_module(const SSAModule* ssa_mod, PseudoRegModule* out);

#endif
