#ifndef CODE_CACHE_H
#define CODE_CACHE_H

#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <sys/mman.h>

#include "elf_reader.h"
#include "decode.h"
#include "encode.h"
#include "regfile.h"
#include "util.h"
#include "printUtils.h"
#include "logger.h"
#include "railBasicBlock.h"

namespace rail{

    class CodeCache{
        public:
            static unordered_map<uint64_t, rail::RailBasicBlock> RailBasicBlocks; // Translates from address in binary to metadata for block in cache
            static uint32_t currentBB;
            static int *memory;
            static char *data;
            static int dataOffset;
            static int memoryIndex;
            static int randomVal;

            static long int nearestStubRegion;
            static int stubDistanceTracker;
            static int numStubRegions;
            static int countVar;

            static unordered_map<uint64_t, uint64_t> exits;
        
            /**
             * Inserts root block into CodeCache
             * Root block allocates first block in binary, calls any PRE routines and switches control to the binary
             * @param elfReader ElfReader object which has access to the binary
             * @return integer specifying success or failure
            */
            int allocateRoot(ElfReader &elfReader);
            
            /**
             * Inserts new Basic Block into code cache
             * @param elfReader ElfReader object which has access to the binary
             * @param binaryAddress address specifying start of basic block in binary
             * @param resume specify whether or not a terminal instruction can be included in the block for execution
             * @return integer specifying success or failure
            */
            int allocateBB(ElfReader &elfReader, uint64_t binaryAddress, bool resume);

            /**
             * Inserts new trace of length n into code cache
             * @param elfReader ElfReader object which has access to the binary
             * @param binaryAddress address specifying start of basic block in binary
             * @param count number of BBs within this trace
             * @return integer specifying success or failure
            */
            uint64_t allocateTraceBlock(ElfReader &elfReader, uint64_t binaryAddress, RailBasicBlock &railBB, int count);
            
            int allocateTrace(ElfReader &elfReader, uint64_t binaryAddress, int count);

            /**
             * Allocates a block into the code cache containing a single instruction
             * @param elfReader ElfReader object which has access to the binary
             * @param railBB RailBasicBlock to store metadata about the block being allocated
             * @return integer specifying success or failure
            */
            int allocateNextInst(ElfReader &elfReader, RailBasicBlock &railBB);
            
            /**
             * Sets the current basicblock
             * @param bbAddr address of current basic block in binary
             * @return void
            */
            void setCurrentBB(uint32_t bbAddr);

            /**
             * Returns the address of the current basicblock
             * @return address of current basic block
            */
            uint32_t getCurrentBB();

            /**
             * Inserts metadata for a basic block into the RailBasicBlocks map
             * @param railBB RailBasicBlock containing metadata to be stored
            */
            void insertBB(RailBasicBlock railBB);

            void insertStubRegion(bool override);

            /**
             * Returns the metadata for a basic block which has been allocated into the code cache
             * @param address Address of basic block from binary
             * @return RailBasicBlock containing metadata
            */
            rail::RailBasicBlock retrieveBB(uint64_t address);

            void insertInstRoutineLi(uint64_t value, regs reg);
            void insertBBRoutineLi(uint64_t value, regs reg);
            
        private:
            void insertLi(uint64_t value, regs reg);
            void insertRoutine(char* routine, int length);
            // static uint32_t jalr_inst;
            
    };

}
#endif