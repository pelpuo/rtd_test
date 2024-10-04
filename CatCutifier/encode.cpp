#include "encode.h"

namespace rail{
RvEncoder::RvEncoder(){}

uint32_t RvEncoder::encode_Rtype(RvInst inst){
    uint32_t encoded_inst = inst.opcode | 
                            (inst.rd << 7) | 
                            (inst.funct3 << 12) | 
                            (inst.rs1 << 15) | 
                            (inst.rs2 << 20) | 
                            (inst.funct7 << 25);
    return encoded_inst;
}

uint32_t RvEncoder::encode_Rtype(int opcode, int rd, int funct3, int rs1, int rs2, int funct7){
    uint32_t encoded_inst = opcode | 
                            (rd << 7) | 
                            (funct3 << 12) | 
                            (rs1 << 15) | 
                            (rs2 << 20) | 
                            (funct7 << 25);
    return encoded_inst;
}


uint32_t RvEncoder::encode_Itype(RvInst inst){
    uint32_t encoded_inst = inst.opcode | 
                            (inst.rd << 7) | 
                            (inst.funct3 << 12) | 
                            (inst.rs1 << 15) | 
                            (inst.imm << 20);
    return encoded_inst;
}

uint32_t RvEncoder::encode_Itype(int opcode, int rd, int funct3, int rs1, int imm){
    uint32_t encoded_inst = opcode | 
                            (rd << 7) | 
                            (funct3 << 12) | 
                            (rs1 << 15) | 
                            (imm << 20);
    return encoded_inst;
}


uint32_t RvEncoder::encode_Stype(RvInst inst){
    // Encode fields into instruction
    uint32_t instruction = 0;
    instruction |= (inst.opcode & 0x7f);
    instruction |= ((inst.funct3 & 0x7) << 12);
    instruction |= ((inst.rs1 & 0x1f) << 15);
    instruction |= ((inst.rs2 & 0x1f) << 20);

    // Extract the immediate field and split it into imm_0to4 and imm_5to11
    int imm_11_0 = (inst.imm & 0xfff);
    int imm_0to4 = imm_11_0 & 0x1f;
    int imm_5to11 = (imm_11_0 >> 5) & 0x7f;

    // Encode the immediate fields into instruction
    instruction |= (imm_0to4 << 7);
    instruction |= (imm_5to11 << 25);

    return instruction;
}

uint32_t RvEncoder::encode_Stype(int opcode, int funct3, int rs1, int rs2, int imm){
    // Encode fields into instruction
    uint32_t instruction = 0;
    instruction |= (opcode & 0x7f);
    instruction |= ((funct3 & 0x7) << 12);
    instruction |= ((rs1 & 0x1f) << 15);
    instruction |= ((rs2 & 0x1f) << 20);

    // Extract the immediate field and split it into imm_0to4 and imm_5to11
    int imm_11_0 = (imm & 0xfff);
    int imm_0to4 = imm_11_0 & 0x1f;
    int imm_5to11 = (imm_11_0 >> 5) & 0x7f;

    // Encode the immediate fields into instruction
    instruction |= (imm_0to4 << 7);
    instruction |= (imm_5to11 << 25);

    return instruction;
}


uint32_t RvEncoder::encode_Utype(RvInst inst){
    uint32_t encoded_inst = inst.opcode | 
                            (inst.rd << 7) | 
                            (inst.imm << 12);
    return encoded_inst;
}


uint32_t RvEncoder::encode_Btype(RvInst inst){
    uint32_t instruction = 0; // Initialize instruction to 0

    instruction |= (inst.opcode & 0x7F); // Extract and set opcode bits

    // Extract and set imm bits
    uint32_t imm = inst.imm;
    instruction |= ((imm >> 12) & 0x1) << 31; // imm_12
    instruction |= ((imm >> 11) & 0x1) << 7;  // imm_11
    instruction |= ((imm >> 5) & 0x3F) << 25; // imm_5to10
    instruction |= ((imm >> 1) & 0xF) << 8;    // imm_1to4

    // Extract and set funct3, rs1, and rs2 bits
    instruction |= ((inst.funct3 & 0x7) << 12);
    instruction |= ((inst.rs1 & 0x1F) << 15);
    instruction |= ((inst.rs2 & 0x1F) << 20);

    return instruction; // Return the encoded instruction
}

uint32_t RvEncoder::encode_Btype(int opcode, int rs1, int rs2, int funct3, int imm){
    uint32_t instruction = 0; // Initialize instruction to 0

    instruction |= (opcode & 0x7F); // Extract and set opcode bits

    // Extract and set imm bits
    instruction |= ((imm >> 12) & 0x1) << 31; // imm_12
    instruction |= ((imm >> 11) & 0x1) << 7;  // imm_11
    instruction |= ((imm >> 5) & 0x3F) << 25; // imm_5to10
    instruction |= ((imm >> 1) & 0xF) << 8;    // imm_1to4

    // Extract and set funct3, rs1, and rs2 bits
    instruction |= ((funct3 & 0x7) << 12);
    instruction |= ((rs1 & 0x1F) << 15);
    instruction |= ((rs2 & 0x1F) << 20);

    return instruction; // Return the encoded instruction
}

uint32_t RvEncoder::encode_Jtype(RvInst inst){
    uint32_t imm_x1 = (inst.imm & 0x7fe) >> 1;
    uint32_t imm_x2 = (inst.imm >> 11) & 0x1;
    uint32_t imm_x3 = (inst.imm >> 12) & 0xff;
    uint32_t imm_x4 = (inst.imm >> 20) & 0x1;

    uint32_t enc_imm = imm_x3 | (imm_x2 << 8) | (imm_x1 << 9) | (imm_x4 << 19); 
    uint32_t encoded_inst = inst.opcode | (inst.rd << 7) | (enc_imm << 12);

    return encoded_inst;
}

uint32_t RvEncoder::encode_Jtype(int opcode, int rd, int imm){
    uint32_t imm_x1 = (imm & 0x7fe) >> 1;
    uint32_t imm_x2 = (imm >> 11) & 0x1;
    uint32_t imm_x3 = (imm >> 12) & 0xff;
    uint32_t imm_x4 = (imm >> 20) & 0x1;

    uint32_t enc_imm = imm_x3 | (imm_x2 << 8) | (imm_x1 << 9) | (imm_x4 << 19); 
    uint32_t encoded_inst = opcode | (rd << 7) | (enc_imm << 12);

    return encoded_inst;
}


uint32_t RvEncoder::encode_instruction(RvInst inst, bool debug){
    int opcode = inst.opcode;

    uint32_t encodedInst;

    switch(opcode){
        case OP:
        case OP_32:
        case AMO:
            encodedInst = RvEncoder::encode_Rtype(inst);
            break;
        // encode I Type
        case LOAD:
        case JALR:
        case OPIMM:
        case OPIMM_32:
            encodedInst = RvEncoder::encode_Itype(inst);
            break;
        // encode S Type
        case STORE:
            encodedInst = RvEncoder::encode_Stype(inst);
            break;
        // encode U Type
        case AUIPC:
        case LUI:
            encodedInst = RvEncoder::encode_Utype(inst);
            break;
        // encode B Type
        case BRANCH:
            encodedInst = RvEncoder::encode_Btype(inst);
            break;
        // encode J Type
        case JAL:
            encodedInst = RvEncoder::encode_Jtype(inst);
            break;
        default:
            encodedInst = 0x00000013; // addi x0, x0, 0
    };
    if(debug){
        // PrintInst printer;
        // outs() << format_hex(inst, 8) << "\t";
        // printer.printinst(encodedInst);
    }

    return encodedInst;
}

}