#ifndef ENCODE_H
#define ENCODE_H

#include "instructions.h"
#include "instruction_names.h"
#include "instruction_types.h"
#include "decode.h"

namespace rail{
    class RvEncoder{
        public:
            RvEncoder();

            /**
             * Generic function to encode RISC-V instructions
             * @param inst RvInst object containing sub components of instruction
             * @param debug specifies whether or not encoded instruction should be printed
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_instruction(RvInst inst, bool debug);

            /**
             * Encodes a RISC-V instruction whose opcode is R-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Rtype(RvInst inst);

            uint32_t encode_Rtype(int opcode, int rd, int funct3, int rs1, int rs2, int funct7);

            /**
             * Encodes a RISC-V instruction whose opcode is I-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Itype(RvInst inst);
            uint32_t encode_Itype(int opcode, int rd, int funct3, int rs1, int imm);

            /**
             * Encodes a RISC-V instruction whose opcode is S-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Utype(RvInst inst);
            uint32_t encode_Utype(int opcode, int rd, int imm);

            /**
             * Encodes a RISC-V instruction whose opcode is U-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Stype(RvInst inst);
            uint32_t encode_Stype(int opcode, int funct3, int rs1, int rs2, int imm);

            /**
             * Encodes a RISC-V instruction whose opcode is B-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Btype(RvInst inst);
            uint32_t encode_Btype(int opcode, int rs1, int rs2, int funct3, int imm);

            /**
             * Encodes a RISC-V instruction whose opcode is J-Type
             * @param inst RvInst object containing sub components of instruction
             * @return uint32_t containing encoded instruction
            */
            uint32_t encode_Jtype(RvInst inst);
            uint32_t encode_Jtype(int opcode, int rd, int imm);

    };
}

#endif