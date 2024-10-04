#ifndef ELF_READER_H
#define ELF_READER_H

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <bitset>
#include <elf.h>
#include "logger.h"

namespace rail{

class ElfReader{
    private:
        std::ifstream elfFile; //
        Elf64_Shdr shstrtabHeader; //
        std::vector<char> shstrtab; //
        std::vector<Elf64_Shdr> sectionHeaders; //
        std::vector<Elf64_Sym> symbolTable; 
        std::vector<char> strtab;

        int textSectionOffset; //
        int program_counter;
        int main_size;

    public:
        std::vector<Elf64_Phdr> programHeaders; //
        Elf64_Ehdr elfHeader; //
        std::vector<char> textSection;
        ElfReader(const std::string &filename);

        /**
         * Prints all sections in binary 
        */
        void printHeaders();

        /**
         * Prints all sections in binary 
        */
        void printSectionNames();
        /**
         * Prints the content of a specific section from an ELF binary
         * @param sectionName name of section
        */
        void printSection(const char *sectionName);
        
        /**
         * Move programs counter to main function 
        */
        int jumpToMain();

        /**
         * Move programs counter to entry function 
        */
        int jumpToEntry();

        /**
         * Stores data sections from binary in a buffer
        */
        int getDataSections(char* buffer);

        /**
         * Loads text sections into vector textSection
        */
        void getTextSection();

        /**
         * Returns value of pseudo program counter
         * @return program counter
        */
        int getProgramCounter();

        /**
         * Returns address of text section
         * @return text section address
        */
        int getTextSectionOffset();

        /**
         * Moves pseudo program counter to an arbitrary location
         * @param newPC new value for program Counter
        */
        void setProgramCounter(int newPC);

        /**
         * Returns instruction at the location of the program counter
         * @return next instruction as uint32_t
        */
        uint32_t getNextInstruction();

        /**
         * Increments pseudo program counter by 4
        */
        void incProgramCounter();

        /**
         * Decrements pseudo program counter by 4
        */
        void decProgramCounter();
        
        /**
         * Retrieves information on symbols in a symbol table
         * @param symtabName name of symbol table
        */
        int getSymbols(const char *symtabName);

        /**
         * Retrieves the name of a function from the symbol table based on the address
         * @param addr address of function
         * @return name of function as string
        */
        const char* getFunctionName(int addr);

        /**
         * Retrieves the info for a symbol from the symbol table based on the name
         * @param symName name of the symbol
         * @return Symbol 
        */
        Elf64_Sym getSymbol(const char *symName);

};

}

#endif