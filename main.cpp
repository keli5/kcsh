#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "shellutil/stringutil.hpp"
#include "shellutil/vecutil.hpp"
#include "shellutil/colors.hpp"
#include "shellutil/sysexec.hpp"
#include "shellutil/shellexec.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    while (true) {

        std::string prompt_color        = FG_WHITE;
        std::string prompt_character    = "$";

        std::string cwd         = trim(fs::current_path());
        std::string cwd_color   = FG_BLUE;

        std::string user        = trim(sysexec("whoami"));
        std::string user_color  = FG_RED;

        std::string hostname        = trim(sysexec("cat /proc/sys/kernel/hostname"));
        std::string hostname_color  = FG_MAGENTA;

        std::string cmd = "";

        char** args;

        std::string formattedchar = prompt_color + prompt_character + " ";
        std::string formattedcwd  = cwd_color + cwd;
        std::string formatteduser = user_color + user;
        std::string formattedhost = hostname_color + hostname;

        std::string prompt ="[" + formatteduser + RESET + "@" + formattedhost + RESET + 
                            "] " + formattedcwd + RESET + "\n" + formattedchar + RESET;

        std::cout << prompt;
        
        std::getline(std::cin, cmd);
        if (std::cin.eof()){
            exit(0);
        }
        std::vector<std::string> tokens = split(&cmd);
        if (tokens.empty()) {
            continue;
        }

        std::vector<const char*> cstringTokens = cstringArray(tokens);
        cmd = cstringTokens[0];
        args = const_cast<char**>(cstringTokens.data());

        // UGHHHHH
        if (cmd == "cd") {
            fs::path path = join(args+1);
            
            path = path.is_absolute() ? path : fs::absolute(path);

            if (fs::exists(path) && fs::is_directory(path)) {
                fs::current_path(path);
            } else {
                errno = 2;
                perror("cd");
            }
        } else {
            shellexec(cmd, args);
        }
    }
    
    return 0;
}