#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "code_cache.h"
#include "regfile.h"
#include "logger.h"
#include "system_calls.h"
#include "rail.h"
#include "auxvec.h"

namespace rail{

    extern ElfReader elfReaderPtr;

    /**
     * Initializes registers and call PRE routines before execution starts
     * @param TRAPFRAME stack pointer value for trapframe
    */
    extern "C" uint64_t init(uint64_t *TRAPFRAME);

    /**
     * Runs instrumentation routines and switches control to the next block
     * @param TRAPFRAME stack pointer value for trapframe
     * @param taken informs whether or not branch was taken
     * @param shouldInstrument informs whether or not to run instrumentation routines
    */
    extern "C" uint64_t stub(uint64_t* TRAPFRAME, uint64_t taken, int shouldInstrument, uint64_t lastInstAddress);

    /**
     * Calls PRE routines for instruction types
     * @param TRAPFRAME stack pointer value for trapframe
     * @param taken informs whether or not branch was taken
    */
    extern "C" uint64_t instrument(uint64_t* TRAPFRAME, uint64_t taken);

    /**
     * Calls exit function and exits from RAIL
     * @param TRAPFRAME stack pointer value for trapframe
    */
    extern "C" void exit_binary(uint64_t *TRAPFRAME);
}
#endif
