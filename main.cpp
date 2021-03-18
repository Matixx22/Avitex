#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <memory>
#include <vector>
#include <sstream>

#include "lib/Utils.cpp"

void cache_hashes(const std::string& dir) {
    std::ofstream cache;
    cache.open("md5sums.md5");

    if (cache.is_open()) {
        std::string cmd = "find " + dir + " -type f -exec md5sum {} + > md5sums.md5";
        std::vector<std::string> hashes = split(exec(cmd.c_str()), '\n');
        for (auto &hash : hashes)  {
            cache << hash << '\n';
        }
        cache.close();
    } else {
        std::cout << "Cannot open cache file" << std::endl;
    }
}

int main() {
    /*
     * TODO:
     *  - cache hashes to the file during first run (maybe divide into smaller files)
     *  - compare differed hash functions times
     */
    // Vector of all files in given directory
    std::vector<std::string> files = split(exec("find /home/mateusz/Desktop -type f -print0 | xargs -0 ls"), '\n');
    std::ifstream virus_signatures;

    //cache_hashes("/home");

    virus_signatures.open("../v_signatures.txt");

    std::cout << "Welcome to Avitex" << std::endl;

    if (virus_signatures.is_open()) {
        std::string line;
        while (std::getline(virus_signatures, line)) {
            // Compares hashes from files found by find command with hashes in virus_signatures
            for (auto & i : files) {
                const std::string& file = i;
                // Gets only hashes form md5sum command
                std::string md5_command = "md5sum '" + file + "' | cut -f 1 -d \" \"";
                std::string file_hash = exec(md5_command.c_str());
                file_hash.pop_back(); // Remove eol char

                if (line == file_hash) {
                    std::cout << "Virus found: " + i << std::endl;
                }
            }
        }
        virus_signatures.close();
    } else {
        std::cerr << "Error during opening a virus signatures file\n";
    }

    return 0;
}
