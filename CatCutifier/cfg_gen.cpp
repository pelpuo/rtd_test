#include "cfg_gen.h"

namespace rail{

    std::ostringstream CFGGenerator::oss;
    
    CFGGenerator::CFGGenerator(const std::string &filename){
        writefile.open(filename);
        writefile << "digraph " << "RAIL_CFG" << " {" << endl;
    }

    void CFGGenerator::addEdge(uint64_t addr1, uint64_t addr2){
        // oss << std::hex << addr1 << "-" << std::hex << addr2;
        oss << "\"" << hex << addr1 << "\" -> \""  << hex << addr2 << "\"";
        if(!entries.count(oss.str())){
            // writefile << "\"" << hex << addr1 << "\" -> \""  << hex << addr2 << "\";" << endl;
            entries[oss.str()] = 1;
        }else{
            entries[oss.str()] += 1;
        }

        oss.str("");
        oss.clear();
    }

    void CFGGenerator::addEdge(uint64_t addr1, string addr2){
        // oss << std::hex << addr1 << "-" << std::hex << addr2;
        oss << "\"" << hex << addr1 << "\" -> \""  << addr2 << "\"";
        if(!entries.count(oss.str())){
            // writefile << "\"" << hex << addr1 << "\" -> \""  << hex << addr2 << "\";" << endl;
            entries[oss.str()] = 1;
        }else{
            entries[oss.str()] += 1;
        }

        oss.str("");
        oss.clear();
    }
 
    void CFGGenerator::addEdge(string addr1, uint64_t addr2){
        // oss << std::hex << addr1 << "-" << std::hex << addr2;
        oss << "\"" << addr1 << "\" -> \""  << hex << addr2 << "\"";
        if(!entries.count(oss.str())){
            // writefile << "\"" << hex << addr1 << "\" -> \""  << hex << addr2 << "\";" << endl;
            entries[oss.str()] = 1;
        }else{
            entries[oss.str()] += 1;
        }

        oss.str("");
        oss.clear();
    }

    void CFGGenerator::end(){
        for (const auto& entry : entries){
            writefile << entry.first << " [label=\"" << entry.second << "\"];" << endl;
        }
        writefile << "}" << endl;
        writefile.close();
    }
}