#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "shellutil/stringutil.hpp"
#include "shellutil/vecutil.hpp"
#include "shellutil/colors.hpp"
#include "shellutil/sysutil.hpp"
#include "shellutil/shellexec.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    setenv("OLDPWD", fs::current_path().c_str(), 1);
    setenv("PWD", fs::current_path().c_str(), 1);
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
        std::string formattedcwd  = prettyPath(cwd_color + cwd);
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

        for (char** arg = args; *arg != nullptr; ++arg) {
            *arg = const_cast<char*>(replaceSubstring(*arg, "~", homePath().string()).c_str());
        }

        // UGHHHHH
        if (cmd == "cd") {
            std::string params = join(args+1);
            if (params == "-"){
                char* dest = std::getenv("OLDPWD");
                setenv("OLDPWD", fs::current_path().c_str(), 1);
                fs::current_path(dest);
                setenv("PWD", fs::current_path().c_str(), 1);
                continue;
            }
            fs::path path = params;
            
            path = path.is_absolute() ? path : fs::absolute(path);

            if (fs::exists(path) && fs::is_directory(path)) {
                setenv("OLDPWD", fs::current_path().c_str(), 1);
                fs::current_path(path);
                setenv("PWD", fs::current_path().c_str(), 1);
            } else {
                errno = 2;
                perror("cd");
            }
        } else if (cmd == "exit") {
            exit(0);
        } else {
            shellexec(cmd, args);
        }
    }
    
    return 0;
}