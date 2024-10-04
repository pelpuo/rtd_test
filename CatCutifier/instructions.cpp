#include "instructions.h"

namespace rail{
        RvInst InstUtils::createRType(int opcode, int rd, int funct3, int rs1, int rs2, int funct7){
            RvInst inst;
            inst.type = R_TYPE;
            inst.opcode = opcode;
            inst.rd = rd;
            inst.funct3 = funct3;
            inst.rs1 = rs1;
            inst.rs2 = rs2;
            inst.funct7 = funct7;

            return inst;
        }

        RvInst InstUtils::createIType(int opcode, int rd, int funct3, int rs1, int imm){
            RvInst inst;
            inst.type = I_TYPE;
            inst.opcode = opcode;
            inst.rd = rd;
            inst.funct3 = funct3;
            inst.rs1 = rs1;
            inst.imm = imm;

            return inst;
        }

        RvInst InstUtils::createSType(int opcode, int funct3, int rs1, int rs2, int imm){
            RvInst inst;
            inst.type = S_TYPE;
            inst.opcode = opcode;
            inst.funct3 = funct3;
            inst.rs1 = rs1;
            inst.rs2 = rs2;
            inst.imm = imm;

            return inst;
        }
        
        RvInst InstUtils::createUType(int opcode, int rd, int imm){
            RvInst inst;
            inst.type = U_TYPE;
            inst.opcode = opcode;
            inst.rd = rd;
            inst.imm = imm;
            return inst;
        }

        RvInst InstUtils::createBType(int opcode, int rs1, int rs2, int funct3, int imm){
            RvInst inst;
            inst.type = B_TYPE;
            inst.opcode = opcode;
            inst.funct3 = funct3;
            inst.rs1 = rs1;
            inst.rs2 = rs2;
            inst.imm = imm;

            return inst;
        }

        RvInst InstUtils::createJType(int opcode, int rd, int imm){
            RvInst inst;
            inst.type = J_TYPE;
            inst.opcode = opcode;
            inst.rd = rd;
            inst.imm = imm;

            return inst;
        }
}