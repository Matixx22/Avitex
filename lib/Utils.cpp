//
// Created by mateusz on 18.03.2021.
//

#include <array>
#include <memory>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <csignal>
#include <ftw.h>
#include <syslog.h>

static std::string exec(const char *cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

static void skeleton_daemon() {
    // Define variables
    pid_t pid, sid;

    // Fork the current process
    pid = fork();
    // The parent process continues with a process ID greater than 0
    if(pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
        // A process ID lower than 0 indicates a failure in either process
    else if(pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    // The parent process has now terminated, and the forked child process will continue
    // (the pid of the child process was 0)

    // Since the child process is a daemon, the umask needs to be set so files and logs can be written
    umask(0);

    // Open system logs for the child process
    openlog("Avitex", LOG_NOWAIT | LOG_PID, LOG_USER);
    syslog(LOG_NOTICE, "Successfully started Avitex");

    // Generate a session ID for the child process
    sid = setsid();
    // Ensure a valid SID for the child process
    if(sid < 0)
    {
        // Log failure and exit
        syslog(LOG_ERR, "Could not generate session ID for child process");

        // If a new session ID could not be generated, we must terminate the child process
        // or it will be orphaned
        exit(EXIT_FAILURE);
    }

    // Change the current working directory to a directory guaranteed to exist
    if((chdir("/")) < 0)
    {
        // Log failure and exit
        syslog(LOG_ERR, "Could not change working directory to /");

        // If our guaranteed directory does not exist, terminate the child process to ensure
        // the daemon has not been hijacked
        exit(EXIT_FAILURE);
    }

    // A daemon cannot use the terminal, so close standard file descriptors for security reasons
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}
