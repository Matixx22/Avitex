#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <ftw.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "lib/Utils.cpp"

using namespace std;

//void cache_hashes(const std::string& dir) {
//    std::ofstream cache;
//    cache.open("md5sums.md5");
//
//    if (cache.is_open()) {
//        std::string cmd = "find " + dir + " -type f -exec md5sum {} + > md5sums.md5";
//        std::vector<std::string> hashes = split(exec(cmd.c_str()), '\n');
//        for (auto &hash : hashes)  {
//            cache << hash << '\n';
//        }
//        cache.close();
//    } else {
//        std::cout << "Cannot open cache file" << std::endl;
//    }
//}

void scan(const string& type, const string& flag, const string& path) {
    //cout << "Scanning..." << endl;
    // Execute daemon heartbeat, where your recurring activity occurs
    // Vector of all files in given directory
    string command;

    if (type == "file")
        command = "find " + path + " -type f -print0 | xargs -0 ls";
    else if (type == "dir") {
        if (flag == "-r")
            // Recursive
            command = "find " + path + " -type f -print0 | xargs -0 ls";
        else if (flag == "-l")
            // Linear
            command = "find " + path + " -maxdepth 1 -type f -print0 | xargs -0 ls";
        else {
            cerr << "Wrong flag. Type -r to scan recursively or -l to scan linearly" << endl;
            exit(1);
        }
    } else {
        cerr << "Wrong scan type. Type file to scan certain file or dir to scan given directory" << endl;
        exit(1);
    }


    vector<string> files = split(exec(command.c_str()), '\n');

    ifstream virus_signatures;

    // TODO: change to some dir with program (probably change child working dir)
    virus_signatures.open("/home/mateusz/Desktop/Avitex/v_signatures.txt");

    if (virus_signatures.is_open()) {
        string line;
        bool virus_found = false;
        vector<string> viruses;
        while (getline(virus_signatures, line)) {
            // Compares hashes from files found by find command with hashes in virus_signatures
            for (auto &i : files) {
                const string &file = i;
                // Gets only hashes form md5sum command
                string md5_command = "md5sum '" + file + "' | cut -f 1 -d \" \"";
                string file_hash = exec(md5_command.c_str());
                file_hash.pop_back(); // Remove eol char

                if (line == file_hash) {
                    virus_found = true;
                    syslog(LOG_ALERT, "%s", ("Virus found: " + i).c_str());
                    cout << "Virus found: " << i << endl;
                    viruses.push_back(i);
                }

            }
        }
        if (!virus_found) {
            syslog(LOG_NOTICE, "No viruses :)");
        }
        virus_signatures.close();
        //std::cout << "Scan finished. Results can be found in /var/sys/syslog." << std::endl;

        // If return 1 than daemon run once
        //exit(0);
        //return viruses;

    } else {
        syslog(LOG_ERR, "Cannot open signatures file");
        exit(1);
    }
}

int main() {
    /*
     * TODO:
     *  - cache hashes to the file during first run (maybe divide into smaller files)
     *  - compare differed hash functions times
     */

//    if (argc < 2) {
//        std::cerr << "Type 'start' to start the program or 'stop' to terminate" << std::endl;
//        return 1;
//    }
//
//    if (std::string(argv[1]) != "start") {
//        std::cerr << "Type 'start' to start the program or 'stop' to terminate" << std::endl;
//        return 1;
//    }

    //skeleton_daemon2();

    // Daemon-specific initialization should go here
    const int SLEEP_INTERVAL = 1;


    // TODO: Implement some input handling (display statistics, start scanning, ...)
    while (true) {
        string option;
        cout << "Type 'q' to exit and 'scan' to scan: ";
        cin >> option;
        cin.ignore();
        if (option == "q") {
            cout << "Press any key to quit" << endl;
            exit(0);
        }
        cout << "Type 'file <absolute_path_to_file>' to scan certain file or dir -r/l <absolute_path_to_dir>"
                "to scan given directory recursively (-r) or linearly (-l)" << endl;

        string scan_option;
        getline(cin, scan_option);
        vector<string> scan_parameters = split(scan_option, ' ');
        string scan_type = scan_parameters[0];
        string flag = scan_parameters[1];
        string path = scan_parameters[2];

        //std::cout << "Your input: " + a << std::endl;

        if (option == "scan") {
            pid_t pid = fork();

            /* Set new file permissions */
            umask(0);
            /* Open the log file */
            openlog("Avitex", LOG_PID, LOG_DAEMON);

            switch (pid) {
                case -1: {
                    printf("Fork error! Scan aborted.");
                    exit(1);
                }
                case 0: {
                    scan(scan_type, flag, path);

                    exit(0);
                }
                default: {
                    cout << "Scanning..." << std::endl;
                }
            }
            int status;
            waitpid(pid, &status, 0);
        }
        //exit(0);

        //std::cout << "Scan finished. Results can be found in /var/sys/syslog." << std::endl;

    }

    // Close system logs for the child process
    syslog(LOG_NOTICE, "Stopping Avitex");
    closelog();

    // Terminate the child process when the daemon completes
    exit(EXIT_SUCCESS);
}
