// #pragma once
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include "instruction_names.h"

namespace rail{

    enum InstType{
        R_TYPE,
        I_TYPE,
        S_TYPE,
        U_TYPE,
        B_TYPE,
        J_TYPE,
        SYSTEM_TYPE,
        MEM_ACCESS_TYPE,
        AMO_TYPE
    };

    typedef struct {
        int opcode;
        int rd;
        int funct3;
        int rs1;
        int rs2;
        int funct7;
        int imm;
        int address;

        InstType type;
        InstName name;
    } RvInst;


    typedef struct {
        int opcode;
        int rd;
        int funct3;
        int rs1;
        int rs2;
        int funct7;
    } Rtype;

    typedef struct {
        int opcode;
        int rd;
        int funct3;
        int rs1;
        int imm;
    } Itype;

    typedef struct {
        int opcode;
        int rd;
        int imm;
    } Utype;

    typedef struct {
        int opcode;
        int imm;
        int funct3;
        int rs1;
        int rs2;
        // int imm2;
    } Stype;

    typedef struct {
        int opcode;
        int imm;
        int funct3;
        int rs1;
        int rs2;
    } Btype;

    typedef struct {
        int opcode;
        int rd;
        int imm;
        // int imm2;
    } Jtype;

    class RvFunct{
        public:
            class R{
            public:
                // ARITHMETIC
                static const int ADD = 0x0;
                static const int SUB = 0x0;
                static const int SUB_FUNCT7 = 0x20;

                static const int SLL = 0x1;
                
                static const int SLT = 0x2;
                static const int SLTU = 0x3;

                static const int XOR = 0x4;

                static const int SRL = 0x5;
                static const int SRA = 0x5;
                static const int SRA_FUNCT7 = 0x20;

                static const int OR = 0x6;
                static const int AND = 0x7;

                // M EXTENSION
                static const int M_FUNCT7 = 0x01;

                static const int MUL = 0x00;
                static const int MULH = 0x01;
                static const int MULSU = 0x02;
                static const int MULU = 0x03;

                static const int DIV = 0x04;
                static const int DIVU = 0x05;

                static const int REM = 0x06;
                static const int REMU = 0x07;
                
                // M EXTENSION RV64
                static const int M64_FUNCT7 = 0x01;

                static const int MULW = 0x00;
                static const int DIVW = 0x04;
                static const int DIVUW = 0x05;
                static const int REMW = 0x06;
                static const int REMUW = 0x07;


                // OP 32
                static const int ADDW = 0x00;
                static const int SUBW = 0x00;
                static const int ADDW_FUNCT7 = 0x00;
                static const int SUBW_FUNCT7 = 0x20;

                static const int SLLW = 0x01;

                static const int SRLW = 0x05;
                static const int SRAW_FUNCT7 = 0x20;

                // AMO
                static const int RV32A_FUNCT3 = 0x2;
                static const int RV64A_FUNCT3 = 0x3;

                static const int LR_FUNCT7 = 0x2;
                static const int SC_FUNCT7 = 0x3;
                static const int AMOSWAP_FUNCT7 = 0x1;
                static const int AMOADD_FUNCT7 = 0x0;
                static const int AMOXOR_FUNCT7 = 0x4;
                static const int AMOAND_FUNCT7 = 0x12;
                static const int AMOOR_FUNCT7 = 0x8;
                static const int AMOMIN_FUNCT7 = 0x16;
                static const int AMOMAX_FUNCT7 = 0x20;
                static const int AMOMINU_FUNCT7 = 0x24;
                static const int AMOMAXU_FUNCT7 = 0x28;



            };

            class I{
            public:
                // IMMEDIATE
                static const int ADDI = 0x0;
                static const int SLLI = 0x1;
                static const int SLTI = 0x2;
                static const int SLTIU = 0x3;
                static const int XORI = 0x4;

                static const int SRLI = 0x5;
                static const int SRAI = 0x5;
                static const int SRAI_IMM_5to11 = 0x20;

                static const int ORI = 0x6;
                static const int ANDI = 0x7;

                // LOAD
                static const int LB = 0x0;
                static const int LH = 0x1;
                static const int LW = 0x2;
                static const int LBU = 0x4;
                static const int LHU = 0x5;
                
                // RV64I
                static const int LWU = 0x6;
                static const int LD = 0x3;

                static const int ADDIW = 0x0;
                static const int SLLIW = 0x1;
                static const int SRLIW = 0x5;
                static const int SRAIW_FUNCT_7 = 0x20;

                // MISC-MEM
                static const int FENCE = 0x20;
                // SYSTEM
                static const int ECALL_IMM = 0x0;
                static const int EBREAK_IMM = 0x1;

            };

            class S{
            public:
                static const int SB = 0x0;
                static const int SH = 0x1;
                static const int SW = 0x2;
                static const int SD = 0x3;
            };

            class U{
            public:

            };

            class B{
            public:
                static const int BEQ = 0x0;
                static const int BNE = 0x1;
                static const int BLT = 0x4;
                static const int BGE = 0x5;
                static const int BLTU = 0x6;
                static const int BGEU = 0x7;
            };

            class J{
            // public:
            };
    };


    class InstUtils{
        public:
            RvInst createRType(int opcode, int rd, int funct3, int rs1, int rs2, int funct7);
            RvInst createIType(int opcode, int rd, int funct3, int rs1, int imm);
            RvInst createSType(int opcode, int rd, int funct3, int rs1, int rs2);
            RvInst createUType(int opcode, int rd, int imm);
            RvInst createBType(int opcode, int rs1, int rs2, int funct3, int imm);
            RvInst createJType(int opcode, int rd, int imm);
    };


}

#endif