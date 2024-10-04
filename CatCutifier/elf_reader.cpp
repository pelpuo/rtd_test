#include "elf_reader.h"

using namespace std;

namespace rail{

ElfReader::ElfReader(const std::string &filename): elfFile(filename, std::ios::binary) {
    // Initialize program counter
    ElfReader::program_counter = 0;
    
    // Verify if file exists
    if (!elfFile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        throw std::runtime_error("Error opening file");
    }

    // Reading out ELF Header
    elfFile.read(reinterpret_cast<char*>(&(ElfReader::elfHeader)), sizeof(Elf64_Ehdr));


    // Verify ELF magic
    if (strncmp(reinterpret_cast<char *>(elfHeader.e_ident), "\x7F""ELF", 4) != 0) {
        std::cerr << "Not a valid ELF file: " << filename << std::endl;
        throw std::runtime_error("Not a valid ELF file");
    }

    // Verifying whether binary is RISC-V
    if ((int)(elfHeader.e_machine) != 243) {
        std::cerr << "Not a RISC-V Binary: " << filename << std::endl;
        throw std::runtime_error("Not a RISC-V Binary");
    }else{
        #ifdef DEBUG
            outfile << "RISC-V Binary detected" <<  std::endl;
        #endif
    }

    // Read the program header
    elfFile.seekg(elfHeader.e_phoff, std::ios::beg);

    // Reading program headers
    std::vector<Elf64_Phdr> _programHeaders(elfHeader.e_phnum);
    ElfReader::programHeaders = _programHeaders;
    elfFile.read(reinterpret_cast<char*>(ElfReader::programHeaders.data()), sizeof(Elf64_Phdr) * ElfReader::elfHeader.e_phnum);

    // Read the program header
    elfFile.seekg(elfHeader.e_shoff, std::ios::beg);

    // Reading Section headers
    std::vector<Elf64_Shdr> _sectionHeaders(elfHeader.e_shnum);
    ElfReader::sectionHeaders = _sectionHeaders;
    elfFile.read(reinterpret_cast<char*>(ElfReader::sectionHeaders.data()), sizeof(Elf64_Shdr) * ElfReader::elfHeader.e_shnum);

    // Navigate to string table
    elfFile.seekg(elfHeader.e_shstrndx + 1, std::ios::beg);

    ElfReader::shstrtabHeader = sectionHeaders[elfHeader.e_shstrndx];
    std::vector<char> _shstrtab(shstrtabHeader.sh_size);
    ElfReader::shstrtab = _shstrtab;
    elfFile.seekg(shstrtabHeader.sh_offset);
    elfFile.read(shstrtab.data(), shstrtab.size());

};

void ElfReader::printHeaders(){
    outfile << "Headers: \n";
    // for (const Elf64_Phdr& header : programHeaders) {
    //     const char* name = shstrtab.data() + header.p_type;
    //     outfile << "Name: " << name << "\n"
    //          << "Type: " << header.p_offset << "\n"
    //          << "Size: " << header.sh_size << "\n"
    //          << "Address: " << hex << header.sh_addr << "\n"
    //          << "-----------------------------------------" << "\n";
    // }
}

void ElfReader::printSectionNames(){
    outfile << "Sections: \n";
    for (const Elf64_Shdr& section : sectionHeaders) {
        const char* name = shstrtab.data() + section.sh_name;
        outfile << "Name: " << name << "\n"
             << "Type: " << section.sh_type << "\n"
             << "Size: " << section.sh_size << "\n"
             << "Address: " << hex << section.sh_addr << "\n"
             << "-----------------------------------------" << "\n";
    }
}

void ElfReader::getTextSection(){

    for(const Elf64_Shdr& section : sectionHeaders){

        const char* name = shstrtab.data() + section.sh_name;
        
        if (strcmp(name, ".text") == 0) {
            textSectionOffset = section.sh_addr;

            // Move the file pointer to the beginning of the text section
            elfFile.seekg(section.sh_offset, std::ios::beg);

            // Read the content of the text section
            std::vector<char> _textSection(section.sh_size);
            textSection = _textSection;
            elfFile.read(textSection.data(), textSection.size());
            program_counter = section.sh_offset;
            break;

        }
    
    }
}

// int ElfReader::getDataSections(char* buffer){
//     uint64_t start = 0;
//     uint64_t start_addr = 0;
//     int size = 0;

//     char *buffer_pointer = buffer;
//     #ifdef DEBUG
//         outfile << "Buffer Address: " << (int *)(buffer) << endl;
//     #endif
    
//     for (const Elf64_Shdr& section : sectionHeaders) {
//         const char* name = shstrtab.data() + section.sh_name;
        
//         if (strcmp(name, ".rodata") == 0) {
//             start = section.sh_offset;
//             start_addr = section.sh_addr;

//             elfFile.seekg(start, std::ios::beg);
            
//             #ifdef DEBUG
//                 outfile << "Found start of data sections at: " << start_addr << endl;
//             #endif
//         }

//         if (strcmp(name, ".text") != 0) {  
//             elfFile.seekg(section.sh_offset, std::ios::beg);

//             if(section.sh_addr == 0){
//                 continue;
//             }

//             buffer_pointer = (char *)(buffer + section.sh_addr-start_addr);

//             #ifdef DEBUG
//                 outfile << "Adding section <" << name <<"> with size " << section.sh_size << " to addr " << (int *)buffer_pointer << endl;
//             #endif

//             if(!elfFile.read(buffer_pointer, section.sh_size)){
//                 outfile << "Failed to read <" << name <<">" << endl;
//                 return start_addr;
//             }

//             #ifdef DEBUG
//                 outfile << "Completed adding <" << name <<">" << endl;
//             #endif
//         }
//     }
//     #ifdef DEBUG
//     outfile << "Total size: " << size << endl;
//     #endif
    
//     return start_addr;
// }

int ElfReader::getDataSections(char* buffer){
    uint64_t start = 0;
    uint64_t start_addr = 0;
    int size = 0;

    char *buffer_pointer = buffer;

    #ifdef DEBUG
        outfile << "Buffer Address: " << (int *)(buffer) << endl;
    #endif
    
    // cout << "AAAAAAAAAAAAAAAAA\n";
    // Elf64_Sym envSym = getSymbol(".sbss");

    // cout << (char *)(strtab.data() + envSym.st_name) << " | 0x" << hex << envSym.st_value << " | " << envSym.st_size << endl;


    std::size_t index = 0; 
    
    for (index; index < sectionHeaders.size(); index++) {
    const Elf64_Shdr& section = sectionHeaders[index];
        const char* name = shstrtab.data() + section.sh_name;
        if(strcmp(name, ".text") == 0)
        break;

        buffer_pointer = (char *)(buffer + section.sh_addr);
        elfFile.read(buffer_pointer, section.sh_size);
    }
    
    elfFile.seekg(0, std::ios::beg);
    elfFile.read(buffer + 0x10000, elfHeader.e_ehsize);

    index++;
    // cout << index << endl;
    const Elf64_Shdr& section = sectionHeaders[index];
    start = section.sh_offset;
    start_addr = section.sh_addr;
    elfFile.seekg(start, std::ios::beg);

    for (index; index < sectionHeaders.size(); index++) {
    const Elf64_Shdr& section = sectionHeaders[index];
        const char* name = shstrtab.data() + section.sh_name;
        
        if(section.sh_addr == 0){
            continue;
        }

        if(strcmp(name, ".sbss") == 0 || 
            strcmp(name, ".bss") == 0 || 
            strcmp(name, ".tbss") == 0){
            buffer_pointer = (char *)(buffer + section.sh_addr);
            // cout << "BINGO! " << hex << section.sh_addr << " " << (uint64_t)(buffer_pointer) << endl;
            memset(buffer_pointer, 0x0, section.sh_size);
            continue;
        }

        elfFile.seekg(section.sh_offset, std::ios::beg);

        buffer_pointer = (char *)(buffer + section.sh_addr);

        if(!elfFile.read(buffer_pointer, section.sh_size)){
            outfile << "Failed to read <" << name <<">" << endl;
            return start_addr;
        }

        #ifdef DEBUG
            outfile << "Completed adding <" << name <<">" << endl;
        #endif

    }
    
    #ifdef DEBUG
    outfile << "Total size: " << size << endl;
    #endif

    // // Zero out __environ region
    // if(envSym.st_name != UINT32_MAX){
    //     // cout << "ZEROING OUT" << endl;
    //     int offset = 0x09000000;
    //     memset((void *)(envSym.st_value + offset), 0x0, envSym.st_size);
    // }

    
    return start_addr;
}



void ElfReader::printSection(const char *sectionName) {

    for(const Elf64_Shdr& section : sectionHeaders){

      const char* name = shstrtab.data() + section.sh_name;
      
      if (strcmp(name, sectionName) == 0) {
        outfile << "Section Content:" << "\n";

        // Move the file pointer to the beginning of the text section
        elfFile.seekg(section.sh_offset, std::ios::beg);

        // Read and print the content of the text section
        std::vector<char> textContent(section.sh_size);
        elfFile.read(textContent.data(), textContent.size());

        // Print the content in hexadecimal format as 32-bit instructions
        for (size_t i = 0; i < textContent.size(); i += 4) {
            uint32_t instruction = 0;

            // Combine four consecutive bytes into a 32-bit value
            for (size_t j = 0; j < 4; ++j) {
                instruction |= static_cast<uint32_t>(static_cast<unsigned char>(textContent[i + j])) << (j * 8);
            }

            // Print the 32-bit instruction in hexadecimal
            outfile << hex << instruction << ' ';
        }
        outfile << "\n";
        break;
      }
    
    }

}

int ElfReader::getSymbols(const char *symtabName){
    Elf64_Shdr symtabHeader;

    for (int i = 0; i < elfHeader.e_shnum; ++i) {
        Elf64_Shdr shHeader;
        elfFile.seekg(elfHeader.e_shoff + i * elfHeader.e_shentsize);
        elfFile.read(reinterpret_cast<char*>(&shHeader), sizeof(Elf64_Shdr));

        // Read section name from the section header string table
        std::string sectionName(shstrtab.data() + shHeader.sh_name);

        if (sectionName == symtabName) {
            symtabHeader = shHeader;
            // std::outfile << "Found symtab" << std::endl;
            break;
        }
    }

    if (symtabHeader.sh_type != 2) { // SHT_SYMTAB
        std::cerr << "Symbol table not found in the ELF file." << std::endl;
        return 0;
    }

    // Read the symbol table
    std::vector<Elf64_Sym> _symbolTable(symtabHeader.sh_size / sizeof(Elf64_Sym));
    symbolTable = _symbolTable;

    elfFile.seekg(symtabHeader.sh_offset);
    elfFile.read(reinterpret_cast<char*>(symbolTable.data()), symtabHeader.sh_size);


    // Find the string table associated with .symtab (usually .strtab)
    Elf64_Shdr strtabHeader;
    elfFile.seekg(elfHeader.e_shoff);
    for (int i = 0; i < elfHeader.e_shnum; ++i) {
        Elf64_Shdr shHeader;
        elfFile.read(reinterpret_cast<char*>(&shHeader), sizeof(Elf64_Shdr));

        if (shHeader.sh_type == SHT_STRTAB && i == symtabHeader.sh_link) {
            strtabHeader = shHeader;
            // std::outfile << "Found associated string table for .symtab" << std::endl;
            break;
        }
    }

    if (strtabHeader.sh_type != SHT_STRTAB) {
        // std::cerr << "String table associated with .symtab not found." << std::endl;
        return 0;
    }

    // Read the string table
    std::vector<char> _strtab(strtabHeader.sh_size);
    strtab = _strtab;

    elfFile.seekg(strtabHeader.sh_offset);
    elfFile.read(strtab.data(), strtab.size());

    return 1;

}

Elf64_Sym ElfReader::getSymbol(const char *symName){
    const char* symtabName = ".symtab";

    Elf64_Sym notFoundSymbol = {};
    notFoundSymbol.st_name = UINT32_MAX;  // Use a value that is never valid
    
    if(symbolTable.size() == 0 || strtab.size() == 0){
        int symbols = getSymbols(symtabName);
        
        if(symbols == 0)return notFoundSymbol;
    }


    for (const Elf64_Sym &symbol : symbolTable) {
        const char *name = strtab.data() + symbol.st_name;
        if (strcmp(name, symName) == 0) {
            return symbol;
        }
    }
    
    return notFoundSymbol;

}

const char* ElfReader::getFunctionName(int addr){
    const char* symtabName = ".symtab";
    
    if(symbolTable.size() == 0 || strtab.size() == 0){
        int symbols = getSymbols(symtabName);
        
        if(symbols == 0)return 0;
    }

    // Search for the main symbol in the symbol table
    const char *mainStr = "main";

    for (const Elf64_Sym &symbol : symbolTable) {
        if (addr == symbol.st_value) {
            // std::string symbolName(strtab.data() + symbol.st_name);
            return strtab.data() + symbol.st_name;
        }
    }

    const char *x = "";
    return x;
}


int ElfReader::jumpToMain(){

    const char* symtabName = ".symtab";

    if(symbolTable.size() == 0 || strtab.size() == 0){
        int symbols = getSymbols(symtabName);
        
        if(symbols == 0)return 0;
    }

    // Search for the main symbol in the symbol table
    const char *mainStr = "main";

    for (const Elf64_Sym &symbol : symbolTable) {
        std::string symbolName(strtab.data() + symbol.st_name);
        // std::outfile << symbolName << std::endl;
        if (symbolName == mainStr) {
            program_counter = symbol.st_value;
            return 1;
        }
    }

    std::cerr << "Main function not found in the symbol table." << std::endl;
    return 0;
}

int ElfReader::jumpToEntry(){
    #ifdef DEBUG
    outfile << "Jumping to Entry Value at " << hex << ElfReader::elfHeader.e_entry << endl;
    #endif
    program_counter = ElfReader::elfHeader.e_entry;
    return 0;
}

uint32_t ElfReader::getNextInstruction(){
    int offset = program_counter - textSectionOffset;
    uint32_t instruction = 0;

    if(offset < textSection.size() - 4){
        // Combine four consecutive bytes into a 32-bit value
        for (size_t j = 0; j < 4; ++j) {
            instruction |= static_cast<uint32_t>(static_cast<unsigned char>(textSection[offset + j])) << (j * 8);
        }

    }
    return instruction;
}

int ElfReader::getTextSectionOffset(){
    return ElfReader::textSectionOffset;
}

int ElfReader::getProgramCounter(){
    return ElfReader::program_counter;
}

void ElfReader::incProgramCounter(){
    ElfReader::program_counter+=4;
}

void ElfReader::decProgramCounter(){
    ElfReader::program_counter-=4;
}

void ElfReader::setProgramCounter(int newPC){
    ElfReader::program_counter = newPC;
}

}