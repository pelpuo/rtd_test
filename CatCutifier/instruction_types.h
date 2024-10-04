#ifndef INSTRUCTION_TYPES_H
#define INSTRUCTION_TYPES_H

namespace rail{
// INSTRUCTION TYPES: BEGINS
// R Type
#define OP          0b0110011
#define OP_32       0b0111011
#define AMO         0b0101111

// I Type
#define OPIMM       0b0010011
#define OPIMM_32    0b0011011
#define JALR        0b1100111
#define LOAD        0b0000011
#define MISC_MEM    0b0001111
#define SYSTEM      0b1110011

// S Type
#define STORE       0b0100011

// U Type
#define AUIPC       0b0010111
#define LUI         0b0110111

// B Type
#define BRANCH      0b1100011

// J Type
#define JAL         0b1101111
// INSTRUCTION TYPES: ENDS

}

#endif