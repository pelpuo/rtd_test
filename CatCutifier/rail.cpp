#include "rail.h"
#include "code_cache.h"

namespace rail{
    typedef long(*fn)(long);


    #define CACHE_SIZE 4*1024*1024
    #define DATA_SIZE 4*1024*1024 // This needs to be calculated based on the binary and not hard coded
    
    std::unique_ptr<ElfReader> Rail::elfReaderPtr;
    std::unique_ptr<CFGGenerator> Rail::cfgGenerator;
    std::function<void(uint64_t*)> Rail::exitRoutine;
    bool Rail::preInstrumentationRoutines = false;
    bool Rail::postInstrumentationRoutines = false;
    bool Rail::traceLinkingEnabled = false;
    ofstream outfile;

    long int code_addr = 0x01000000;
    long int data_addr = 0x09000000;

    int Rail::instructionRoutinesStart = 0;
    int Rail::instructionRoutinesEnd = 0;

    BBRoutineVec Rail::basicBlockRoutinesPre;
    BBRoutineVec Rail::basicBlockRoutinesPost;
    InstRoutineVec Rail::instructionRoutinesPre;
    InstRoutineVec Rail::instructionRoutinesPost;
    char** Rail::railEnvp;
    char** Rail::railArgv;
    int Rail::railArgc;
    std::unordered_map<InstType, InstRoutineVec> Rail::instructionRoutinesTypedPre;
    std::unordered_map<InstType, InstRoutineVec> Rail::instructionRoutinesTypedPost;

    vector<uint32_t> Rail::inlineBBRoutinePost;
    vector<uint32_t> Rail::inlineInstRoutinePost;
    std::unordered_map<InstType, vector<uint32_t>> Rail::inlineInstRoutinesTypedPost;

    Rail::Rail(){
        CodeCache::memory = reinterpret_cast<int*>(mmap((void *)(code_addr),// address
                    CACHE_SIZE, // size = 4MB
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,               // fd (not used here)
                    0));               // offset (not used here)
        // CodeCache::memoryIndex += elfReader.getProgramCounter();

        CodeCache::data = reinterpret_cast<char*>(mmap((void *)(data_addr),// address
                    DATA_SIZE, // size = 4MB
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1,               // fd (not used here)
                    0));               // offset (not used here)

    }

    void Rail::addInstrumentBBAll(const std::function<void(RailBasicBlock, uint64_t*)>&func, InsertPoint iPoint){
        BBRoutine bbRoutine;
        bbRoutine.func = func;
        
        if(iPoint == InsertPoint::PRE){
            basicBlockRoutinesPre.push_back(bbRoutine);
            preInstrumentationRoutines = true;
        }else{
            basicBlockRoutinesPost.push_back(bbRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::addInstrumentBBAll(const std::function<void(RailBasicBlock, uint64_t*)>&func, InsertPoint iPoint, int lowerBound, int upperBound){
        BBRoutine bbRoutine;
        bbRoutine.func = func;
        bbRoutine.lowerBound = lowerBound;
        bbRoutine.upperBound = upperBound;
        
        if(iPoint == InsertPoint::PRE){
            basicBlockRoutinesPre.push_back(bbRoutine);
            preInstrumentationRoutines = true;
        }else{
            basicBlockRoutinesPost.push_back(bbRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::addInstrumentInstAll(const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint){
        InstRoutine instRoutine;
        instRoutine.func = func;

        instructionRoutinesStart = 0;
        instructionRoutinesEnd = std::numeric_limits<int>::max();
        
        if(iPoint == InsertPoint::PRE){
            instructionRoutinesPre.push_back(instRoutine);
            preInstrumentationRoutines = true;
        }else{
            instructionRoutinesPost.push_back(instRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::addInstrumentInstAll(const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint, int lowerBound, int upperBound){
        InstRoutine instRoutine;
        instRoutine.func = func;
        instRoutine.lowerBound = lowerBound;
        instRoutine.upperBound =upperBound;

        if(lowerBound < instructionRoutinesStart){
            instructionRoutinesStart = lowerBound;
        }

        if(upperBound > instructionRoutinesEnd){
            instructionRoutinesEnd = upperBound;
        }

        
        if(iPoint == InsertPoint::PRE){
            instructionRoutinesPre.push_back(instRoutine);
            preInstrumentationRoutines = true;
        }else{
            instructionRoutinesPost.push_back(instRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::addInstrumentInstType(InstType Type, const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint){
        InstRoutine instRoutine;
        instRoutine.func = func;

        instructionRoutinesStart = 0;
        instructionRoutinesEnd = std::numeric_limits<int>::max();

        instRoutine.lowerBound = 0;
        instRoutine.upperBound = std::numeric_limits<int>::max();

        if(iPoint == InsertPoint::PRE){
            instructionRoutinesTypedPre[Type].push_back(instRoutine);
            preInstrumentationRoutines = true;
        }else{
            instructionRoutinesTypedPost[Type].push_back(instRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::addInstrumentInstType(InstType Type, const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint, int lowerBound, int upperBound){
        InstRoutine instRoutine;
        instRoutine.func = func;
        instRoutine.lowerBound = lowerBound;
        instRoutine.upperBound = upperBound;

        if(lowerBound < instructionRoutinesStart){
            instructionRoutinesStart = lowerBound;
        }

        if(upperBound > instructionRoutinesEnd){
            instructionRoutinesEnd = upperBound;
        }

        if(iPoint == InsertPoint::PRE){
            instructionRoutinesTypedPre[Type].push_back(instRoutine);
            preInstrumentationRoutines = true;
        }else{
            instructionRoutinesTypedPost[Type].push_back(instRoutine);
            postInstrumentationRoutines = true;
        }
    }

    void Rail::setExitRoutine(const std::function<void(uint64_t*)>& func){
        Rail::exitRoutine = func;
    }

    void Rail::runInstrument(){
        if(!outfile){
            outfile.open("./rail_logs");
        }


        CodeCache codeCache;
        // outfile << "PROGRAM COUNTER VALUE: " << rail::Rail::elfReaderPtr->getProgramCounter() << endl;
        /*
            First block allocated should be a stub which places arguments in their appropriate regs
        */

        // codeCache.allocateBB(*rail::Rail::elfReaderPtr, rail::Rail::elfReaderPtr->getProgramCounter(), false);
        // Rail::elfReaderPtr->jumpToEntry();

        codeCache.allocateRoot(*rail::Rail::elfReaderPtr);
        int retval = reinterpret_cast<fn>(CodeCache::memory)(CACHE_SIZE);
    }

    void Rail::registerArgs(int argc, char** argv){
        railArgc = argc;
        railArgv = argv;
    }

    void Rail::registerArgs(int argc, char** argv, char** envp){
        railArgc = argc;
        railArgv = argv;
        railEnvp = envp;
    }

    void Rail::setTarget(const std::string &filename){
        Rail::elfReaderPtr = std::make_unique<ElfReader>(filename);
        Rail::elfReaderPtr->getTextSection();
        Rail::elfReaderPtr->jumpToEntry();
        
        int start_addr = Rail::elfReaderPtr->getDataSections(CodeCache::data);
        // int offset = data_addr - start_addr;
        int offset = data_addr; 
        
        CodeCache::dataOffset = offset;

        #ifdef DEBUG
            outfile << "Offset from data mmap is " << offset << endl;
        #endif
    }

    void Rail::addCFGGeneration(const std::string &filename){
        cfgGenerator = std::make_unique<CFGGenerator>(filename);
    }

    void Rail::setLoggingFile(const std::string &filename){
        outfile.open(filename);
    }

    void Rail::setLoggingFileAppend(const std::string &filename){
        outfile.open(filename, std::ios::app);
    }

    void Rail::addInlineInstforInstRoutine(uint32_t instruction){
        RvEncoder encoder;
        Rail::inlineInstRoutinePost.push_back(instruction);    
    }
    
    void Rail::addInlineInstforBBRoutine(uint32_t instruction){
        RvEncoder encoder;
        Rail::inlineBBRoutinePost.push_back(instruction);    
    }


    void Rail::addInlineLiforInstRoutine(uint64_t value, regs reg){
        uint64_t addr = reinterpret_cast<uint64_t>(value);

        uint32_t addr_lsb = addr & 0xfff;
        addr_lsb = addr_lsb >> 1;
        uint32_t addr_msb = (addr >> 12) & 0xfffff;

        uint32_t lui_inst = LUI | (reg << 7) | (addr_msb << 12);
        Rail::inlineInstRoutinePost.push_back(lui_inst);

        uint32_t addi_inst = OPIMM | (reg << 7) | (RvFunct::I::ADDI << 12) | (reg << 15) | (addr_lsb << 20);
        
        Rail::inlineInstRoutinePost.push_back(addi_inst);
        Rail::inlineInstRoutinePost.push_back(addi_inst);

    }

    void Rail::addInlineLiforInstTypeRoutine(uint64_t value, regs reg, InstType type){
        uint64_t addr = reinterpret_cast<uint64_t>(value);

        uint32_t addr_lsb = addr & 0xfff;
        addr_lsb = addr_lsb >> 1;
        uint32_t addr_msb = (addr >> 12) & 0xfffff;

        uint32_t lui_inst = LUI | (reg << 7) | (addr_msb << 12);
        Rail::inlineInstRoutinesTypedPost[type].push_back(lui_inst);

        uint32_t addi_inst = OPIMM | (reg << 7) | (RvFunct::I::ADDI << 12) | (reg << 15) | (addr_lsb << 20);
        
        Rail::inlineInstRoutinesTypedPost[type].push_back(addi_inst);
        Rail::inlineInstRoutinesTypedPost[type].push_back(addi_inst);

    }

    void Rail::addInlineLiforBBRoutine(uint64_t value, regs reg){
        uint64_t addr = reinterpret_cast<uint64_t>(value);

        uint32_t addr_lsb = addr & 0xfff;
        addr_lsb = addr_lsb >> 1;
        uint32_t addr_msb = (addr >> 12) & 0xfffff;

        uint32_t lui_inst = LUI | (reg << 7) | (addr_msb << 12);
        Rail::inlineBBRoutinePost.push_back(lui_inst);

        uint32_t addi_inst = OPIMM | (reg << 7) | (RvFunct::I::ADDI << 12) | (reg << 15) | (addr_lsb << 20);
        
        Rail::inlineBBRoutinePost.push_back(addi_inst);
        Rail::inlineBBRoutinePost.push_back(addi_inst);

    }

    void Rail::enableTraceLinking(){
        traceLinkingEnabled = true;
    }


    void Rail::addInlineInstforInstTypeRoutine(uint32_t instruction, InstType type){
        inlineInstRoutinesTypedPost[type].push_back(instruction);
    }

}