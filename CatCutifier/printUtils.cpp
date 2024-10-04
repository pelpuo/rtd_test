#include "printUtils.h"

using namespace std;

namespace rail{

int32_t toSignedInt(uint32_t value) {
    // If the MSB is set, interpret the number as a negative value
    if (value & 0x80000000) {
        value -= 0x100000000; // Subtract 2^32 to convert to signed 32-bit integer
    }
    return static_cast<int32_t>(value);
}

Regs reg;
void PrintInst::printRType(RvInst r_type){
    outfile << InstNamesString[r_type.name] << " " << 
    reg.regnames[r_type.rd] << ", " <<  
    reg.regnames[r_type.rs1] << ", " << 
    reg.regnames[r_type.rs2] << "\n";
}

void PrintInst::printIType(RvInst i_type){
    if(i_type.opcode == LOAD){
        outfile << InstNamesString[i_type.name] << " " << 
        reg.regnames[i_type.rd] << ", " << dec <<
        i_type.imm << 
        "(" << reg.regnames[i_type.rs1] << ") " <<  "\n";
    }else if(i_type.opcode == SYSTEM){
        outfile << InstNamesString[i_type.name] << "\n";
    }else{
        outfile << InstNamesString[i_type.name] << ", " << 
        reg.regnames[i_type.rd] << ", " <<  
        reg.regnames[i_type.rs1] << ", " << dec << 
        i_type.imm << "\n";
    }
}

void PrintInst::printSType(RvInst s_type){
    outfile << InstNamesString[s_type.name] << " " << 
    reg.regnames[s_type.rs2] << ", " << dec <<
    s_type.imm <<
    "(" << reg.regnames[s_type.rs1] << ") " <<  "\n";
}

void PrintInst::printUType(RvInst u_type){
    outfile << InstNamesString[u_type.name] << " " << 
    reg.regnames[u_type.rd] << ", " <<  
    u_type.imm << "\n";
}

void PrintInst::printBType(RvInst b_type){
    outfile << InstNamesString[b_type.name] << " " << 
    reg.regnames[b_type.rs1] << ", " <<  
    reg.regnames[b_type.rs2] << ", " << 
    dec << b_type.imm << "\n";
}

void PrintInst::printJType(RvInst j_type){
    outfile << InstNamesString[j_type.name] << " " << 
    reg.regnames[j_type.rd] << ", " <<  dec <<
    j_type.imm << "\n";
}

void PrintInst::printInstruction(RvInst inst){
    switch(inst.type){
        case R_TYPE:
            printRType(inst);
            break;
        case I_TYPE:
            printIType(inst);
            break;
        case S_TYPE:
            printSType(inst);
            break;
        case U_TYPE:
            printUType(inst);
            break;
        case B_TYPE:
            printBType(inst);
            break;
        case J_TYPE:
            printJType(inst);
            break;
        default:
        outfile << "INVALID INSTRUCTION\n";
    }
}

void PrintInstHex::printRType(RvInst r_type){
    outfile << "R Type" << "\n";
    outfile 
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << r_type.opcode << "\n"
        << "Funct3: " << std::setw(3) << std::setfill('0') << std::hex << r_type.funct3 << "\n"
        << "Funct7: " << std::setw(7) << std::setfill('0') << std::hex << r_type.funct7 << "\n"
        << "Rd: " << std::setw(5) << std::setfill('0') << std::hex << r_type.rd << "\n"
        << "Rs1: " << std::setw(5) << std::setfill('0') << std::hex << r_type.rs1 << "\n"
        << "Rs2: " << std::setw(5) << std::setfill('0') << std::hex << r_type.rs2 << "\n";
};

void PrintInstHex::printIType(RvInst i_type){  
    outfile << "I Type" << "\n";

    outfile
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << i_type.opcode << "\n"
        << "Funct3: " << std::setw(3) << std::setfill('0') << std::hex << i_type.funct3 << "\n"
        << "Rd: " << std::setw(5) << std::setfill('0') << std::hex << i_type.rd << "\n"
        << "Imm: " << std::setw(12) << std::setfill('0') << std::hex << i_type.imm << "\n"
        << "Rs1: " << std::setw(5) << std::setfill('0') << std::hex << i_type.rs1 << "\n";
}

void PrintInstHex::printSType(RvInst s_type){
    outfile << "S Type" << "\n";

    outfile
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << s_type.opcode << "\n"
        << "Funct3: " << std::setw(3) << std::setfill('0') << std::hex << s_type.funct3 << "\n"
        << "Imm: " << std::setw(12) << std::setfill('0') << std::hex << s_type.imm << "\n"
        << "Rs1: " << std::setw(5) << std::setfill('0') << std::hex << s_type.rs1 << "\n"
        << "Rs2: " << std::setw(5) << std::setfill('0') << std::hex << s_type.rs2 << "\n";
}

void PrintInstHex::printUType(RvInst u_type){
    outfile << "U Type" << "\n";

    outfile
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << u_type.opcode << "\n"
        << "Rd: " << std::setw(5) << std::setfill('0') << std::hex << u_type.rd << "\n"
        << "Imm: " << std::setw(20) << std::setfill('0') << std::hex << u_type.imm << "\n";
}

void PrintInstHex::printBType(RvInst b_type){
    outfile << "B Type" << "\n";

    outfile
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << b_type.opcode << "\n"
        << "Funct3: " << std::setw(3) << std::setfill('0') << std::hex << b_type.funct3 << "\n"
        << "Imm: " << std::setw(12) << std::setfill('0') << std::hex << b_type.imm << "\n"
        << "Rs1: " << std::setw(5) << std::setfill('0') << std::hex << b_type.rs1 << "\n"
        << "Rs2: " << std::setw(5) << std::setfill('0') << std::hex << b_type.rs2 << "\n";
}

void PrintInstHex::printJType(RvInst j_type){
    outfile << "J Type" << "\n"; 

    outfile
        << "Opcode: " << std::setw(7) << std::setfill('0') << std::hex << j_type.opcode << "\n"
        << "Imm: " << std::setw(20) << std::setfill('0') << std::hex << j_type.imm << "\n"
        << "Rd: " << std::setw(5) << std::setfill('0') << std::hex << j_type.rd << "\n";
}

}