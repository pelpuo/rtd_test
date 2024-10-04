#ifndef CFG_GEN_H
#define CFG_GEN_H

#include <fstream>
#include <unordered_map>
#include <sstream>

using namespace std;

namespace rail{
    class CFGGenerator{
        private:
            static std::ostringstream oss;
            ofstream writefile;
            unordered_map<string, int> entries;

        public:
            /**
             * Constructor for CFG generation object
             * Open file which CFG data should be stored in 
             * @param filename name of file where dotfile info should be stored
            */
            CFGGenerator(const std::string &filename);

            /**
             * Add a new entry to CFG dotfile
             * @param addr1, addr2 addresses which should have an edge between them
             * @return void
            */
            void addEdge(uint64_t addr1, uint64_t addr2);
            void addEdge(string addr1, uint64_t addr2);
            void addEdge(uint64_t addr1, string addr2);
            
            /**
             * End CFG generation and close file
             * @return void
            */
            void end();
    };

}

#endif