#include "dispatcher.h"
namespace rail{
    CodeCache cache;
    RvDecoder dispatcherDecoder;
    RvEncoder dispatcherEncoder;

    extern "C" uint64_t stub(uint64_t *TRAPFRAME, uint64_t taken, int shouldInstrument, uint64_t lastInstAddress) {
        
        // rail::outfile << "IN DISPATCHER\n";

        // cout << "In DISPATCHER" << endl;
        // cout << "Last Inst " << hex << lastInstAddress << endl;

        rail::RailBasicBlock currentBasicBlock = CodeCache::RailBasicBlocks[CodeCache::currentBB];
        
        RvInst lastInst; 
        BasicBlockType exitType;

        // cout << hex << lastInstAddress << endl;
        
        if(lastInstAddress && CodeCache::exits.count(lastInstAddress)){
            Rail::elfReaderPtr->setProgramCounter(lastInstAddress);
            lastInst = dispatcherDecoder.decode_instruction(Rail::elfReaderPtr->getNextInstruction(), false);
            lastInst.address = lastInstAddress;
            
            switch(lastInst.opcode){
                case BRANCH:            
                    exitType = BasicBlockType::DIRECT_BRANCH;
                    break;
                case JAL:
                    exitType = BasicBlockType::DIRECT_BRANCH;
                    // taken = 1;
                    break;
                case JALR:
                    exitType = BasicBlockType::INDIRECT_BRANCH;
                    break;
                case SYSTEM:
                    exitType = BasicBlockType::SYSCALL;
                    break;
                default:
                    exitType = BasicBlockType::SEGMENTED;
                    break;
            }

        }else{
            lastInst = dispatcherDecoder.decode_instruction(currentBasicBlock.terminalInst, false);
            lastInst.address = currentBasicBlock.lastAddr;
            exitType = currentBasicBlock.type;
        }



        uint64_t rs1 = lastInst.rs1;
        uint64_t rd = lastInst.rd;

        #ifdef DEBUG
            outfile << endl << "Reached Stub function with taken value: " << taken << endl;
            outfile << hex << "Basic Block Address: " << currentBasicBlock.basicBlockAddress << endl;
            outfile << hex << "Current Block: " << CodeCache::currentBB << endl;
            outfile << hex << "Current Block First Addr: " << currentBasicBlock.firstAddr << endl;
            outfile << hex << "Current Block Last Addr: " << currentBasicBlock.lastAddr << endl;
            outfile << hex << "Current Block Location in Cache: " << currentBasicBlock.startLocationInCache << endl;
            outfile << hex << "Current Block End Location in Cache: " << currentBasicBlock.endLocationInCache << endl;
            outfile << "Current Block Terminal Inst: ";
            dispatcherDecoder.decode_instruction(currentBasicBlock.terminalInst, true);
            outfile << "##### Current Block Type: " << dec << BasicBlockTypeStr[currentBasicBlock.type] << endl;
        #endif

        

        uint64_t nextBlockAddr; 
        bool shouldNotExecLast = true;

        
        switch(exitType){
            case BasicBlockType::DIRECT_BRANCH:
                if(lastInst.opcode == BRANCH){
                    if(taken){
                        nextBlockAddr = lastInst.address + lastInst.imm;     
                        currentBasicBlock.takenBlock = true;
                    }else{
                        nextBlockAddr = lastInst.address + 4;
                        currentBasicBlock.takenBlock = false;
                    }

            
                }else{
                    nextBlockAddr = lastInst.address + lastInst.imm;     
                    currentBasicBlock.takenBlock = true;

                    if(lastInst.type == J_TYPE && rd != regs::ZERO){
                        TRAPFRAME[rd-1] = lastInst.address + 4;
                    }
                }

                if(Rail::cfgGenerator){
                    // Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, lastInst.address + lastInst.imm);
                    // Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, lastInst.address + 4);
                    Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, nextBlockAddr);
                    // if(lastInst.name != InstName::JAL_INST)
                        // Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, lastInst.address + 4);
                }
                break;
            case BasicBlockType::INDIRECT_BRANCH:
                nextBlockAddr = TRAPFRAME[rs1-1] + lastInst.imm;
                if(rd != regs::ZERO){
                    TRAPFRAME[rd-1] = lastInst.address + 4;
                }

                if(Rail::cfgGenerator){
                    // Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, TRAPFRAME[rs1-1] + lastInst.imm);
                    Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, nextBlockAddr);
                    // if(lastInst.name != InstName::JALR_INST)
                        // Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, lastInst.address + 4);
                }
                break;
            case BasicBlockType::SEGMENTED:
                nextBlockAddr = lastInst.address + 4;

                if(Rail::cfgGenerator){
                    Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, nextBlockAddr);
                }
                break;
            case BasicBlockType::SYSCALL:
                for (const auto& routine : Rail::instructionRoutinesTypedPre[SYSTEM_TYPE]) {
                    if(routine.lowerBound <= lastInst.address && 
                    routine.upperBound >= lastInst.address){
                        routine.func(lastInst, TRAPFRAME-1);
                    }
                }
                
                shouldNotExecLast = rail::handle_syscalls(TRAPFRAME);
                nextBlockAddr = lastInst.address + 4;

                for (const auto& routine : Rail::instructionRoutinesTypedPost[SYSTEM_TYPE]) {
                    if(routine.lowerBound <= lastInst.address && 
                    routine.upperBound >= lastInst.address){
                        routine.func(lastInst, TRAPFRAME-1);
                    }
                }

                if(Rail::cfgGenerator){
                    Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, nextBlockAddr);
                }
                    break;
                #ifdef DEBUG
                    outfile << "BLOCK WITH A SYSCALL DETECTED" << endl;
                #endif
                break;
            default:
                break;
        }

        // Invoking Instrumentation Routines:
        if(Rail::postInstrumentationRoutines){
            if(currentBasicBlock.type != BasicBlockType::SEGMENTED){
                for (const auto& routine : Rail::basicBlockRoutinesPost) {

                    if(routine.lowerBound <= currentBasicBlock.firstAddr && 
                        routine.upperBound >= currentBasicBlock.lastAddr){
                        routine.func(currentBasicBlock, TRAPFRAME-1);
                    }
                }
            }

            for (const auto& routine : Rail::instructionRoutinesPost) {
                if(routine.lowerBound <= currentBasicBlock.lastAddr && 
                routine.upperBound >= currentBasicBlock.lastAddr){
                    routine.func(lastInst, TRAPFRAME-1);
                }
            }

            for (const auto& routine : Rail::instructionRoutinesTypedPost[(InstType)(lastInst.type)]) {
                if(routine.lowerBound <= currentBasicBlock.lastAddr && 
                routine.upperBound >= currentBasicBlock.lastAddr){
                    routine.func(lastInst, TRAPFRAME-1);
                }
            }

            if(lastInst.opcode == LOAD || lastInst.opcode == STORE){
                for (const auto& routine : Rail::instructionRoutinesTypedPost[MEM_ACCESS_TYPE]) {
                    if(routine.lowerBound <= currentBasicBlock.lastAddr && 
                    routine.upperBound >= currentBasicBlock.lastAddr){
                        routine.func(lastInst, TRAPFRAME-1);
                    }
                }
            }
        }

        #ifdef DEBUG
        // outfile << "PC Value: " << elfReaderPtr->getProgramCounter() << endl;
        // outfile << "RA Value: " << hex << TRAPFRAME[regs::RA - 1] << endl;
        // outfile << "GP Value: " << hex << TRAPFRAME[regs::GP - 1] << endl;
        // outfile << "SP Value: " << hex << TRAPFRAME[regs::SP - 1] << endl;
        // outfile << "S0 Value: " << hex << TRAPFRAME[regs::S0 - 1] << endl;
        // outfile << "A0 Value: " << hex << TRAPFRAME[regs::A0 - 1] << endl;
        // outfile << "A1 Value: " << hex << TRAPFRAME[regs::A1 - 1] << endl;
        // outfile << "A2 Value: " << hex << TRAPFRAME[regs::A2 - 1] << endl;
        // outfile << "A3 Value: " << hex << TRAPFRAME[regs::A3 - 1] << endl;
        // outfile << "A4 Value: " << hex << TRAPFRAME[regs::A4 - 1] << endl;
        // outfile << "A4 Value: " << hex << TRAPFRAME[regs::A4 - 1] << endl;
        // outfile << "A5 Value: " << hex << TRAPFRAME[regs::A5 - 1] << endl;
        // outfile << "T6 Value: " << hex << TRAPFRAME[regs::T6 - 1] << endl;
        #endif
        
        rail::RailBasicBlock nextBasicBlock;
        if(!CodeCache::RailBasicBlocks.count(nextBlockAddr)){
            if(Rail::traceLinkingEnabled){
                cache.allocateTrace(*rail::Rail::elfReaderPtr, nextBlockAddr, 10);
            }else{
                cache.allocateBB(*rail::Rail::elfReaderPtr, nextBlockAddr, shouldInstrument);
            }
        }
        
        nextBasicBlock = CodeCache::RailBasicBlocks[nextBlockAddr];

        if(Rail::traceLinkingEnabled){
            if(exitType == BasicBlockType::DIRECT_BRANCH && CodeCache::exits.count(lastInstAddress)){
                uint64_t exitAddr = CodeCache::exits[lastInstAddress];
                if(!taken){
                    int target = (long int)(nextBasicBlock.startLocationInCache+4) - (exitAddr+8);
                    uint64_t patchedInst = dispatcherEncoder.encode_Jtype(JAL, regs::ZERO, target);
                    CodeCache::memory[((exitAddr+4)-0x1000000)/4] = 0x01010113;
                    CodeCache::memory[((exitAddr+8)-0x1000000)/4] = patchedInst;
                }else{
                    int target = (long int)(nextBasicBlock.startLocationInCache+4) - (exitAddr+40);
                    uint64_t patchedInst = dispatcherEncoder.encode_Jtype(JAL, regs::ZERO, target);
                    CodeCache::memory[((exitAddr+36)-0x1000000)/4] = 0x01010113;
                    CodeCache::memory[((exitAddr+40)-0x1000000)/4] = patchedInst;
                }
            }
        }

        if(nextBasicBlock.type == BasicBlockType::SEGMENTED){
            CodeCache::RailBasicBlocks[nextBlockAddr].basicBlockAddress = currentBasicBlock.basicBlockAddress;
        }else{
            // add entry to cfgFile
            // outfile << "Adding entry to CFG File" << endl;
            // if(Rail::cfgGenerator){
            //     Rail::cfgGenerator->addEdge(currentBasicBlock.basicBlockAddress, CodeCache::RailBasicBlocks[nextBlockAddr].basicBlockAddress);
            // }
            
            /*
            Branches will have 2 entries: 
            1 for the taken branch
            The other for the not taken branch
            4 different conditions:
                Direct Branch
                    Taken
                    Not Taken
                Indirect Branch
                    Taken
                    Not Taken
            */
        }


        // Safely return from binary if RAIl was unable to locate a next block
        if(nextBasicBlock.firstAddr == 0){
            exit_binary(TRAPFRAME);
            outfile << "Exited from binary prematurely due to issue locating block" << endl;
        }
        
        // Calling Pre routines
        if(Rail::preInstrumentationRoutines){
            RvInst firstInst = dispatcherDecoder.decode_instruction(nextBasicBlock.startInst, false);
            firstInst.address = nextBasicBlock.firstAddr;

            if(nextBasicBlock.type != BasicBlockType::SEGMENTED){
                for (const auto& routine : Rail::basicBlockRoutinesPre) {
                    RailBasicBlock block; // Create a RailBasicBlock object
                    routine.func(nextBasicBlock, TRAPFRAME-1); // Call the function with the block object
                }
            }


            for (const auto& routine : Rail::instructionRoutinesPre) {
                if(routine.lowerBound <= nextBasicBlock.firstAddr && 
                routine.upperBound >= nextBasicBlock.firstAddr){
                    routine.func(firstInst, TRAPFRAME-1);
                }
            }
        }

        cache.setCurrentBB(nextBasicBlock.firstAddr);

        #ifdef DEBUG
        outfile << endl << hex << "Next Block First Addr: " << nextBasicBlock.firstAddr << endl;
        outfile << hex << "Next Block Last Addr: " << nextBasicBlock.lastAddr << endl;
        outfile << hex << "Next Block Location in Cache: " << nextBasicBlock.startLocationInCache << endl;
        outfile << hex << "Next Block Terminal Inst: "; // << hex << nextBasicBlock.terminalInst << endl;
        dispatcherDecoder.decode_instruction(nextBasicBlock.terminalInst, true);
        outfile << "##### Next Block Type: " << dec << BasicBlockTypeStr[nextBasicBlock.type] << endl;

        outfile << "Leaving Stub function and returning to " << hex << nextBasicBlock.startLocationInCache << endl << endl;
        #endif

        uint64_t returnVal = nextBasicBlock.startLocationInCache;

        // If we need to execute the last instruction in the block (eg ECALL) then we should deduct the returnVal
        if(!shouldNotExecLast){
            returnVal -= 4;
        }

        // cout << hex << Rail::railArgv[0] << endl;
        return returnVal;
    }

    // Handler for Typed PRE-Instrumentation Routines
    extern "C" uint64_t instrument(uint64_t* TRAPFRAME, uint64_t taken){

        rail::RailBasicBlock currentBasicBlock = CodeCache::RailBasicBlocks[CodeCache::currentBB];

        #ifdef DEBUG
            outfile << endl << "Reached SINGLE INST Stub function for inst: ";
            dispatcherDecoder.decode_instruction(currentBasicBlock.terminalInst, true);
        #endif

        RvInst firstInst = dispatcherDecoder.decode_instruction(currentBasicBlock.startInst, false);
        firstInst.address = currentBasicBlock.firstAddr;

        for (const auto& routine : Rail::instructionRoutinesTypedPre[(InstType)(firstInst.type)]) {
            routine.func(firstInst, TRAPFRAME-1);
        }
        if(firstInst.opcode == LOAD || firstInst.opcode == STORE){
            for (const auto& routine : Rail::instructionRoutinesTypedPre[MEM_ACCESS_TYPE]) {
                if(Rail::instructionRoutinesStart <= currentBasicBlock.firstAddr && 
                Rail::instructionRoutinesEnd >= currentBasicBlock.firstAddr){
                    routine.func(firstInst, TRAPFRAME-1);
                }
            }
        }

        return currentBasicBlock.startLocationInCache+24;
    }


    extern "C" uint64_t init(uint64_t *TRAPFRAME){
        uint64_t entryAddr = Rail::elfReaderPtr->getProgramCounter();

        if(Rail::traceLinkingEnabled){
            cache.allocateTrace(*rail::Rail::elfReaderPtr, entryAddr, 1);
        }else{
            cache.allocateBB(*rail::Rail::elfReaderPtr, entryAddr, false);
        }


        if(Rail::cfgGenerator){
            Rail::cfgGenerator->addEdge("START", entryAddr);
        }

        // for(int i=0; i<31; i++){
        //     if(i>3 || i<1)
        //     TRAPFRAME[i] = 0; 
        // }
            // TRAPFRAME[regs::A1-1] = 1; 
            // const char *path = "testbins/fib\0";
            // TRAPFRAME[regs::A2-1] = *path; 

        rail::RailBasicBlock nextBasicBlock;
        nextBasicBlock = CodeCache::RailBasicBlocks[entryAddr];
        
        // Execute Pre-Routines
        // RvInst firstInst = dispatcherDecoder.decode_instruction(nextBasicBlock.startInst, false);
        
        if(Rail::preInstrumentationRoutines){
            RvInst firstInst = dispatcherDecoder.decode_instruction(nextBasicBlock.startInst, false);
            firstInst.address = nextBasicBlock.firstAddr;

            if(nextBasicBlock.type != BasicBlockType::SEGMENTED){
                for (const auto& routine : Rail::basicBlockRoutinesPre) {
                    RailBasicBlock block; // Create a RailBasicBlock object

                    if(routine.lowerBound <= nextBasicBlock.firstAddr && 
                        routine.upperBound >= nextBasicBlock.lastAddr){
                        routine.func(nextBasicBlock, TRAPFRAME-1);
                    }

                }
            }

            for (const auto& routine : Rail::instructionRoutinesPre) {
                if(routine.lowerBound <= nextBasicBlock.firstAddr && 
                routine.upperBound >= nextBasicBlock.firstAddr){
                    routine.func(firstInst, TRAPFRAME-1);
                }
            }
        }

        // Copying Argc and Argv
        int stEnd = 0;
        *(TRAPFRAME + stEnd++) = Rail::railArgc;
        // *(TRAPFRAME) = Rail::railArgc;
        // stEnd++;
        for(int i=0; i<Rail::railArgc; i++){
            *(TRAPFRAME + stEnd++) = (uint64_t)(Rail::railArgv[i]);
        }
        
        *(TRAPFRAME+ stEnd++) = 0;

        // Copying Envp
        int envpCount = 0;
        for (envpCount; Rail::railEnvp[envpCount] != NULL; envpCount++) {
            *(TRAPFRAME + stEnd++) = (uint64_t)(Rail::railEnvp[envpCount]);
            // printf("envp[%d] = %s\n", envpCount, Rail::railEnvp[envpCount]);
        }

        *(TRAPFRAME+ stEnd++) = 0;
        *(TRAPFRAME+ stEnd++) = 0;
        *(TRAPFRAME+ stEnd++) = 0;
        *(TRAPFRAME+ stEnd++) = 0;
        *(TRAPFRAME+ stEnd++) = 0;


        // stEnd -=3;

        // Copying of Auxiliary vector
        // uint64_t *auxVec = (uint64_t *)(Rail::railEnvp);
        // auxVec += envpCount+1; 

        // for(int i=0; i<50; i++){
        //     cout << "Aux " << *(auxVec + i) << endl;
        // }

        // uint64_t auxVal;
        // while(*auxVec != AT_NULL){
        //     auxVal = *(auxVec + 1);

        //     switch(auxVal){
        //         case AT_PAGESZ:
        //         case AT_HWCAP:
        //         case AT_HWCAP2:
        //         case AT_CLKTCK:
        //         case AT_FLAGS:
        //         case AT_UID:
        //         case AT_EUID:
        //         case AT_GID:
        //         case AT_EGID:
        //         case AT_SECURE:
        //         case AT_SYSINFO_EHDR:
        //         case AT_MINSIGSTKSZ:
        //         case AT_PHENT:
        //         case AT_RANDOM:
        //             *(TRAPFRAME + stEnd++) = *auxVec;
        //             *(TRAPFRAME + stEnd++) = auxVal;
        //             break;

        //         // case AT_PLATFORM:
        //         // case AT_EXECFN:
        //         // case AT_BASE:
        //         // case AT_PHDR:
        //         // case AT_PHNUM:
        //         // case AT_ENTRY:
        //         default:
        //             break;
        //     }


        //     // cout << "Aux Vec " << *(auxVec) << endl;
        //     // cout << "Aux Val " << auxVal << endl;

        //     auxVec+=2;
        // }


        uint64_t phdr_address = 0;
        uint64_t base_address = 0;
        for (const auto& phdr : Rail::elfReaderPtr->programHeaders) {
            if (phdr.p_type == PT_PHDR) {
                phdr_address = phdr.p_vaddr;
            } else if (phdr.p_type == PT_LOAD && (phdr.p_flags & PF_X)) {
                // PT_LOAD segment that is executable (typically the base address)
                base_address = phdr.p_vaddr;
            }
        }

        AuxvEntry auxv[] = {
            { AT_HWCAP,  (uint64_t)0}, 
            { AT_PAGESZ, (uint64_t)4096}, 
            { AT_CLKTCK,  (uint64_t)100}, 
            { AT_PHDR,  (uint64_t)phdr_address}, 
            { AT_PHENT, (uint64_t)(Rail::elfReaderPtr->elfHeader.e_phentsize) },
            { AT_PHNUM, (uint64_t)(Rail::elfReaderPtr->elfHeader.e_phnum) },   
            { AT_BASE, (uint64_t)base_address }, 
            { AT_FLAGS, (uint64_t)0 }, 
            { AT_ENTRY, (uint64_t)((Rail::elfReaderPtr->elfHeader.e_entry)) }, 
            { AT_UID,   (uint64_t)1000 },  
            { AT_EUID,  (uint64_t)1000 }, 
            { AT_GID,   (uint64_t)1000 }, 
            { AT_EGID,  (uint64_t)1000 },
            // { AT_SECURE,   0 },     
            // { AT_RANDOM,   0x7ffc7b5509a9},     
            { AT_HWCAP2,   2 },     
            { AT_EXECFN,   (uint64_t)(Rail::railArgv[0]) },     
            // { AT_PLATFORM,   (uint64_t)(Rail::railArgv[0]) },     
            { AT_NULL,  0 }               
        };

        int auxvIndex = 0;
        while (auxv[auxvIndex].a_type != AT_NULL) {
            *(TRAPFRAME + stEnd++) = auxv[auxvIndex].a_type;  // Type
            *(TRAPFRAME + stEnd++) = auxv[auxvIndex].a_val;   // Value
            auxvIndex++;
        }
        *(TRAPFRAME + stEnd++) = AT_NULL; // End of auxiliary vector
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;
        *(TRAPFRAME + stEnd++) = 0;

        
        return nextBasicBlock.startLocationInCache;

    }



    extern "C" void exit_binary(uint64_t *TRAPFRAME){

        if(rail::Rail::exitRoutine){
            uint64_t *offset_TRAPFRAME = TRAPFRAME - 1;
            rail::Rail::exitRoutine(offset_TRAPFRAME);
        }

        if(Rail::cfgGenerator){
            Rail::cfgGenerator->addEdge(CodeCache::RailBasicBlocks[CodeCache::currentBB].basicBlockAddress, "END");
            Rail::cfgGenerator->end();
        }
        
        outfile.close();
        munmap(CodeCache::memory, 4*1024*1024);
        munmap(CodeCache::data, 4*1024*1024);
        exit(1);
    }

}
