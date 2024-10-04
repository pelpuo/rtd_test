#include "decode.h"

namespace rail{

RvDecoder::RvDecoder(){}

uint32_t RvDecoder::getOpcode(uint32_t instruction){
    return instruction & 0x7f;
}

RvInst RvDecoder::getOpType(uint32_t instruction){
    RvInst inst;
    inst.opcode = instruction & 0x7f;
    inst.type = getType(inst.opcode);
    return inst;
}

RvInst RvDecoder::decode_Rtype(uint32_t instruction){
    RvInst r_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    r_type.type = R_TYPE;

    // Decoding instruction  
    r_type.opcode = instruction & 0x7f;
    r_type.rd = instruction >> 7 & 0x1f;
    r_type.funct3 = instruction >> 12 & 0x7;
    r_type.rs1 = instruction >> 15 & 0x1f;
    r_type.rs2 = instruction >> 20 & 0x1f;
    r_type.funct7 = instruction >> 25;

    // Getting Inst name

    if(r_type.opcode == OP && r_type.funct7 == RvFunct::R::M_FUNCT7){ // MULTIPLY EXTENSION

        switch(r_type.funct3){
            // MULTIPLICATION
            case RvFunct::R::MUL:
                Inst = MUL_INST;
                break;
            // MULTIPLICATION HIGH
            case RvFunct::R::MULH:
                Inst = MULH_INST;
                break;
            // MULTIPLICATION HIGH SIGNED UPPER
            case RvFunct::R::MULSU:
                Inst = MULSU_INST;
                break;
            // MULTIPLICATION HIGH UPPER
            case RvFunct::R::MULU:
                Inst = MULU_INST;
                break;
            // DIVISION
            case RvFunct::R::DIV:
                Inst = DIV_INST;
                break;
            // DIVISION UNSIGNED
            case RvFunct::R::DIVU:
                Inst = DIVU_INST;
                break;
            // REMAINDER
            case RvFunct::R::REM:
                Inst = REM_INST;
                break;
            // REMAINDER UNSIGNED
            case RvFunct::R::REMU:
                Inst = REMU_INST;
                break;

            default:
                Inst = UNDEF_INST;
        }
    }else if(r_type.opcode == OP_32 && r_type.funct7 == RvFunct::R::M64_FUNCT7){ // MULTIPLY EXTENSION RV64

        switch(r_type.funct3){
            // MULTIPLICATION
            case RvFunct::R::MULW:
                Inst = MULW_INST;
                break;
            // MULTIPLICATION HIGH
            case RvFunct::R::DIVW:
                Inst = DIVW_INST;
                break;
            // MULTIPLICATION HIGH SIGNED UPPER
            case RvFunct::R::DIVUW:
                Inst = DIVUW_INST;
                break;
            // MULTIPLICATION HIGH UPPER
            case RvFunct::R::REMW:
                Inst = REMW_INST;
                break;
            // DIVISION
            case RvFunct::R::REMUW:
                Inst = REMUW_INST;
                break;
            default:
                Inst = UNDEF_INST;
        }
    }else if(r_type.opcode == OP){
        // Determining specific instruction based on funct3 and funct7

        switch(r_type.funct3){
            // ADD and SUB
            case RvFunct::R::ADD:
            // Difference Between ADD and SUB is funct7
                if(r_type.funct7 == RvFunct::R::SUB_FUNCT7){ // SUB
                    Inst = SUB_INST;
                }else{ // ADD
                    Inst= ADD_INST;
                }
                break;
            // XOR
            case RvFunct::R::XOR:
                Inst = XOR_INST;
                break;
            // OR
            case RvFunct::R::OR:
                Inst = OR_INST;
                break;
            // AND
            case RvFunct::R::AND:
                Inst = AND_INST;
                break;
            // Shift Left Logical
            case RvFunct::R::SLL:
                Inst = SLL_INST;
                break;
            // Shift Right Logical/Arithmetic
            case RvFunct::R::SRL:
                // Difference Between Logical and Arithmetic is funct7
                if(r_type.funct7 == RvFunct::R::SRA_FUNCT7){ // Arithmetic
                    Inst = SRA_INST;
                }else{ //Logical
                    Inst = SRL_INST;
                }
                break;
            // Set Less Than
            case RvFunct::R::SLT:
                Inst = SLT_INST;
                // registers[r_type.rd] = builder.CreateSet(registers[r_type.rs1] , registers[r_type.rs2] , ");
                break;
            // Set Less Than (U)
            case RvFunct::R::SLTU:
                Inst = SLTU_INST;
                break;
            // If no match, intruction is currently undefined
            default:
                Inst = UNDEF_INST;
                break;
        }

    }else if(r_type.opcode == OP_32){
        switch(r_type.funct3){
            case RvFunct::R::ADDW:
                if(r_type.funct7 == RvFunct::R::SUBW_FUNCT7){ // SUB
                    Inst = SUBW_INST;
                }else{ // ADD
                    Inst= ADDW_INST;
                }
                break;
            case RvFunct::R::SLLW:
                Inst= SLLW_INST;
                break;
            case RvFunct::R::SRLW:
                if(r_type.funct7 == RvFunct::R::SRAW_FUNCT7){ // Arithmetic
                    Inst = SRAW_INST;
                }else{ //Logical
                    Inst = SRLW_INST;
                }
                break;
            default:
                Inst = UNDEF_INST;
                break;
        }
    }else if(r_type.opcode == AMO){
        int adjusted_funct7 = r_type.funct7 >> 2;
        switch(r_type.funct3){
            case RvFunct::R::RV32A_FUNCT3:
                switch(adjusted_funct7){
                    case RvFunct::R::LR_FUNCT7:
                        Inst = LRW_INST;
                        break;
                    case RvFunct::R::SC_FUNCT7:
                        Inst = SCW_INST;
                        break;
                    case RvFunct::R::AMOSWAP_FUNCT7:
                        Inst = AMOSWAPW_INST;
                        break;
                    case RvFunct::R::AMOADD_FUNCT7:
                        Inst = AMOADDW_INST;
                        break;
                    case RvFunct::R::AMOXOR_FUNCT7:
                        Inst = AMOXORW_INST;
                        break;
                    case RvFunct::R::AMOAND_FUNCT7:
                        Inst = AMOANDW_INST;
                        break;
                    case RvFunct::R::AMOOR_FUNCT7:
                        Inst = AMOORW_INST;
                        break;
                    case RvFunct::R::AMOMIN_FUNCT7:
                        Inst = AMOMINW_INST;
                        break;
                    case RvFunct::R::AMOMAX_FUNCT7:
                        Inst = AMOMAXW_INST;
                        break;
                    case RvFunct::R::AMOMINU_FUNCT7:
                        Inst = AMOMINUW_INST;
                        break;
                    case RvFunct::R::AMOMAXU_FUNCT7:
                        Inst = AMOMAXUW_INST;
                        break;
                    default:
                        Inst = UNDEF_INST;
                    break;
                }
                break;
            case RvFunct::R::RV64A_FUNCT3:
                switch(r_type.funct7){
                    case RvFunct::R::LR_FUNCT7:
                        Inst = LRD_INST;
                        break;
                    case RvFunct::R::SC_FUNCT7:
                        Inst = SCD_INST;
                        break;
                    case RvFunct::R::AMOSWAP_FUNCT7:
                        Inst = AMOSWAPD_INST;
                        break;
                    case RvFunct::R::AMOADD_FUNCT7:
                        Inst = AMOADDD_INST;
                        break;
                    case RvFunct::R::AMOXOR_FUNCT7:
                        Inst = AMOXORD_INST;
                        break;
                    case RvFunct::R::AMOAND_FUNCT7:
                        Inst = AMOANDD_INST;
                        break;
                    case RvFunct::R::AMOOR_FUNCT7:
                        Inst = AMOORD_INST;
                        break;
                    case RvFunct::R::AMOMIN_FUNCT7:
                        Inst = AMOMIND_INST;
                        break;
                    case RvFunct::R::AMOMAX_FUNCT7:
                        Inst = AMOMAXD_INST;
                        break;
                    case RvFunct::R::AMOMINU_FUNCT7:
                        Inst = AMOMINUD_INST;
                        break;
                    case RvFunct::R::AMOMAXU_FUNCT7:
                        Inst = AMOMAXUD_INST;
                        break;
    
                    default:
                        Inst = UNDEF_INST;
                    break;
                }
                break;
            default:
                Inst = UNDEF_INST;
                break;
        }
    }

    r_type.name = Inst;

    return r_type;

}

RvInst RvDecoder::decode_Itype(uint32_t instruction){
    RvInst i_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    // Decoding instruction
    i_type.opcode = instruction & 0x7f;
    i_type.rd = instruction >> 7 & 0x1f;
    i_type.funct3 = instruction >> 12 & 0x7;
    i_type.rs1 = instruction >> 15 & 0x1f;
    
    int32_t imm_11_0 = (instruction >> 20) & 0xfff;
    i_type.imm = (imm_11_0 & 0x800) ? (imm_11_0 | 0xFFFFF000) : imm_11_0;

    i_type.type = I_TYPE;

    // Determining Inst name
    if(i_type.opcode == LOAD){
        // allocate(i_type.rd, context, builder);
        switch(i_type.funct3){
        case RvFunct::I::LB:
            Inst= LB_INST;
            break;
        case RvFunct::I::LH:
            Inst= LH_INST;
            break;
        case RvFunct::I::LW:
            Inst= LW_INST;
            break;
        case RvFunct::I::LBU:
            Inst= LBU_INST;
            break;
        case RvFunct::I::LHU:
            Inst= LHU_INST;
            break;
        case RvFunct::I::LWU:
            Inst= LWU_INST;
            break;
        case RvFunct::I::LD:
            Inst= LD_INST;
            break;
        default:
            Inst = UNDEF_INST;
            break;
        }
    }else if(i_type.opcode == JALR){
       Inst= JALR_INST;
    }else if(i_type.opcode == OPIMM){
        switch(i_type.funct3){
            // ADD Immediate
            case RvFunct::I::ADDI:
                Inst= ADDI_INST;
                break;
            // Shift Left Logical Immediate
            case RvFunct::I::SLLI:
                Inst = SLLI_INST;
                break;
            // Set Less Than Immediate
            case RvFunct::I::SLTI:
                Inst = SLTI_INST;
                break;
            // Set Less Than Immediate (U)
            case RvFunct::I::SLTIU:
                Inst = SLTIU_INST;
                break;
            // XOR Immediate
            case RvFunct::I::XORI:
                Inst = XORI_INST;
                break;
            // Shift Right Logical Immediate
            case RvFunct::I::SRLI:
                if((i_type.imm >> 5 & 0x3f) == RvFunct::I::SRAI_IMM_5to11){
                    Inst = SRAI_INST;
                }else{
                    Inst = SRLI_INST;
                }
                break;
            // OR Immediate
            case RvFunct::I::ORI:
                Inst = ORI_INST;
                break;
            // AND Immediate
            case RvFunct::I::ANDI:
                Inst = ANDI_INST;
                break;
            // If no match, intruction is currently undefined
            default:
                Inst = UNDEF_INST;
                break;
        }

    }else if(i_type.opcode == OPIMM_32){
        switch(i_type.funct3){
            case RvFunct::I::ADDIW:
                Inst = ADDIW_INST;
                break;
            case RvFunct::I::SLLIW:
                Inst = SLLIW_INST;
                break;
            case RvFunct::I::SRLIW:
                if(i_type.funct7 == RvFunct::I::SRAIW_FUNCT_7){
                    Inst = SRAIW_INST;
                }else{
                    Inst = SRLIW_INST;
                }
                break;
            default:
                Inst = UNDEF_INST;
                break;
        }
    }else if(i_type.opcode == SYSTEM){
        switch(i_type.imm){
            case RvFunct::I::ECALL_IMM:
                Inst = ECALL_INST;
                break;
            case RvFunct::I::EBREAK_IMM:
                Inst = EBREAK_INST;
                break;
            default:
                Inst = UNDEF_INST;
                break;
        }
    }else if(i_type.opcode == MISC_MEM){
        Inst = FENCE_INST;
    }

    i_type.name = Inst;

    return i_type;

}

RvInst RvDecoder::decode_Utype(uint32_t instruction){
    RvInst u_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    // Decoding instruction
    u_type.opcode = instruction & 0x7f;
    u_type.rd = instruction >> 7 & 0x1f;
    u_type.imm = instruction >> 12;

    u_type.type = U_TYPE;


    if(u_type.opcode == LUI){
       Inst = LUI_INST; 
    }else if(u_type.opcode == AUIPC){
       Inst = AUIPC_INST;
    }

    u_type.name = Inst;

    return u_type;
}

RvInst RvDecoder::decode_Stype(uint32_t instruction){
    RvInst s_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    // Instruction Decoding
    s_type.opcode = instruction & 0x7f;
    
    int imm_0to4 = instruction >> 7 & 0x1f;
    
    s_type.funct3 = instruction >> 12 & 0x7;
    s_type.rs1 = instruction >> 15 & 0x1f;
    s_type.rs2 = instruction >> 20 & 0x1f;
    
    int imm_5to11 = instruction >> 25;

    int32_t imm_11_0 = (imm_5to11 << 5 | imm_0to4)&0xfff;
    s_type.imm = (imm_11_0 & 0x800) ? (imm_11_0 | 0xFFFFF000) : imm_11_0;    

    s_type.type = S_TYPE;

        switch(s_type.funct3){
        // Store Byte
        case RvFunct::S::SB:
            Inst = SB_INST;
            break; 
        // Store Half
        case RvFunct::S::SH:
            Inst = SH_INST;

            break;
        // Store Word
        case RvFunct::S::SW:
            Inst = SW_INST;
            break;
        case RvFunct::S::SD:
            Inst = SD_INST;
            break;
        // If no match, intruction is currently undefined 
        default:
            Inst = UNDEF_INST;
            break;
    }

    s_type.name = Inst;

    return s_type;
}

RvInst RvDecoder::decode_Btype(uint32_t instruction){
    RvInst b_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    b_type.opcode = instruction & 0x7f;

    int imm_11 = instruction >> 7 & 0x1;
    int imm_1to4 = instruction >> 8 & 0xf;
    
    b_type.funct3 = instruction >> 12 & 0x7;
    b_type.rs1 = instruction >> 15 & 0x1f;
    b_type.rs2 = instruction >> 20 & 0x1f;
    
    int imm_5to10 = instruction >> 25 & 0x3f;
    int imm_12 = instruction >> 31;

    // b_type.imm = ((((((imm_12 << 1) | imm_11) << 6) | imm_5to10) << 4) | imm_1to4) << 1; 
    // Constructing the immediate value with proper sign extension
    b_type.imm = (imm_12 << 12) | (imm_11 << 11) | (imm_5to10 << 5) | (imm_1to4 << 1);

    // Sign extend the immediate value for negative numbers
    if (imm_12) {
        b_type.imm |= 0xFFFFF000; // Sign extension for negative numbers
    }

    b_type.type = B_TYPE;

    switch(b_type.funct3){
        case RvFunct::B::BEQ:
            Inst = BEQ_INST;
            break; 
        case RvFunct::B::BNE:
            Inst = BNE_INST;
            break;
        case RvFunct::B::BLT:
            Inst = BLT_INST;
            break; 
        case RvFunct::B::BGE:
            Inst = BGE_INST;
            break; 
        case RvFunct::B::BLTU:
            Inst = BLTU_INST;
            break; 
        case RvFunct::B::BGEU:
            Inst = BGEU_INST;
            break; 
        default:
            Inst = UNDEF_INST;
            break;
    }

    b_type.name = Inst;

    return b_type;
}

RvInst RvDecoder::decode_Jtype(uint32_t instruction){
    RvInst j_type; // Object to store decoded R type
    InstName Inst; // String name for operation

    // outs() << "THE J TYPE INSTRUCTION " << format_hex(instruction, 8) << "\n";

    j_type.opcode = instruction & 0x7f;
    j_type.rd = instruction >> 7 & 0x1f;
    
    int imm_12to19 = (instruction >> 12) & 0xff;
    int imm_11 = (instruction >> 20) & 0x1;
    int imm_1to10 = (instruction >> 21) & 0x3ff;
    int imm_20 = (instruction >> 31) & 0x1;

    j_type.imm = (imm_20 << 20) | (imm_12to19 << 12) | (imm_11 << 11) | (imm_1to10 << 1);

    // Sign extend the immediate value for negative numbers
    if (imm_20) {
        j_type.imm |= 0xFFF00000; // Sign extension for negative numbers
    }

    j_type.type = J_TYPE;

    Inst = JAL_INST;

    j_type.name = Inst;

    return j_type;
}



RvInst RvDecoder::decode_instruction(uint32_t instruction, bool debug){
    int opcode = instruction & 0x7f;

    RvInst decodedInst;

    switch(opcode){
        case OP:
        case OP_32:
        case AMO:
            decodedInst = RvDecoder::decode_Rtype(instruction);
            break;
        // Decode I Type
        case OPIMM:
        case OPIMM_32:
        case JALR:
        case LOAD:
        case MISC_MEM:
        case SYSTEM:
            decodedInst = RvDecoder::decode_Itype(instruction);
            break;
        // Decode S Type
        case STORE:
            decodedInst = RvDecoder::decode_Stype(instruction);
            break;
        // Decode U Type
        case AUIPC:
        case LUI:
            decodedInst = RvDecoder::decode_Utype(instruction);
            break;
        // Decode B Type
        case BRANCH:
            decodedInst = RvDecoder::decode_Btype(instruction);
            break;
        // Decode J Type
        case JAL:
            decodedInst = RvDecoder::decode_Jtype(instruction);
            break;
        default:
            decodedInst.name = UNDEF_INST;
            decodedInst.type = R_TYPE;
            decodedInst.funct3 = 0;
            decodedInst.funct7 = 0;
            decodedInst.imm = 0;
            decodedInst.rd = 0;
            decodedInst.rs1 = 0;
            decodedInst.rs2 = 0;
            decodedInst.opcode = 0;
    };
    if(debug){
        PrintInst printer;
        // outs() << format_hex(instruction, 8) << "\t";
        printer.printInstruction(decodedInst);
    }

    return decodedInst;
}

RvInst RvDecoder::decode_instruction(uint32_t instruction, bool debug, ElfReader &elfReader){
    int opcode = instruction & 0x7f;

    RvInst decodedInst;

    switch(opcode){
        // Decode R Type
        case OP:
        case OP_32:
        case AMO:
            decodedInst = RvDecoder::decode_Rtype(instruction);
            break;
        // Decode I Type
        case OPIMM:
        case OPIMM_32:
        case JALR:
        case LOAD:
        case MISC_MEM:
        case SYSTEM:
            decodedInst = RvDecoder::decode_Itype(instruction);
            break;
        // Decode S Type
        case STORE:
            decodedInst = RvDecoder::decode_Stype(instruction);
            break;
        // Decode U Type
        case AUIPC:
        case LUI:
            decodedInst = RvDecoder::decode_Utype(instruction);
            break;
        // Decode B Type
        case BRANCH:
            decodedInst = RvDecoder::decode_Btype(instruction);
            break;
        // Decode J Type
        case JAL:
            decodedInst = RvDecoder::decode_Jtype(instruction);
            break;
        default:
            decodedInst.name = UNDEF_INST;
            decodedInst.type = R_TYPE;
            decodedInst.funct3 = 0;
            decodedInst.funct7 = 0;
            decodedInst.imm = 0;
            decodedInst.rd = 0;
            decodedInst.rs1 = 0;
            decodedInst.rs2 = 0;
            decodedInst.opcode = 0;
    };
    if(debug){
        PrintInst printer;
        // outs() << format_hex(instruction, 8) << "\t";
        printer.printInstruction(decodedInst);
    }

    decodedInst.address = elfReader.getProgramCounter();

    return decodedInst;
}


InstType RvDecoder::getType(uint32_t opcode){
    switch(opcode){
        // Decode R Type
        case OP:
        case OP_32:
        case AMO:
            return InstType::R_TYPE;
        // Decode I Type
        case OPIMM:
        case OPIMM_32:
        case JALR:
        case LOAD:
        case MISC_MEM:
        case SYSTEM:
            return InstType::I_TYPE;
            break;
        // Decode S Type
        case STORE:
            return InstType::S_TYPE;
        // Decode U Type
        case AUIPC:
        case LUI:
            return InstType::U_TYPE;
        // Decode B Type
        case BRANCH:
            return InstType::B_TYPE;
        // Decode J Type
        case JAL:
            return InstType::J_TYPE;
        default:
            return R_TYPE;
    };
}

}