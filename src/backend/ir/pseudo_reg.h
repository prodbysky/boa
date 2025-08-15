#ifndef PSEUDO_REG_H_
#define PSEUDO_REG_H_

#include "ssa.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t PseudoReg;

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
            PseudoReg dest;
            bool src_is_imm;
            union {
                PseudoReg src;
                uint64_t imm;
            };
        } mov;
        struct {
            PseudoReg dest;
            bool src_is_imm;
            union {
                PseudoReg src;
                uint64_t imm;
            };

        } add, sub, mul, div;
        struct {
            bool with_value;
            bool src_is_imm;
            union {
                PseudoReg src;
                uint64_t imm;
            };
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
    PseudoReg current_reg;
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

bool pseudo_reg_convert_module(const IRModule* ssa_mod, PseudoRegModule* out);



#endif
