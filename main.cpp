#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <ftw.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "lib/Utils.cpp"

using namespace std;

#define PORT 8080

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

    // TODO: Validate path (regex?)

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
            cerr << "Wrong flag. Type '-r' to scan recursively or '-l' to scan linearly" << endl;
            return;
        }
    } else {
        cerr << "Wrong scan type. Type 'file' to scan certain file or 'dir' to scan given directory" << endl;
        return;
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
            cout << "No viruses :)" << endl;
        }
        virus_signatures.close();


        // If return 1 than daemon run once
        //exit(0);
        //return viruses;

    } else {
        syslog(LOG_ERR, "Cannot open signatures file");
        return;
    }
}

vector<string> get_input() {
    string scan_option;
    vector<string> input_parameters;
    vector<string> scan_parameters;
    getline(cin, scan_option);

    input_parameters = split(scan_option, ' ');
    if (input_parameters.size() == 2) {
        scan_parameters.push_back(input_parameters[0]);
        scan_parameters.emplace_back("-l");
        scan_parameters.push_back(input_parameters[1]);
    } else if (input_parameters.size() == 3) {
        scan_parameters.push_back(input_parameters[0]);
        scan_parameters.push_back(input_parameters[1]);
        scan_parameters.push_back(input_parameters[2]);
    } else {
        cerr << "Wrong input!" << endl;
    }
    return scan_parameters;
}

int main() {

    string greetings = "               _ _            \n"
                       "     /\\       (_) |           \n"
                       "    /  \\__   ___| |_ _____  __\n"
                       "   / /\\ \\ \\ / / | __/ _ \\ \\/ /\n"
                       "  / ____ \\ V /| | ||  __/>  < \n"
                       " /_/    \\_\\_/ |_|\\__\\___/_/\\_\\\n\n"
                       "  ___              _    _                  _       _   _     _             \n"
                       " | __| _ ___ ___  | |  (_)_ _ _  ___ __   /_\\  _ _| |_(_)_ _(_)_ _ _  _ ___\n"
                       " | _| '_/ -_) -_) | |__| | ' \\ || \\ \\ /  / _ \\| ' \\  _| \\ V / | '_| || (_-<\n"
                       " |_||_| \\___\\___| |____|_|_||_\\_,_/_\\_\\ /_/ \\_\\_||_\\__|_|\\_/|_|_|  \\_,_/__/\n"
                       "                                                                           \n";
    string option_menu = "Type 'file <absolute_path_to_file>' to scan certain file\n"
                         "Type 'dir -r/-l <absolute_path_to_dir>' to scan given directory\n"
                         "(-r - recursively, -l - linearly)\n"
                         "Type 'stats' to get scanning statistics\n"
                         "Type 'exit' to exit the program\n";

    // INIT SERVER
    int server_fd, new_socket;
    long valread;
    struct sockaddr_in address{};
    int opt = 1;
    int addrlen = sizeof(address);
    char user_input[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) <0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /*
     * TODO:
     *  - cache hashes to the file during first run (maybe divide into smaller files)
     *  - compare differed hash functions times
     */

    // TODO: socket w programie, pipe

    // TODO: Implement some input handling (display statistics, start scanning, ...)


    while (true) {

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<  0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //send(new_socket, (greetings + option_menu).c_str(), (greetings+option_menu).size(), 0 );
        //printf("Greetings sent\n");

        valread = recv(new_socket, user_input, 1024, 0);
        if (valread < 0) {
            perror("Receive error");
        } else if (valread == 0) {
            cout << "No user input" << endl;
        } else {
            printf("User input: %s\n", user_input);

            vector<string> options;
            string flag = {0};
            string path = {0};

            options = split(user_input, ' ');
            if (options.size() == 1) {
                flag = options[0];
                path = "null";
            } else {
                flag = options[0];
                path = options[1];
            }

            if (flag == "-s") {
                send(new_socket, "Last scan statistics", 30, 0);
            } else if (flag == "-sall") {
                send(new_socket, "Scanning statistics", 30, 0);
            } else if (flag == "-f") {
                send(new_socket, "Scan file", 30, 0);
            } else if (flag == "-dr") {
                send(new_socket, "Scan dir rec", 30, 0);
            } else if (flag == "-dl") {
                send(new_socket, "Scan dir lin", 30, 0);
            } else {
                send(new_socket, "Something went wrong", 30, 0);
            }

            if (strncmp(user_input, "scan", strlen(user_input)) == 0) {
                scan("dir", "-r", "/home/mateusz/Desktop");
            }

            memset(user_input, 0, strlen(user_input));
        }
        //shutdown(new_socket, SHUT_RDWR);
//        string option;
//        cout << "Type 'scan' to chose scanning options\n"
//                "Type 'stats' to get statistics about scanning\n"
//                "Press quickly 3 times escape or exit a terminal to leave\n"
//                "command: ";
//        cin >> option;
//        cin.ignore();
//
//        if (option == "scan") {
//            vector<string> scan_options = get_input();
//            string scan_type = scan_options[0];
//            string flag = scan_options[1];
//            string path = scan_options[2];
//
//            pid_t pid = fork();
//
//            /* Set new file permissions */
//            umask(0);
//            /* Open the log file */
//            openlog("Avitex", LOG_PID, LOG_DAEMON);
//
//            switch (pid) {
//                case -1: {
//                    printf("Fork error! Scan aborted.");
//                    exit(1);
//                }
//                case 0: {
//                    scan(scan_type, flag, path);
//
//                    exit(0);
//                }
//                default: {
//                    cout << "Scanning..." << std::endl;
//                }
//            }
//            int status;
//            waitpid(pid, &status, 0);
//        }


    }

    // Close system logs for the child process
    syslog(LOG_NOTICE, "Stopping Avitex");
    closelog();

    // Terminate the child process when the daemon completes
    exit(EXIT_SUCCESS);
}
