#ifndef PRINTUTILS_H
#define PRINTUTILS_H

#include "instructions.h"
#include "instruction_types.h"
#include "logger.h"
#include "regfile.h"
#include <iostream>
#include <iomanip>

namespace rail{
    class PrintInst{
        public:

            /**
             * Generic function to print decoded RISC-V instructions
             * @param inst RvInst object containing the instruction
            */
            static void printInstruction(RvInst inst);


            /**
             * Prints a RISC-V instruction whose opcode is under R-Type
             * @param inst RvInst object containing the instruction
            */
            static void printRType(RvInst r_type);

            /**
             * Prints a RISC-V instruction whose opcode is under I-Type
             * @param inst RvInst object containing the instruction
            */
            static void printIType(RvInst i_type);

            /**
             * Prints a RISC-V instruction whose opcode is under S-Type
             * @param inst RvInst object containing the instruction
            */
            static void printSType(RvInst s_type);

            /**
             * Prints a RISC-V instruction whose opcode is under U-Type
             * @param inst RvInst object containing the instruction
            */
            static void printUType(RvInst u_type);

            /**
             * Prints a RISC-V instruction whose opcode is under B-Type
             * @param inst RvInst object containing the instruction
            */
            static void printBType(RvInst b_type);

            /**
             * Prints a RISC-V instruction whose opcode is under J-Type
             * @param inst RvInst object containing the instruction
            */
            static void printJType(RvInst j_type);
    };

    class PrintInstHex{
        public:
            /**
             * Prints hexadecimal values for each component of an R-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printRType(RvInst r_type);

            /**
             * Prints hexadecimal values for each component of an I-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printIType(RvInst i_type);

            /**
             * Prints hexadecimal values for each component of an S-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printSType(RvInst s_type);

            /**
             * Prints hexadecimal values for each component of an U-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printUType(RvInst u_type);

            /**
             * Prints hexadecimal values for each component of an B-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printBType(RvInst b_type);

            /**
             * Prints hexadecimal values for each component of an J-Type Instruction
             * @param inst RvInst object containing the instruction
            */
            static void printJType(RvInst j_type);
    };

}

#endif