#ifndef RAIL_H
#define RAIL_H

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>

#include "instructions.h"
#include "elf_reader.h"
#include "logger.h"
#include "railBasicBlock.h"
#include "cfg_gen.h"
#include "regfile.h"
#include <limits>


/*

RAIL API
* addInstrumentBBAll()
* addInstrumentBBType()
* addInstrumentInstAll()
* addInstrumentInstType()
* setExitRoutine()
* setTarget()

*/

namespace rail{
    using InstRoutineFunc = std::function<void(RvInst, uint64_t*)>;
    using BBRoutineFunc = std::function<void(RailBasicBlock, uint64_t*)>;    


    
    typedef struct{
        InstRoutineFunc func;
        int lowerBound = 0;
        int upperBound = std::numeric_limits<int>::max();
    }InstRoutine;

    typedef struct{
        BBRoutineFunc func;
        int lowerBound = 0;
        int upperBound = std::numeric_limits<int>::max();
    }BBRoutine;


    using InstRoutineVec = std::vector<InstRoutine>;
    using BBRoutineVec = std::vector<BBRoutine>;    


    typedef enum{
        PRE,
        POST
    }InsertPoint;

    static const char* railArgs[] = {"rail_bin", "arg1", "arg2", "arg3", "arg4", "arg5"};
    class Rail{

        public:
            static BBRoutineVec basicBlockRoutinesPre;
            static BBRoutineVec basicBlockRoutinesPost;
            static int basicBlockRoutinesStart;
            static int basicBlockRoutinesEnd;
            static char** railEnvp;
            static char** railArgv;
            static int railArgc;
            static bool traceLinkingEnabled;

            static InstRoutineVec instructionRoutinesPre;
            static InstRoutineVec instructionRoutinesPost;
            static int instructionRoutinesStart;
            static int instructionRoutinesEnd;
            
            static std::unordered_map<InstType, InstRoutineVec> instructionRoutinesTypedPre;
            static std::unordered_map<InstType, InstRoutineVec> instructionRoutinesTypedPost;
            static int instructionRoutinesTypedStart;
            static int instructionRoutinesTypedEnd;
            
            static std::unique_ptr<ElfReader> elfReaderPtr;
            static std::unique_ptr<CFGGenerator> cfgGenerator;
            static std::function<void(uint64_t*)> exitRoutine;
            static bool preInstrumentationRoutines;
            static bool postInstrumentationRoutines;

            static vector<uint32_t> inlineBBRoutinePost;
            static vector<uint32_t> inlineInstRoutinePost;

            void addInlineInstforInstRoutine(uint32_t instruction);
            void addInlineInstforBBRoutine(uint32_t instruction);

            static std::unordered_map<InstType, vector<uint32_t>> inlineInstRoutinesTypedPost;
            void addInlineInstforInstTypeRoutine(uint32_t instruction, InstType Type);

            void enableTraceLinking();
            
            void addInlineLiforInstRoutine(uint64_t value, regs reg);
            void addInlineLiforInstTypeRoutine(uint64_t value, regs reg, InstType type);
            void addInlineLiforBBRoutine(uint64_t value, regs reg);
            
            /**
             * Constructor for RAIL object
             * @param none
            */
            Rail();

            /**
             * Add an instrumentation function to be called on all BasicBlocks (Either before or after)
             * @param func void function with an RvInst and uint64_t* as arguments which is called on each BasicBlock
             * @param iPoint InsertPoint object to determine if function should be placed before or after BasicBlock
             * @return void
            */
            void addInstrumentBBAll(const std::function<void(RailBasicBlock, uint64_t*)>& func, InsertPoint iPoint);

            void addInstrumentBBAll(const std::function<void(RailBasicBlock, uint64_t*)>& func, InsertPoint iPoint, int lowerBound, int upperBound);

            /**
             * Add an instrumentation function to be called on all Instructions (Either before or after)
             * @param func void function with an RvInst and uint64_t* as arguments which is called on each instruction
             * @param iPoint InsertPoint object to determine if function should be placed before or after BasicBlock
             * @return void
            */
            void addInstrumentInstAll(const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint);
            
            void addInstrumentInstAll(const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint, int lowerBound, int upperBound);

            /**
             * Add an instrumentation function to be called on instructions based on RISC-V type (R, I, S, U, B, J)
             * @param Type InstType parameter determining the RISC-V class (eg: R_TYPE)
             * @param func void function with an RvInst and uint64_t* as arguments which is called on each instruction of type Type
             * @param iPoint InsertPoint object to determine if function should be placed before or after BasicBlock
             * @return void
            */
            void addInstrumentInstType(InstType Type, const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint);

            void addInstrumentInstType(InstType Type, const std::function<void(RvInst, uint64_t*)>& func, InsertPoint iPoint, int lowerBound, int upperBound);

            /**
             * Specify file where output from RAIL logger should go
             * @param filename file name for logger output
             * @return void
            */
            void setLoggingFile(const std::string &filename);

            /**
             * Specify file where output from RAIL logger should go. 
             * Appends new output to the file
             * @param filename file name for logger output
             * @return void
            */
            void setLoggingFileAppend(const std::string &filename);

            /**
             * Specify name of file to be instrumented
            * @param filename path + name of file to be instrumented
             * @return void
            */
            void setTarget(const std::string &filename);

            /**
             * Specify function to be called on when RAIL exits the instrumented binary
             * @param func void function with a uint64_t* as argument which is called on exit
             * @return void
            */
            void setExitRoutine(const std::function<void(uint64_t*)>& func);

            /**
             * Pass the command line arguments to the binary
             * @param argc number of arguments as int
             * @param argv pointer to argument string array
             * @return void
            */
            void registerArgs(int argc, char** argv);

            /**
             * Pass the command line arguments to the binary
             * @param argc number of arguments as int
             * @param argv pointer to argument string array
             * @return void
            */
            void registerArgs(int argc, char** argv, char** envp);
            
            /**
             * Start the instrumentation
             * @return void
            */
            void runInstrument();

            /**
             * Enable generation of Control Flow Graph from binary and specify where dotfile data should be stored
             * @param filename path + name of file where dotfile information should be stored
             * @return void
            */
            void addCFGGeneration(const std::string &filename);
    };
}


#endif