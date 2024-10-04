#ifndef DECODE_H
#define DECODE_H

#include "instructions.h"
#include "instruction_types.h"
#include "elf_reader.h"
#include "printUtils.h"

namespace rail{

    class RvDecoder{
        public:
            RvDecoder();

            uint32_t getOpcode(uint32_t instruction);

            InstType getType(uint32_t opcode);

            RvInst getOpType(uint32_t instruction);

            RvInst getOpTypeInst(uint32_t instruction);

            /**
             * Generic function to decode RISC-V instructions
             * @param instruction uint32_t which specifies the int value of the instruction
             * @param debug specifies whether or not decoded instruction should be printed
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_instruction(uint32_t instruction, bool debug);

            /**
             * @overload
             * Generic function to decode RISC-V instructions and include location of instruction in binary
             * @param instruction uint32_t which specifies the int value of the instruction
             * @param debug specifies whether or not decoded instruction should be printed
             * @param elfReader ElfReader object which has access to binary
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_instruction(uint32_t instruction, bool debug, ElfReader &elfReader);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Rtype(uint32_t instruction);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Itype(uint32_t instruction);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Utype(uint32_t instruction);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Stype(uint32_t instruction);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Btype(uint32_t instruction);

            /**
             * Decodes a RISC-V instruction whose opcode is under R-Type
             * @param instruction uint32_t which specifies the int value of the instruction
             * @return RvInst object containing decoded instructions parameters
            */
            RvInst decode_Jtype(uint32_t instruction);

            /**
             * Returns the name of the instruction as a string
             * @param instruction RvInst object containing the decoded instruction
             * @return string with instruction name
            */
            std::string getInstName(RvInst instruction);

    };

}
#endif