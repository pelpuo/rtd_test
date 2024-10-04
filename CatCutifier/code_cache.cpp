#include "code_cache.h"
#include "rail.h"

#define CACHE_SIZE 4*1024*1024
#define DATA_SIZE 1*1024*1024
#define MAX_STUB_DISTANCE 900 // - len of stub_taken 

namespace rail{

    int CodeCache::dataOffset = 0x8fee9d4;
    int* CodeCache::memory;
    char* CodeCache::data;
    // uint32_t CodeCache::jalr_inst = 0x000f8067;

    int CodeCache::memoryIndex = 0;
    unordered_map<uint64_t, rail::RailBasicBlock> CodeCache::RailBasicBlocks;
    uint32_t CodeCache::currentBB = 0;
    
    long int CodeCache::nearestStubRegion = 0;
    int CodeCache::stubDistanceTracker = 0;
    int CodeCache::numStubRegions = 0;

    int CodeCache::countVar = 0;
    
    unordered_map<uint64_t, uint64_t> CodeCache::exits;


    int CodeCache::allocateRoot(ElfReader &elfReader){
        elfReader.jumpToEntry();
        
        insertLi(reinterpret_cast<uint64_t>(&init_switch), regs::T6);
        memory[memoryIndex++] = 0x000f8067; //jalr x0, 0(t6)
        
        #ifdef STUBREGIONS
            insertStubRegion(true);
        #endif

        return 0;
    }


    int CodeCache::allocateBB(ElfReader &elfReader, uint64_t binaryAddress, bool resume){
        uint32_t jalr_inst = 0x000f8067;
        bool print_inst = false;
        int allocatedPrev = 0;

        #ifdef DEBUG
            print_inst = true;
            outfile << endl << hex << "Allocating block at addr: " << binaryAddress << endl;
        #endif

        RvDecoder decoder;
        RvEncoder encoder;
        elfReader.setProgramCounter(binaryAddress);
        
        uint32_t nextInst = elfReader.getNextInstruction();

        rail::RailBasicBlock railBB;
        railBB.firstAddr = elfReader.getProgramCounter();
        railBB.basicBlockAddress = elfReader.getProgramCounter();
        railBB.startInst = nextInst;
        railBB.numInstructions = 0;
        CodeCache::setCurrentBB(railBB.firstAddr);
        railBB.startLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);

        // Before every routine starts, we must restore the value of T6 that we used to jump to the block
        insertRoutine((char *)&restore_scratch, 1);

        bool shouldInstrumentOnInst;

        // RvInst decodedInst;
        uint8_t opcode;

        while(nextInst){
            shouldInstrumentOnInst = true;

            if(elfReader.getProgramCounter() < Rail::instructionRoutinesStart &&
                elfReader.getProgramCounter() > Rail::instructionRoutinesEnd){
                    shouldInstrumentOnInst = false;
                }
            
            #ifdef DEBUG
            outfile << "\t";
            #endif
            railBB.numInstructions++;
            // RvInst decodedInst = decoder.decode_instruction(nextInst, print_inst);
            // RvInst decodedInst;
            opcode = nextInst & 0x7f;

            // decodedInst.address = elfReader.getProgramCounter();

            /* 
                If we need to insert a pre-instrumentation routine then the context switch 
                instructions should be placed before the instruction is inserted
            */
            // cout << hex << elfReader.getProgramCounter() << "  " << elfReader.getNextInstruction() << endl;

            if(Rail::preInstrumentationRoutines){
                    RvInst decodedInst;
                    decodedInst.type = decoder.getType(opcode);
                    
                    
                    if(( (Rail::instructionRoutinesTypedPre[(InstType)(decodedInst.type)].size() > 0)
                    || ((opcode == LOAD || opcode == STORE) &&  Rail::instructionRoutinesTypedPre[MEM_ACCESS_TYPE].size() > 0))
                    && shouldInstrumentOnInst){
                    #ifdef DEBUG
                    outfile << "PRE ROUTINE EXISTS...ALLOCATING AS SINGLE BLOCK " << endl;
                    #endif
                    // cout << decodedInst.type << " : " << Rail::instructionRoutinesTypedPre[(InstType)(decodedInst.type)].size() << endl;
                    
                    // decoder.decode_instruction(elfReader.getNextInstruction(), true);

                    if(allocatedPrev){
                        railBB.type = BasicBlockType::SEGMENTED;
                        elfReader.decProgramCounter();
                        railBB.lastAddr = elfReader.getProgramCounter();
                        railBB.terminalInst = elfReader.getNextInstruction();
                        elfReader.incProgramCounter();

                        insertRoutine((char *)&inline_save, 2);
                        insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6);
                        memory[memoryIndex++] = jalr_inst;

                        #ifdef STUBREGIONS
                            stubDistanceTracker++;
                        #endif

                        // memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                        // stubDistanceTracker++;
                        // cout << "asadasdsadsafdafds\n";

                        railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                        insertBB(railBB);
                    }
                    /*
                        End first BB here
                        Allocate new BB for pre-inst instruction
                        New BB should start with jump to instrument, execution of inst and jump to context switch
                        After allocating, switch currentBB back to this BB
                    */

                    // cout << hex << elfReader.getProgramCounter() << "  " << elfReader.getNextInstruction() << endl;

                    if(CodeCache::RailBasicBlocks.count(elfReader.getProgramCounter())){
                        return 1;
                    }



                    rail::RailBasicBlock preInstBB;
                    preInstBB.firstAddr = elfReader.getProgramCounter();
                    // preInstBB.lastAddr = elfReader.getProgramCounter();
                    preInstBB.startInst = elfReader.getNextInstruction();
                    preInstBB.terminalInst = elfReader.getNextInstruction();
                    preInstBB.basicBlockAddress = railBB.basicBlockAddress;

                    preInstBB.startLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_instrument), regs::T6);
                    memory[memoryIndex++] = jalr_inst;

                    #ifdef STUBREGIONS
                        stubDistanceTracker++;
                    #endif

                    allocateNextInst(elfReader, preInstBB);
                    // cout << "Allocated\n";
                    
                    preInstBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(preInstBB); 

                    #ifdef STUBREGIONS
                        insertStubRegion(false);
                    #endif

                    return 1;
                }
            }

            

            // If branch instruction has been seen
            // if(decodedInst.opcode == BRANCH){
            if(opcode == BRANCH){
                #ifdef DEBUG
                outfile << "Block with BRANCH detected" <<endl;
                #endif
                
                railBB.type = BasicBlockType::DIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();


                // BRANCH TAKEN
                #ifdef STUBREGIONS
                    RvInst beqInst = decoder.decode_Btype(nextInst);
                    beqInst.imm = (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24;
                    // cout << dec << beqInst.imm << "   BRANCH TAKEN IMMEDIATE  " << memoryIndex << "  " << &memory[memoryIndex] << endl;
                    memory[memoryIndex++] = encoder.encode_Btype(beqInst);  // 1 inst

                    // BRANCH NOT TAKEN
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                    stubDistanceTracker+=2;
                    insertStubRegion(false); 
                #else
                    insertRoutine((char *)&inline_save, 2); // 2 insts

                    RvInst decodedInst = decoder.decode_Btype(nextInst);
                    decodedInst.imm = 20;
                    uint32_t encodedBranch = encoder.encode_Btype(decodedInst);
                    memory[memoryIndex++] = encodedBranch;  // 1 inst

                    // BRANCH NOT TAKEN
                    insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6); // 3 insts
                    memory[memoryIndex++] = jalr_inst; // 1 inst
                    // BRANCH TAKEN
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6); // 3 insts
                    memory[memoryIndex++] = jalr_inst; // 1 insts
                #endif

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                insertBB(railBB); 

                return 1;
            }

            // If function call or jump instruction has been seen (Also a direct Branch)
            // else if(decodedInst.opcode == JAL){
            else if(opcode == JAL){

                railBB.type = BasicBlockType::DIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();
                
                #ifdef DEBUG
                outfile << "Terminal Instruction for Direct Branch: ";
                decoder.decode_instruction(railBB.terminalInst, print_inst);
                #endif

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;
                    insertStubRegion(false);
                #else
                    // Saving T6 and ra, we will modify these for our jump
                    insertRoutine((char *)&inline_save, 2); 
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    memory[memoryIndex++] = jalr_inst;
                #endif

                railBB.endLocationInCache= reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                insertBB(railBB);

                // cout << "JAL AT " << memoryIndex -1 << " " << hex << elfReader.getProgramCounter() << " " << &memory[memoryIndex] << endl;

                return 1;
            }
            // decoder.decode_instruction(nextInst, true);
            // Indirect jump -> JALR rd imm(rs1)
            // else if(decodedInst.opcode == JALR){
            else if(opcode == JALR){
                railBB.type = BasicBlockType::INDIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();
                
                #ifdef DEBUG
                outfile << "Terminal Instruction for Indirect Branch: ";
                decoder.decode_instruction(railBB.terminalInst, print_inst);
                #endif


                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;

                    insertStubRegion(false);
                #else
                    // Saving t6 and ra, we will modify these for our jump
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    // stubDistanceTracker++;
                #endif

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                insertBB(railBB);

                return 1;
            }

            // FIX FOR AUIPC INSTRUCTIONS :: BEGINS
            // else if(decodedInst.opcode == AUIPC){
            else if(opcode == AUIPC){
                
                // decodedInst = decoder.decode_Utype(nextInst); //Replace with getImm
                RvInst decodedInst = decoder.decode_Utype(nextInst); //Replace with getImm

                uint64_t addr = elfReader.getProgramCounter() + (decodedInst.imm << 12);
                Regs reg;

                #ifdef DEBUG
                    outfile << "\tREPLACING AUIPC " << "Adjusted PC Value is " << addr << " and rd is " << reg.regnames[decodedInst.rd] << " Immediate is " << decodedInst.imm << endl;
                #endif
                insertLi(addr, (regs)(decodedInst.rd));    

                if((Rail::instructionRoutinesPre.size() > 0 || 
                Rail::instructionRoutinesPost.size() > 0 || 
                Rail::instructionRoutinesTypedPost[U_TYPE].size() > 0) 
                && shouldInstrumentOnInst){

                    railBB.type = BasicBlockType::SEGMENTED;
                    railBB.lastAddr = elfReader.getProgramCounter();
                    railBB.terminalInst = nextInst;
                    
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;

                    railBB.endLocationInCache= reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(railBB);
                    
                    #ifdef STUBREGIONS
                        insertStubRegion(false);
                    #endif

                    return 1;
                }

                elfReader.incProgramCounter();
                nextInst = elfReader.getNextInstruction();
                allocatedPrev++;

                continue;
            }
            // FIX FOR AUIPC INSTRUCTIONS :: ENDS      

            // else if(decodedInst.opcode == STORE || decodedInst.opcode == LOAD){
            else if(opcode == STORE || opcode == LOAD){
            // else if(opcode == STORE || opcode == LOAD || opcode == AMO){

                // decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                RvInst decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                bool lrsc = false;
                // if(decodedInst.opcode == AMO && (decodedInst.funct7 == 12 || decodedInst.funct7 == 8)){
                //     cout << "LR or SC seen" << endl;
                //     lrsc = true;
                // }
                if(decodedInst.rs1 != regs::SP){

                    regs scratch_reg = (regs)(decodedInst.rs1) == regs::T6? regs::T5: regs::T6;
                    // save scratch to stack
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_save, 2);
                    else insertRoutine((char *)&inline_save_T5, 2);

                    // li t6, dataOffset
                    insertLi(dataOffset, scratch_reg);

                    // blt LW/SW_rs1, t6, 12
                    memory[memoryIndex++] = encoder.encode_Btype(BRANCH, decodedInst.rs1, scratch_reg, RvFunct::B::BLT, 12);

                    // sw/lw rd, x(rs1)
                    memory[memoryIndex++] = nextInst;

                    // jal x0, 12
                    memory[memoryIndex++] = 0x00c0006f; 

                    // add t6, rs1, t6
                    memory[memoryIndex++] = encoder.encode_Rtype(OP, scratch_reg, RvFunct::R::ADD, decodedInst.rs1, scratch_reg, 0);

                    // add load/store
                    decodedInst.rs1 = scratch_reg;
                    memory[memoryIndex++] = encoder.encode_instruction(decodedInst, true);

                    // restore scratch
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_load, 2);
                    else insertRoutine((char *)&inline_load_T5, 2);

                    stubDistanceTracker+=6;

                }else{
                    CodeCache::memory[memoryIndex++] = nextInst;
                }

                if((Rail::instructionRoutinesPre.size() > 0 || 
                    Rail::instructionRoutinesPost.size() > 0 || 
                    Rail::instructionRoutinesTypedPost[MEM_ACCESS_TYPE].size() > 0 ||
                    Rail::instructionRoutinesTypedPost[InstType::I_TYPE].size() > 0 ||
                    Rail::instructionRoutinesTypedPost[InstType::S_TYPE].size() > 0
                    )
                    && shouldInstrumentOnInst){

                    railBB.type = BasicBlockType::SEGMENTED;
                    railBB.lastAddr = elfReader.getProgramCounter();
                    railBB.terminalInst = nextInst;
                    
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;
                    // memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    // stubDistanceTracker++;

                    railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(railBB);
                    
                    #ifdef STUBREGIONS
                        insertStubRegion(false); 
                    #endif
                    
                    return 1;
                }

                elfReader.incProgramCounter();
                nextInst = elfReader.getNextInstruction(); 
                allocatedPrev++;
                continue;
                

                // cout << "MARCO \n";


                // cout << "POLO\n";
                
            }else if(opcode == SYSTEM){
                // If the ECALL is at the beginning of the BB then we will add this ECALL to the current block 
                // and terminate the BB at the next ECALL if any

                railBB.type = BasicBlockType::SYSCALL;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();

                #ifdef DEBUG
                    outfile << "SYSCALL INSTRUCTION DETECTED" << endl;
                #endif

                // Terminate basic block at syscall if the syscall ends the block
                // insertRoutine((char *)&inline_save, 2);
                // insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6);
                // // uint32_t jalr_inst = 0x000f8067;
                // CodeCache::memory[memoryIndex++] = jalr_inst;

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                    stubDistanceTracker++;
                    insertStubRegion(false);
                #else
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6);
                    CodeCache::memory[memoryIndex++] = jalr_inst;
                #endif

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                CodeCache::memory[memoryIndex++] = nextInst;
                // stubDistanceTracker+=2;

                elfReader.incProgramCounter();
                insertBB(railBB);
                
                return 1;

            }
            else if(opcode == AMO){
                RvInst decodedInst = decoder.decode_instruction(nextInst, false);

                uint32_t newInst = nextInst;
                int functVal;

                switch(decodedInst.funct7 >> 2){
                    // LR
                    case RvFunct::R::LR_FUNCT7:
                        functVal = decodedInst.funct3 == 0x2?RvFunct::I::LW:RvFunct::I::LD;
                        newInst = encoder.encode_Itype(OP, decodedInst.rd, functVal, decodedInst.rs1, 0);
                    break;
                    // SC
                    case RvFunct::R::SC_FUNCT7:
                        functVal = decodedInst.funct3 == 0x2?RvFunct::S::SW:RvFunct::S::SD;
                        newInst = encoder.encode_Stype(OP, functVal, decodedInst.rs1, decodedInst.rs2, 0);
                    break;
                    default:
                    break;
                }

                RvInst newDecodedInst = decoder.decode_instruction(newInst, false);

                if(decodedInst.rs1 != regs::SP){

                    regs scratch_reg = (regs)(newDecodedInst.rs1) == regs::T6? regs::T5: regs::T6;
                    // save scratch to stack
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_save, 2);
                    else insertRoutine((char *)&inline_save_T5, 2);
                    // li t6, dataOffset
                    insertLi(dataOffset, scratch_reg);
                    // blt LW/SW_rs1, t6, 16
                    memory[memoryIndex++] = encoder.encode_Btype(BRANCH, newDecodedInst.rs1, scratch_reg, RvFunct::B::BLT, 12);
                    // sw/lw rd, x(rs1)
                    memory[memoryIndex++] = newInst;
                    // jal x0, 16
                    memory[memoryIndex++] = 0x0100006f; 
                    // add rs1, rs1, t6
                    memory[memoryIndex++] = encoder.encode_Rtype(OP, newDecodedInst.rs1, RvFunct::R::ADD, newDecodedInst.rs1, scratch_reg, 0);
                    // add load/store
                    memory[memoryIndex++] = encoder.encode_instruction(newDecodedInst, false);
                    // sub rs1, rs1, t6
                    memory[memoryIndex++] = encoder.encode_Rtype(OP, newDecodedInst.rs1, RvFunct::R::SUB, newDecodedInst.rs1, scratch_reg, RvFunct::R::SUB_FUNCT7);
                    // restore scratch
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_load, 2);
                    else insertRoutine((char *)&inline_load_T5, 2);
                    stubDistanceTracker+=6;

                }else{
                    CodeCache::memory[memoryIndex++] = newInst;
                }

                if(decodedInst.funct7 >> 2 == 0x3){
                    // addi rd, zero, 0
                    CodeCache::memory[memoryIndex++] = encoder.encode_Itype(OP, decodedInst.rd, RvFunct::I::ADDI, regs::ZERO, 0);
                }

                elfReader.incProgramCounter();
                nextInst = elfReader.getNextInstruction(); 
                allocatedPrev++;
                continue;
            }
            
            // if((Rail::instructionRoutinesPost.size() > 0  || Rail::instructionRoutinesPre.size() > 0 ||
            // Rail::instructionRoutinesTypedPost[(InstType)(decodedInst.type)].size() > 0) && shouldInstrumentOnInst){
            if((Rail::instructionRoutinesPost.size() > 0  || Rail::instructionRoutinesPre.size() > 0 ||
            Rail::instructionRoutinesTypedPost[(InstType)(decoder.getType(opcode))].size() > 0) && shouldInstrumentOnInst){

                railBB.type = BasicBlockType::SEGMENTED;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = nextInst;
                CodeCache::memory[memoryIndex++] = nextInst;
                
                insertRoutine((char *)&inline_save, 2);
                insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                // uint32_t jalr_inst = 0x000f8067;
                memory[memoryIndex++] = jalr_inst;
                stubDistanceTracker+=2;

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                insertBB(railBB);

                #ifdef STUBREGIONS
                    insertStubRegion(false);
                #endif

                return 1;
            }

            CodeCache::memory[memoryIndex++] = nextInst;
            
            #ifdef STUBREGIONS
                stubDistanceTracker++;
            #endif
            
            elfReader.incProgramCounter();
            nextInst = elfReader.getNextInstruction();   
            allocatedPrev++;

        }

        return 1;
    }

    void CodeCache::insertRoutine(char * routine, int length){
        for(int k=0; k<length;k++) {
            CodeCache::memory[memoryIndex++] = *(int *)(routine + k*4);
        }
        
        #ifdef STUBREGIONS
            stubDistanceTracker+= length;
        #endif
    }


    uint32_t CodeCache::getCurrentBB(){
        return currentBB;
    }

    void CodeCache::setCurrentBB(uint32_t bbAddr){
        currentBB = bbAddr;
    }

    void CodeCache::insertBB(rail::RailBasicBlock railBB){
        RailBasicBlocks[railBB.firstAddr] = railBB;
    }

    rail::RailBasicBlock CodeCache::retrieveBB(uint64_t address){
        return RailBasicBlocks[address];
    }

    void CodeCache::insertLi(uint64_t value, regs reg){
        uint64_t addr = reinterpret_cast<uint64_t>(value);

        uint32_t addr_lsb = addr & 0xfff;
        addr_lsb = addr_lsb >> 1;
        uint32_t addr_msb = (addr >> 12) & 0xfffff;

        uint32_t lui_inst = LUI | (reg << 7) | (addr_msb << 12);
        // outfile << "LUI INST: " << hex << lui_inst << endl; 
        memory[memoryIndex++] = lui_inst;

        uint32_t addi_inst = OPIMM | (reg << 7) | (RvFunct::I::ADDI << 12) | (reg << 15) | (addr_lsb << 20);
        // outfile << "ADDI INST: " << hex << addi_inst << endl; 
        memory[memoryIndex++] = addi_inst;
        memory[memoryIndex++] = addi_inst;

        #ifdef STUBREGIONS
            stubDistanceTracker+= 3;
        #endif
    }




    int CodeCache::allocateNextInst(ElfReader &elfReader, rail::RailBasicBlock &railBB){
        uint32_t jalr_inst = 0x000f8067;
        bool print_inst = false;

        #ifdef DEBUG
            print_inst = true;
            outfile << endl << hex << "Allocating single instruction addr: " << railBB.firstAddr << endl;
            outfile << "\t";
        #endif

        // cout << "ANI\n";
        RvDecoder decoder;
        RvEncoder encoder;
        
        uint32_t nextInst = elfReader.getNextInstruction();

        // RvInst decodedInst = decoder.decode_instruction(nextInst, print_inst);
        uint8_t opcode = nextInst & 0x7f;
        // decodedInst.address = elfReader.getProgramCounter();

            // If branch instruction has been seen
            if(opcode == BRANCH){
                #ifdef DEBUG
                outfile << "Block with BRANCH detected" <<endl;
                #endif
                
                railBB.type = BasicBlockType::DIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = nextInst;

                // BRANCH TAKEN
                #ifdef STUBREGIONS
                    RvInst beqInst = decoder.decode_Btype(nextInst);
                    beqInst.imm = (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24;
                    // cout << dec << beqInst.imm << "   BRANCH TAKEN IMMEDIATE  " << memoryIndex << "  " << &memory[memoryIndex] << endl;
                    memory[memoryIndex++] = encoder.encode_Btype(beqInst);  // 1 inst

                    // BRANCH NOT TAKEN
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                    stubDistanceTracker+=2;
                #else
                    insertRoutine((char *)&inline_save, 2); // 2 insts

                    RvInst decodedInst = decoder.decode_Btype(nextInst);
                    decodedInst.imm = 20;
                    uint32_t encodedBranch = encoder.encode_Btype(decodedInst);
                    memory[memoryIndex++] = encodedBranch;  // 1 inst

                    // BRANCH NOT TAKEN
                    insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6); // 3 insts
                    memory[memoryIndex++] = jalr_inst; // 1 inst
                    // BRANCH TAKEN
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6); // 3 insts
                    memory[memoryIndex++] = jalr_inst; // 1 insts
                #endif

                // cout << "B\n";

                return 1;
            }

            // If function call or jump instruction has been seen (Also a direct Branch)
            else if(opcode == JAL){

                railBB.type = BasicBlockType::DIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();
                
                #ifdef DEBUG
                outfile << "Terminal Instruction for Direct Branch: ";
                decoder.decode_instruction(railBB.terminalInst, print_inst);
                #endif

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;
                #else
                    // Saving T6 and ra, we will modify these for our jump
                    insertRoutine((char *)&inline_save, 2); 
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                #endif

                return 1;
            }
            // decoder.decode_instruction(nextInst, true);
            // Indirect jump -> JALR rd imm(rs1)
            else if(opcode == JALR){
                railBB.type = BasicBlockType::INDIRECT_BRANCH;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();
                
                #ifdef DEBUG
                outfile << "Terminal Instruction for Indirect Branch: ";
                decoder.decode_instruction(railBB.terminalInst, print_inst);
                #endif

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;
                #else
                    // Saving t6 and ra, we will modify these for our jump
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;
                #endif

                return 1;
            }

            // FIX FOR AUIPC INSTRUCTIONS :: BEGINS
            else if(opcode == AUIPC){
                RvInst decodedInst = decoder.decode_Utype(nextInst);
                uint64_t addr = elfReader.getProgramCounter() + (decodedInst.imm << 12);
                Regs reg;

                #ifdef DEBUG
                    outfile << "\tREPLACING AUIPC " << "Adjusted PC Value is " << addr << " and rd is " << reg.regnames[decodedInst.rd] << endl;
                #endif
                insertLi(addr, (regs)(decodedInst.rd));    

                railBB.type = BasicBlockType::SEGMENTED;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();
                    
                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;
                #else
                    // Saving t6 and ra, we will modify these for our jump
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;
                #endif


                railBB.endLocationInCache= reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                // cout << "AUIPC\n";
                return 1;

            }
            // FIX FOR AUIPC INSTRUCTIONS :: ENDS      

            // Adjusting addresses for loads and stores:
            else if(opcode == STORE || opcode == LOAD){

                // decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                RvInst decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                if(decodedInst.rs1 != regs::SP){

                    
                    // If rd is an offset from the stack pointer then we must not adjust
                    // blt LW/SW_rs2, sp, 12

                    regs scratch_reg = (regs)(decodedInst.rs1) == regs::T6? regs::T5: regs::T6;

                    #ifdef DEBUG
                        outfile << "\tLoad/Store seen...adjusting rs1 value" << endl;
                    #endif

                    // save scratch to stack
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_save, 2);
                    else insertRoutine((char *)&inline_save_T5, 2);

                    // li t6, dataOffset
                    insertLi(dataOffset, scratch_reg);

                    // blt LW/SW_rs1, t6, 12
                    RvInst blt_inst;
                    blt_inst.type = B_TYPE;
                    blt_inst.opcode = BRANCH;
                    blt_inst.funct3 = RvFunct::B::BLT;
                    blt_inst.rs1 = decodedInst.rs1;
                    // blt_inst.rs2 = regs::SP;
                    blt_inst.rs2 = scratch_reg;
                    blt_inst.imm = 12;
                    memory[memoryIndex++] = encoder.encode_Btype(blt_inst);

                    // If rd is within our data section then we must not adjust
                    // blt, LW/SW_rs2 t6, 12

                    // sw/lw rd, x(rs1)
                    memory[memoryIndex++] = nextInst;

                    // jal x0, 16
                    memory[memoryIndex++] = 0x0100006f; 

                    // add rs1, rs1, t6
                    RvInst adjustmentInst;
                    adjustmentInst.opcode = OP;
                    adjustmentInst.funct3 = RvFunct::R::ADD;
                    adjustmentInst.funct7 = 0;
                    adjustmentInst.rd = decodedInst.rs1;
                    adjustmentInst.rs1 = decodedInst.rs1;
                    adjustmentInst.rs2 = scratch_reg;
                    memory[memoryIndex++] = encoder.encode_Rtype(adjustmentInst);

                    // add load/store
                    memory[memoryIndex++] = nextInst;

                    // sub rs1, rs1, t6
                    if(((decodedInst.type == I_TYPE) && (decodedInst.rs1 != decodedInst.rd))||
                    ((decodedInst.type == S_TYPE) && (decodedInst.rs1 != decodedInst.rs2))){
                        adjustmentInst.funct7 = RvFunct::R::SUB_FUNCT7;
                        memory[memoryIndex++] = encoder.encode_Rtype(adjustmentInst);
                    }else{
                    memory[memoryIndex++] = 0x00000013;
                    }

                    stubDistanceTracker+=6;

                    // restore scratch
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_load, 2);
                    else insertRoutine((char *)&inline_load_T5, 2);


                    railBB.type = BasicBlockType::SEGMENTED;
                    railBB.lastAddr = elfReader.getProgramCounter();
                    railBB.terminalInst = nextInst;
                    
                    #ifdef STUBREGIONS
                        memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                        stubDistanceTracker++;
                    #else
                        insertRoutine((char *)&inline_save, 2);
                        insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), scratch_reg);
                        // uint32_t jalr_inst = 0x000f8067;
                        memory[memoryIndex++] = jalr_inst;
                    #endif

                    return 1;
                } 

            }else if(opcode == SYSTEM){
                // If the ECALL is at the beginning of the BB then we will add this ECALL to the current block 
                // and terminate the BB at the next ECALL if any

                railBB.type = BasicBlockType::SYSCALL;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = elfReader.getNextInstruction();

                #ifdef DEBUG
                    outfile << "SYSCALL INSTRUCTION DETECTED" << endl;
                #endif

                memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  

                stubDistanceTracker++;

                CodeCache::memory[memoryIndex++] = nextInst;

                return 1;

            }else{

                railBB.type = BasicBlockType::SEGMENTED;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = nextInst;
                CodeCache::memory[memoryIndex++] = nextInst;
                
                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;
                #else
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                #endif

                return 1;
            }


            railBB.type = BasicBlockType::SEGMENTED;
            railBB.lastAddr = elfReader.getProgramCounter();
            railBB.terminalInst = nextInst;
            CodeCache::memory[memoryIndex++] = nextInst;
            
            #ifdef STUBREGIONS
                memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                stubDistanceTracker++;
            #else
                insertRoutine((char *)&inline_save, 2);
                insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                // uint32_t jalr_inst = 0x000f8067;
                memory[memoryIndex++] = jalr_inst;
            #endif
            
            return 1;

    }


    void CodeCache::insertStubRegion(bool override){
        // cout << &memory[memoryIndex] << "   " << stubDistanceTracker << "  " << MAX_STUB_DISTANCE - stubDistanceTracker << endl;
        if((MAX_STUB_DISTANCE - stubDistanceTracker <= 0) || override){
            // cout << "Inserting Stub Region at " << &memory[memoryIndex] << " and mem index " << memoryIndex << endl;
            // Mark region as stub region
            nearestStubRegion = (long int)(&memory[memoryIndex]);

            // FOR NOT TAKEN
            // save T6 = 2 insts
            insertRoutine((char *)&inline_save, 2);
            // Insert Li for Taken (3 insts)
            insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6);
            // jump to T6 (1 inst)
            memory[memoryIndex++] = 0x000f8067;

            // FOR TAKEN
            // save T6 = 2 insts
            insertRoutine((char *)&inline_save, 2);
            // Insert Li for Taken (3 insts)
            insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
            // jump to T6
            memory[memoryIndex++] = 0x000f8067;

            stubDistanceTracker = 6;
            numStubRegions++;
        }
    }



    int CodeCache::allocateTrace(ElfReader &elfReader, uint64_t binaryAddress, int count){
        uint32_t nextInst = elfReader.getNextInstruction();
        elfReader.setProgramCounter(binaryAddress);

        RailBasicBlock railBB;
        railBB.firstAddr = elfReader.getProgramCounter();
        railBB.basicBlockAddress = elfReader.getProgramCounter();
        railBB.startInst = nextInst;
        railBB.numInstructions = 0;
        CodeCache::setCurrentBB(railBB.firstAddr);
        railBB.startLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);

        insertRoutine((char *)&restore_scratch, 1);

        for(int i=count; i>0; i--){
            int nextAddr = allocateTraceBlock(elfReader, binaryAddress, railBB, i-1);
            if(nextAddr == 0)break;
            binaryAddress = nextAddr;
        }

        railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
        railBB.lastAddr = elfReader.getProgramCounter();
        railBB.terminalInst = elfReader.getNextInstruction();
        insertBB(railBB);


        return 0;
    }

    uint64_t CodeCache::allocateTraceBlock(ElfReader &elfReader, uint64_t binaryAddress, RailBasicBlock &railBB, int count){
        uint32_t jalr_inst = 0x000f8067;
        bool print_inst = false;
        int allocatedPrev = 0;

        #ifdef DEBUG
            print_inst = true;
            outfile << endl << hex << "Allocating trace at addr: " << binaryAddress << endl;
        #endif

        RvDecoder decoder;
        RvEncoder encoder;
        elfReader.setProgramCounter(binaryAddress);
        
        uint32_t nextInst = elfReader.getNextInstruction();
        
        RailBasicBlock localRailBB;
        localRailBB.firstAddr = elfReader.getProgramCounter();
        localRailBB.basicBlockAddress = elfReader.getProgramCounter();
        localRailBB.startInst = nextInst;
        localRailBB.numInstructions = 0;
        // CodeCache::setCurrentBB(railBB.firstAddr);
        localRailBB.startLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);

        // Before every routine starts, we must restore the value of T6 that we used to jump to the block
        // insertRoutine((char *)&restore_scratch, 1);

        bool shouldInstrumentOnInst;

        // RvInst decodedInst;
        uint8_t opcode;

        while(nextInst){
            shouldInstrumentOnInst = true;

            if(elfReader.getProgramCounter() < Rail::instructionRoutinesStart &&
                elfReader.getProgramCounter() > Rail::instructionRoutinesEnd){
                    shouldInstrumentOnInst = false;
                }
            
            #ifdef DEBUG
            outfile << "\t";
            #endif
            railBB.numInstructions++;
            // RvInst decodedInst = decoder.decode_instruction(nextInst, print_inst);
            // RvInst decodedInst;
            opcode = nextInst & 0x7f;

            if(0){
            // if(Rail::preInstrumentationRoutines){
                    RvInst decodedInst;
                    decodedInst.type = decoder.getType(opcode);
                    
                    
                    if(( (Rail::instructionRoutinesTypedPre[(InstType)(decodedInst.type)].size() > 0)
                    || ((opcode == LOAD || opcode == STORE) &&  Rail::instructionRoutinesTypedPre[MEM_ACCESS_TYPE].size() > 0))
                    && shouldInstrumentOnInst){
                    #ifdef DEBUG
                    outfile << "PRE ROUTINE EXISTS...ALLOCATING AS SINGLE BLOCK " << endl;
                    #endif
                    // cout << decodedInst.type << " : " << Rail::instructionRoutinesTypedPre[(InstType)(decodedInst.type)].size() << endl;
                    
                    // decoder.decode_instruction(elfReader.getNextInstruction(), true);

                    if(allocatedPrev){
                        railBB.type = BasicBlockType::SEGMENTED;
                        elfReader.decProgramCounter();
                        railBB.lastAddr = elfReader.getProgramCounter();
                        railBB.terminalInst = elfReader.getNextInstruction();
                        elfReader.incProgramCounter();

                        insertRoutine((char *)&inline_save, 2);
                        insertLi(reinterpret_cast<uint64_t>(&context_switch), regs::T6);
                        memory[memoryIndex++] = jalr_inst;

                        #ifdef STUBREGIONS
                            stubDistanceTracker++;
                        #endif

                        // memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                        // stubDistanceTracker++;
                        // cout << "asadasdsadsafdafds\n";

                        railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                        insertBB(railBB);
                    }
                    /*
                        End first BB here
                        Allocate new BB for pre-inst instruction
                        New BB should start with jump to instrument, execution of inst and jump to context switch
                        After allocating, switch currentBB back to this BB
                    */

                    // cout << hex << elfReader.getProgramCounter() << "  " << elfReader.getNextInstruction() << endl;

                    if(CodeCache::RailBasicBlocks.count(elfReader.getProgramCounter())){
                        return 0;
                    }



                    rail::RailBasicBlock preInstBB;
                    preInstBB.firstAddr = elfReader.getProgramCounter();
                    // preInstBB.lastAddr = elfReader.getProgramCounter();
                    preInstBB.startInst = elfReader.getNextInstruction();
                    preInstBB.terminalInst = elfReader.getNextInstruction();
                    preInstBB.basicBlockAddress = railBB.basicBlockAddress;

                    preInstBB.startLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_instrument), regs::T6);
                    memory[memoryIndex++] = jalr_inst;

                    #ifdef STUBREGIONS
                        stubDistanceTracker++;
                    #endif

                    allocateNextInst(elfReader, preInstBB);
                    // cout << "Allocated\n";
                    
                    preInstBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(preInstBB); 

                    #ifdef STUBREGIONS
                        insertStubRegion(false);
                    #endif

                    return 0;
                }
            }

            

            // If branch instruction has been seen
            if(opcode == BRANCH){
                #ifdef DEBUG
                outfile << "Block with BRANCH detected" <<endl;
                #endif
                
                railBB.type = BasicBlockType::DIRECT_BRANCH;

                // BRANCH TAKEN
                RvInst decodedInst = decoder.decode_Btype(nextInst);

                // if(count > 0){
                // if(0){
                //     int retval = elfReader.getProgramCounter() + decodedInst.imm;

                //         // ADD NOT TAKEN BRANCHES TO TRACE
                //         if(0){
                //             // Skip jump instruction
                //             decodedInst.imm = 8;
                //             uint32_t encodedBranch = encoder.encode_Btype(decodedInst);
                //             memory[memoryIndex++] = encodedBranch; 
                //             // Skip jump to stub
                //             memory[memoryIndex++] = 0x02c0006f; // jal x0, 44

                //             // For fallthrough, go back to stub
                //             insertRoutine((char *)&inline_save, 2); // 2 insts
                //             railBB.bbExits[elfReader.getProgramCounter()] = memoryIndex-2;
                //             //Place address of branch instruction onto stack
                //             insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                //             memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                //             insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit), regs::T6); // 3 insts
                //             memory[memoryIndex++] = jalr_inst; // 1 inst

                //             return elfReader.getProgramCounter() + 4;
                        
                //         // ADD TAKEN BRANCHES TO TRACE
                //         }else{
                //             decodedInst.imm = 44;
                //             uint32_t encodedBranch = encoder.encode_Btype(decodedInst);
                //             memory[memoryIndex++] = encodedBranch; 
                //             // For fallthrough, go back to stub
                //             insertRoutine((char *)&inline_save, 2); // 2 insts
                //         //Place address of branch instruction onto stack
                //             insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                //             memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                //             insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit), regs::T6); // 3 insts
                //             memory[memoryIndex++] = jalr_inst; // 1 inst

                //             return retval;
                //         }
                // }else{
                    // cout << "Count at branch not taken is: " << count << endl; 
                    #ifdef STUBREGIONS
                        RvInst beqInst = decoder.decode_Btype(nextInst);
                        beqInst.imm = (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24;
                        // cout << dec << beqInst.imm << "   BRANCH TAKEN IMMEDIATE  " << memoryIndex << "  " << &memory[memoryIndex] << endl;
                        memory[memoryIndex++] = encoder.encode_Btype(beqInst);  // 1 inst

                        // BRANCH NOT TAKEN
                        memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                        stubDistanceTracker+=2;
                        insertStubRegion(false); 
                    #else
                        // // INLINE ROUTINE BEGINS
                        // // insertRoutine((char *)&inline_save, 2); // 2 insts
                        // memory[memoryIndex++] = encoder.encode_Itype(OPIMM, regs::SP, RvFunct::I::ADDI, regs::SP, -16);
                        // memory[memoryIndex++] = encoder.encode_Stype(STORE, RvFunct::S::SD, regs::SP, regs::T6, 0);
                        // memory[memoryIndex++] = encoder.encode_Stype(STORE, RvFunct::S::SD, regs::SP, regs::T5, 8);
                        // insertLi(reinterpret_cast<uint64_t>(&countVar), regs::T6);
                        // memory[memoryIndex++] = encoder.encode_Itype(LOAD, regs::T5, RvFunct::I::LD, regs::T6, 0);
                        // memory[memoryIndex++] = encoder.encode_Itype(OPIMM, regs::T5, RvFunct::I::ADDI, regs::T5, 1);
                        // memory[memoryIndex++] = encoder.encode_Stype(STORE, RvFunct::S::SD, regs::T6, regs::T5, 0);
                        // memory[memoryIndex++] = encoder.encode_Itype(LOAD, regs::T5, RvFunct::I::LD, regs::SP, 8);
                        // insertRoutine((char *)&inline_load, 2); // 2 insts
                        // // INLINE ROUTINE ENDS
                        
                        // Inline Routines
                        if(Rail::inlineBBRoutinePost.size()){
                            for(int i=0; i< Rail::inlineBBRoutinePost.size(); i++){
                                memory[memoryIndex++] = Rail::inlineBBRoutinePost[i];
                            }
                        }

                        if(Rail::inlineInstRoutinePost.size()){
                            for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                                memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                            }
                        }

                        if(Rail::inlineInstRoutinesTypedPost[InstType::B_TYPE].size()){
                            for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::B_TYPE].size(); i++){
                                memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::B_TYPE][i];
                            }
                        }


                        insertRoutine((char *)&inline_save, 2); // 2 insts
                        // decodedInst.imm = 28;
                        decodedInst.imm = 36;
                        uint32_t encodedBranch = encoder.encode_Btype(decodedInst);
                        memory[memoryIndex++] = encodedBranch;  // 1 inst

                        // exits[elfReader.getProgramCounter()] = memoryIndex-1;
                        exits[elfReader.getProgramCounter()] = reinterpret_cast<uint64_t>(&memory[memoryIndex-1]);

                        // Place Address of Branch Instruction onto Stack
                        // BRANCH NOT TAKEN
                        // memory[memoryIndex++] = 0x00000013;
                        // insertRoutine((char *)&inline_save, 2); // 2 insts
                        

                        insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                        memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                        // memory[memoryIndex++] = encoder.encode_Utype(AUIPC, regs::T6, 0);
                        // memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                        insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit), regs::T6); // 3 insts
                        memory[memoryIndex++] = jalr_inst; // 1 inst


                        // BRANCH TAKEN
                        // insertRoutine((char *)&inline_save, 2); // 2 insts

                        insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                        memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)
                        // memory[memoryIndex++] = encoder.encode_Utype(AUIPC, regs::T6, 0);
                        // memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                        insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit_taken), regs::T6); // 3 insts
                        memory[memoryIndex++] = jalr_inst; // 1 insts
                    #endif
                    return 0;
                // }
                // railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                // insertBB(railBB); 

            }

            // If function call or jump instruction has been seen (Also a direct Branch)
            // else if(decodedInst.opcode == JAL){
            else if(opcode == JAL){

                railBB.type = BasicBlockType::DIRECT_BRANCH;
                // railBB.lastAddr = elfReader.getProgramCounter();
                // railBB.terminalInst = elfReader.getNextInstruction();
                

                #ifdef DEBUG
                outfile << "Terminal Instruction for Direct Branch: ";
                decoder.decode_instruction(railBB.terminalInst, print_inst);
                #endif
                
                RvInst decodedInst = decoder.decode_Jtype(nextInst);

                // Inline Routines
                if(Rail::inlineBBRoutinePost.size()){
                    for(int i=0; i< Rail::inlineBBRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineBBRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinePost.size()){
                    for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinesTypedPost[InstType::J_TYPE].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::J_TYPE].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::J_TYPE][i];
                    }
                }

                if(count > 0){
                // if(0){
                    if(decodedInst.rd != regs::ZERO){
                        uint64_t target = elfReader.getProgramCounter() + 4;
                        insertLi(target, (regs)(decodedInst.rd));
                    }

                    int retval = elfReader.getProgramCounter() + decodedInst.imm;
                    return retval;
                }else{
                    #ifdef STUBREGIONS
                        memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                        stubDistanceTracker++;
                        insertStubRegion(false);
                    #else
                        // Saving T6 and ra, we will modify these for our jump
                        exits[elfReader.getProgramCounter()] = reinterpret_cast<uint64_t>(&memory[memoryIndex-1]);
                        insertRoutine((char *)&inline_save, 2); 

                        insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                        memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                        insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit_taken), regs::T6);
                        memory[memoryIndex++] = jalr_inst;
                    #endif
                }

                // railBB.endLocationInCache= reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                // insertBB(railBB);

                // cout << "JAL AT " << memoryIndex -1 << " " << hex << elfReader.getProgramCounter() << " " << &memory[memoryIndex] << endl;

                return 0;
            }
            // decoder.decode_instruction(nextInst, true);
            // Indirect jump -> JALR rd imm(rs1)
            // else if(decodedInst.opcode == JALR){
            else if(opcode == JALR){
                // cout << "Count at JALR is: " << count << endl; 
                railBB.type = BasicBlockType::INDIRECT_BRANCH;
                // railBB.lastAddr = elfReader.getProgramCounter();
                
                #ifdef DEBUG
                uint32_t terminalInst = elfReader.getNextInstruction();
                outfile << "Terminal Instruction for Indirect Branch: ";
                decoder.decode_instruction(terminalInst, print_inst);
                #endif

                // Inline Routines
                if(Rail::inlineBBRoutinePost.size()){
                    for(int i=0; i< Rail::inlineBBRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineBBRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinePost.size()){
                    for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE][i];
                    }
                }

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    stubDistanceTracker++;

                    insertStubRegion(false);
                #else
                    // Saving t6 and ra, we will modify these for our jump
                    exits[elfReader.getProgramCounter()] = reinterpret_cast<uint64_t>(&memory[memoryIndex-1]);
                    insertRoutine((char *)&inline_save, 2);
                    
                    insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                    memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                    insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    // stubDistanceTracker++;
                #endif

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                // insertBB(railBB);

                return 0;
            }

            // FIX FOR AUIPC INSTRUCTIONS :: BEGINS
            // else if(decodedInst.opcode == AUIPC){
            else if(opcode == AUIPC){
                
                // decodedInst = decoder.decode_Utype(nextInst); //Replace with getImm
                RvInst decodedInst = decoder.decode_Utype(nextInst); //Replace with getImm

                uint64_t addr = elfReader.getProgramCounter() + (decodedInst.imm << 12);
                Regs reg;

                #ifdef DEBUG
                    outfile << "\tREPLACING AUIPC " << "Adjusted PC Value is " << addr << " and rd is " << reg.regnames[decodedInst.rd] << " Immediate is " << decodedInst.imm << endl;
                #endif
                insertLi(addr, (regs)(decodedInst.rd));   



                if((Rail::instructionRoutinesPre.size() > 0 || 
                Rail::instructionRoutinesPost.size() > 0 || 
                Rail::instructionRoutinesTypedPost[U_TYPE].size() > 0) 
                && shouldInstrumentOnInst){

                    railBB.type = BasicBlockType::SEGMENTED;
                    railBB.lastAddr = elfReader.getProgramCounter();
                    railBB.terminalInst = nextInst;
                    
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;

                    railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(railBB);

                    // Inline Routine
                    if(Rail::inlineInstRoutinePost.size()){
                        for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                            memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                        }
                    } 
                    
                    if(Rail::inlineInstRoutinesTypedPost[InstType::U_TYPE].size()){
                        for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::U_TYPE].size(); i++){
                            memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::U_TYPE][i];
                        }
                    }
                    
                    
                    #ifdef STUBREGIONS
                        insertStubRegion(false);
                    #endif

                    return 0;
                }

                elfReader.incProgramCounter();
                nextInst = elfReader.getNextInstruction();
                allocatedPrev++;

                continue;
            }
            // FIX FOR AUIPC INSTRUCTIONS :: ENDS      

            // else if(decodedInst.opcode == STORE || decodedInst.opcode == LOAD){
            else if(opcode == STORE || opcode == LOAD){

                // decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                RvInst decodedInst = decoder.decode_instruction(nextInst, false); //Replace with getRs1
                if(decodedInst.rs1 != regs::SP){

                    regs scratch_reg = (regs)(decodedInst.rs1) == regs::T6? regs::T5: regs::T6;

                    // If rd is an offset from the stack pointer then we must not adjust
                    // blt LW/SW_rs2, sp, 12

                    #ifdef DEBUG
                        outfile << "\tLoad/Store seen...adjusting rs1 value" << endl;
                    #endif

                    // save scratch to stack
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_save, 2);
                    else insertRoutine((char *)&inline_save_T5, 2);

                    // li t6, dataOffset
                    insertLi(dataOffset, scratch_reg);

                    // blt LW/SW_rs1, t6, 12
                    RvInst blt_inst;
                    blt_inst.type = B_TYPE;
                    blt_inst.opcode = BRANCH;
                    blt_inst.funct3 = RvFunct::B::BLT;
                    blt_inst.rs1 = decodedInst.rs1;
                    // blt_inst.rs2 = regs::SP;
                    blt_inst.rs2 = scratch_reg;
                    blt_inst.imm = 12;
                    memory[memoryIndex++] = encoder.encode_Btype(blt_inst);

                    // If rd is within our data section then we must not adjust
                    // blt, LW/SW_rs2 t6, 12

                    // sw/lw rd, x(rs1)
                    memory[memoryIndex++] = nextInst;

                    // jal x0, 16
                    memory[memoryIndex++] = 0x0100006f; 

                    // add rs1, rs1, t6
                    RvInst adjustmentInst;
                    adjustmentInst.opcode = OP;
                    adjustmentInst.funct3 = RvFunct::R::ADD;
                    adjustmentInst.funct7 = 0;
                    adjustmentInst.rd = decodedInst.rs1;
                    adjustmentInst.rs1 = decodedInst.rs1;
                    adjustmentInst.rs2 = scratch_reg;
                    memory[memoryIndex++] = encoder.encode_Rtype(adjustmentInst);

                    // add load/store
                    memory[memoryIndex++] = nextInst;

                    // sub rs1, rs1, t6
                    if(((decodedInst.type == I_TYPE) && (decodedInst.rs1 != decodedInst.rd))||
                    ((decodedInst.type == S_TYPE) && (decodedInst.rs1 != decodedInst.rs2))){
                        adjustmentInst.funct7 = RvFunct::R::SUB_FUNCT7;
                        memory[memoryIndex++] = encoder.encode_Rtype(adjustmentInst);
                    }else{
                    memory[memoryIndex++] = 0x00000013;
                    }

                    // restore scratch
                    if(scratch_reg == regs::T6)
                    insertRoutine((char *)&inline_load, 2);
                    else insertRoutine((char *)&inline_load_T5, 2);

                    stubDistanceTracker+=6;

                }else{
                    CodeCache::memory[memoryIndex++] = nextInst;
                }

                if((Rail::instructionRoutinesPre.size() > 0 || 
                    Rail::instructionRoutinesPost.size() > 0 || 
                    Rail::instructionRoutinesTypedPost[InstType::MEM_ACCESS_TYPE].size() > 0)
                    && shouldInstrumentOnInst){

                    railBB.type = BasicBlockType::SEGMENTED;
                    railBB.lastAddr = elfReader.getProgramCounter();
                    railBB.terminalInst = nextInst;
                    
                    insertRoutine((char *)&inline_save, 2);
                    insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                    // uint32_t jalr_inst = 0x000f8067;
                    memory[memoryIndex++] = jalr_inst;
                    stubDistanceTracker++;
                    // memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])) + 24);  
                    // stubDistanceTracker++;

                    railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                    insertBB(railBB);
                    
                    #ifdef STUBREGIONS
                        insertStubRegion(false); 
                    #endif
                    
                    return 0;
                }
                
                // Adding Instrumentation Routines
                if(Rail::inlineInstRoutinePost.size()){
                    for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                    }
                }
                
                if(Rail::inlineInstRoutinesTypedPost[InstType::MEM_ACCESS_TYPE].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::MEM_ACCESS_TYPE].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::MEM_ACCESS_TYPE][i];
                    }
                }

                InstType type = decoder.getType(opcode);
                if(Rail::inlineInstRoutinesTypedPost[type].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[type].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[type][i];
                    }
                }

                elfReader.incProgramCounter();
                nextInst = elfReader.getNextInstruction(); 
                allocatedPrev++;
                continue;
                

            }else if(opcode == SYSTEM){
                // If the ECALL is at the beginning of the BB then we will add this ECALL to the current block 
                // and terminate the BB at the next ECALL if any

                railBB.type = BasicBlockType::SYSCALL;
                // railBB.lastAddr = elfReader.getProgramCounter();
                // railBB.terminalInst = elfReader.getNextInstruction();
                
                // Adding Instrumentation Routines
                if(Rail::inlineBBRoutinePost.size()){
                    for(int i=0; i< Rail::inlineBBRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineBBRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinePost.size()){
                    for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                    }
                }

                if(Rail::inlineInstRoutinesTypedPost[InstType::SYSTEM_TYPE].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::SYSTEM_TYPE].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::SYSTEM_TYPE][i];
                    }
                }

                if(Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE].size()){
                    for(int i=0; i< Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE].size(); i++){
                        memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[InstType::I_TYPE][i];
                    }
                }

                #ifdef DEBUG
                    outfile << "SYSCALL INSTRUCTION DETECTED" << endl;
                #endif

                #ifdef STUBREGIONS
                    memory[memoryIndex++] = encoder.encode_Jtype(JAL, regs::ZERO, (nearestStubRegion - (long int)(&memory[memoryIndex])));  
                    stubDistanceTracker++;
                    insertStubRegion(false);
                #else
                    exits[elfReader.getProgramCounter()] = reinterpret_cast<uint64_t>(&memory[memoryIndex-1]);
                    insertRoutine((char *)&inline_save, 2);

                    insertLi(elfReader.getProgramCounter(), regs::T6); // 3 insts
                    memory[memoryIndex++] = 0x01f13423; // sd t6, 8(sp) (1 inst)

                    insertLi(reinterpret_cast<uint64_t>(&context_switch_trace_exit), regs::T6);
                    CodeCache::memory[memoryIndex++] = jalr_inst;
                #endif

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                CodeCache::memory[memoryIndex++] = nextInst;
                // stubDistanceTracker+=2;

                // elfReader.incProgramCounter();
                insertBB(railBB);
                
                return 0;

            }
            
            if((Rail::instructionRoutinesPost.size() > 0  || Rail::instructionRoutinesPre.size() > 0 ||
            Rail::instructionRoutinesTypedPost[(InstType)(decoder.getType(opcode))].size() > 0) && shouldInstrumentOnInst){

                railBB.type = BasicBlockType::SEGMENTED;
                railBB.lastAddr = elfReader.getProgramCounter();
                railBB.terminalInst = nextInst;
                CodeCache::memory[memoryIndex++] = nextInst;
                
                insertRoutine((char *)&inline_save, 2);
                insertLi(reinterpret_cast<uint64_t>(&context_switch_taken), regs::T6);
                // uint32_t jalr_inst = 0x000f8067;
                memory[memoryIndex++] = jalr_inst;
                stubDistanceTracker+=2;

                railBB.endLocationInCache = reinterpret_cast<uint64_t>(&memory[memoryIndex]);
                insertBB(railBB);

                #ifdef STUBREGIONS
                    insertStubRegion(false);
                #endif

                return 0;
            }

            

            CodeCache::memory[memoryIndex++] = nextInst;
            
            // Adding Instrumentation Routines
            if(Rail::inlineInstRoutinePost.size()){
                for(int i=0; i< Rail::inlineInstRoutinePost.size(); i++){
                    memory[memoryIndex++] = Rail::inlineInstRoutinePost[i];
                }
            }

            InstType type = decoder.getType(opcode);
            if(Rail::inlineInstRoutinesTypedPost[type].size()){
                for(int i=0; i< Rail::inlineInstRoutinesTypedPost[type].size(); i++){
                    memory[memoryIndex++] = Rail::inlineInstRoutinesTypedPost[type][i];
                }
            }
        
            #ifdef STUBREGIONS
                stubDistanceTracker++;
            #endif
            
            elfReader.incProgramCounter();
            nextInst = elfReader.getNextInstruction();   
            allocatedPrev++;

        }

        return 0;
    }

}