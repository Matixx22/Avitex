#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/syslog.h>

using namespace std;
namespace fs = filesystem;

#define PORT 8080

string exec(const char *cmd) {
    array<char, 128> buffer{};
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void cache_hashes(const string& dir) {
    ofstream cache;
    cache.open("md5sums.md5");

    if (cache.is_open()) {
        string cmd = "find " + dir + " -type f -exec md5sum {} + > md5sums.md5";
        vector<std::string> hashes = split(exec(cmd.c_str()), '\n');
        for (auto &hash : hashes)  {
            cache << hash << '\n';
        }
        cache.close();
    } else {
        cout << "Cannot open cache file" << endl;
    }
}

void quarantine(const string& virus_path) {
    string quarantine_path = fs::current_path();
    quarantine_path += "/quarantine";
    if (!fs::exists(quarantine_path)) {
        fs::create_directory(quarantine_path);
        fs::permissions(quarantine_path, fs::perms::owner_write);
    }

    string cmd = "mv " + virus_path + " " + quarantine_path;
    system(cmd.c_str());
}

void scan(const string& flag, const string& path, int socket) {
    string command;
    string output;
    char tmp[4] = {0};

    if (flag == "-dr" || flag == "-f")
        // Recursive
        command = "find " + path + " -type f -print0 | xargs -0 ls";
    else if (flag == "-dl")
        // Linear
        command = "find " + path + " -maxdepth 1 -type f -print0 | xargs -0 ls";
    else {
        cerr << "Wrong flag. Type '-r' to scan recursively or '-l' to scan linearly" << endl;
        return;
    }

    vector<string> files = split(exec(command.c_str()), '\n');

    ifstream virus_signatures;

    string working_dir = fs::current_path();
    virus_signatures.open(working_dir + "/v_signatures.txt");

    if (virus_signatures.is_open()) {
        string line;
        bool virus_found = false;

        while (getline(virus_signatures, line)) {
            // Compares hashes from files found by find command with hashes in virus_signatures
            for (auto &i : files) {
                const string &file = i;
                // Gets only hashes form md5sum command
                string md5_command = "md5sum '" + file + "' | cut -f 1 -d \" \"";
                string file_hash = exec(md5_command.c_str());
                file_hash.pop_back(); // Remove eol char

                if (file.starts_with(working_dir + "/quarantine"))
                    continue;

                if (line == file_hash) {
                    virus_found = true;
                    syslog(LOG_ALERT, "%s", ("Virus found: " + i).c_str());
                    string msg = "Virus found: " + i + "\n";
                    recv(socket, tmp, sizeof(tmp), 0);
                    send(socket, msg.c_str(), msg.size(), 0);
                    quarantine(i);
                }
            }
        }
        if (!virus_found) {
            syslog(LOG_NOTICE, "No viruses :)");
            string msg = "No viruses :)\n";
            recv(socket, tmp, sizeof(tmp), 0);
            send(socket, msg.c_str(), msg.size(), 0);
        }
        virus_signatures.close();
    } else {
        recv(socket, tmp, sizeof(tmp), 0);
        send(socket, "Error occurred during opening virus signatures file\n", 52, 0);
        return;
    }
}

int main() {
    // INIT SERVER
    int server_fd, new_socket;
    long valread;
    struct sockaddr_in address{};
    int opt = 1;
    int addrlen = sizeof(address);
    char user_input[1024] = {0};
    char tmp[4] = {0};

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

    while (true) {

        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        valread = recv(new_socket, user_input, 1024, 0);
        if (valread < 0) {
            perror("Receive error");
        } else if (valread == 0) {
            cout << "No user input" << endl;
        } else {

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

            string scan_output;

            if (flag == "-s") {
                // TODO: Not implemented yet
                send(new_socket, "Last scan statistics\n", 30, 0);
            } else if (flag == "-sall") {
                // TODO: Not implemented yet
                send(new_socket, "Scanning statistics\n", 30, 0);
            } else if (flag == "-f") {
                send(new_socket, "Scanning file...\n", 17, 0);
                scan(flag, path, new_socket);
            } else if (flag == "-dr") {
                send(new_socket, "Scanning directory recursively...\n", 34, 0);
                scan(flag, path, new_socket);
            } else if (flag == "-dl") {
                send(new_socket, "Scanning directory linearly...\n", 31, 0);
                scan(flag, path, new_socket);
            } else {
                send(new_socket, "Something went wrong\n", 21, 0);
            }
            recv(new_socket, tmp, sizeof(tmp), 0);
            send(new_socket, "Scan finished", 13, 0);

            memset(user_input, 0, strlen(user_input));
        }
    }
}
