#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <ftw.h>

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

    skeleton_daemon();

    // Daemon-specific initialization should go here
    const int SLEEP_INTERVAL = 5;

    while(true)
    {
        // Execute daemon heartbeat, where your recurring activity occurs
        // Vector of all files in given directory
        std::vector<std::string> files = split(exec("find /home/mateusz/Desktop -type f -print0 | xargs -0 ls"), '\n');
        std::ifstream virus_signatures;

        // TODO: change to some dir with program (probably change child working dir)
        virus_signatures.open("/home/mateusz/Desktop/Avitex/v_signatures.txt");

        if (virus_signatures.is_open()) {
            std::string line;
            bool virus_found = false;
            while (std::getline(virus_signatures, line)) {
                // Compares hashes from files found by find command with hashes in virus_signatures
                for (auto &i : files) {
                    const std::string &file = i;
                    // Gets only hashes form md5sum command
                    std::string md5_command = "md5sum '" + file + "' | cut -f 1 -d \" \"";
                    std::string file_hash = exec(md5_command.c_str());
                    file_hash.pop_back(); // Remove eol char

                    if (line == file_hash) {
                        virus_found = true;
                        syslog(LOG_ALERT, "%s", ("Virus found: " + i).c_str());
                    }

                }
            }
            if (!virus_found) {
                syslog(LOG_NOTICE, "No viruses :)");
            }
            virus_signatures.close();
            // This break makes daemon to run once
            break;

        } else {
            syslog(LOG_ERR, "Cannot open signatures file");
            break;
        }

        // Sleep for a period of time
        sleep(SLEEP_INTERVAL);

    }

    // Close system logs for the child process
    syslog(LOG_NOTICE, "Stopping Avitex");
    closelog();

    // Terminate the child process when the daemon completes
    exit(EXIT_SUCCESS);
}
