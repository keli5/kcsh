#include "shellutil/colors.hpp"
#include "shellutil/pluginloader.hpp"
#include "shellutil/settingsutil.hpp"
#include "shellutil/shellexec.hpp"
#include "shellutil/stringutil.hpp"
#include "shellutil/sysutil.hpp"
#include "shellutil/vecutil.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h> // This IS used wtf?

namespace fs = std::filesystem;

fs::path settingsDir = homePath().string() + "/.config/kcsh/";
fs::path themesDir   = settingsDir.string() + "/themes/";
extern std::vector<std::pair<std::string, Replacement>> promptReplacementMapping;

int main([[gnu::unused]] int argc, char** argv) {

    setenv("OLDPWD", fs::current_path().c_str(), 1);
    setenv("PWD", fs::current_path().c_str(), 1);
    
    IniData settings;
    IniData theme;

    fs::path pluginsPath = settingsDir.string() + "/plugins";

    fs::create_directory(homePath().string() + "/.config");
    fs::create_directory(settingsDir);
    fs::create_directory(themesDir);
    fs::create_directory(pluginsPath);

    PluginLoader pluginLoader;
    for (const auto& entry : fs::directory_iterator(pluginsPath)) {
        if (entry.path().filename().extension() == ".so") {
            #ifdef NDEBUG
                std::cout << "Loading plugin " << entry.path().string() << std::endl;
            #endif
            pluginLoader.loadPlugin(entry.path());
        }
    }

    settings = parseIniFile(settingsDir.string() + "/kcsh_config.ini");
    
    if (settings.empty()) {
        #ifdef NDEBUG
            std::cout << "Creating default config in " + settingsDir.string() << std::endl;
        #endif
        settings = getDefaultConfig();

        saveIniFile(settings, settingsDir.string() + "/kcsh_config.ini");
    }

    fs::path themePath = themesDir.string() + "/" + getIniValue(settings, "appearance", "theme") + ".ini";
    theme = parseIniFile(themePath);
    setenv("KCSH_THEME", replaceSubstring(themePath.filename(), ".ini", "").c_str(), true);
    
    if (theme.empty()) {
        #ifdef NDEBUG
            std::cout << "Creating " + getIniValue(settings, "appearance", "theme") + " theme in " + themesDir.string() << std::endl;
        #endif
        theme = getDefaultTheme();
        saveIniFile(theme, themePath);
    }

    // Set $SHELL
    char srlbuffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", srlbuffer, sizeof(srlbuffer)-1);
    if (len != -1) {
        srlbuffer[len] = '\0';
        setenv("SHELL", srlbuffer, 1);
    } else {
        std::cerr << "Not setting $SHELL - could not get absolute path";
    }


    while (true) {

        std::string prompt_color        = FG_WHITE;
        std::string prompt_character    = getIniValue(theme, "prompt", "promptcharacter") + " ";

        std::string cwd         = prettyPath(trim(fs::current_path()));
        std::string cwd_color   = getIniValue(theme, "colors", "path");

        std::string user        = trim(sysexec("whoami"));
        std::string user_color  = getIniValue(theme, "colors", "username");

        std::string hostname        = trim(sysexec("cat /proc/sys/kernel/hostname"));
        std::string hostname_color  = getIniValue(theme, "colors", "hostname");;

        std::string cmd = "";

        char **args;



        std::string prompt = parsePromptFormat(getIniValue(theme, "prompt", "format"), prompt_character, cwd, cwd_color,
                                                user, user_color, hostname, hostname_color, pluginLoader);

        std::cout << prompt;

        std::getline(std::cin, cmd);
        if (std::cin.eof()) {
            exit(0);
        }

        auto [environment, remainingCommand] = parseEnvVars(cmd);

        for (const auto& pair : environment) {
            setenv(pair.first.c_str(), pair.second.c_str(), 1);
        }

        cmd = remainingCommand;

        std::vector<std::string> tokens = split(&cmd);
        if (tokens.empty()) {
            continue;
        }

        std::vector<const char *> cstringTokens = cstringArray(tokens);
        cmd = cstringTokens[0];
        args = const_cast<char **>(cstringTokens.data());

        for (int i = 0; args[i] != nullptr; ++i) {
            std::string modifiedArgument = std::string(args[i]);
            modifiedArgument = replaceSubstring(modifiedArgument, "~", homePath());
            modifiedArgument = replaceEnvironmentVariables(modifiedArgument);
            char* modifiedArgumentCstr = new char[modifiedArgument.size() + 1];
            std::strcpy(modifiedArgumentCstr, modifiedArgument.c_str());

            args[i] = modifiedArgumentCstr;
        }

        // FIXME: are some of these vectors redundant
        // FIXME: add support for quotes and escaping
        std::vector<char *> command;
        for (char **arg = args; *arg != nullptr; ++arg) {
            if (strcmp(*arg, "||") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    printf("%s: Syntax error\n", *argv);
                    command.clear();
                    break;
                }
                if (runCommand(command.data()) != 0) {
                    command.clear();
                    continue;
                }
                command.clear();
                break;
            } else if (strcmp(*arg, ";") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    //there is no error with just a semicolon
                    command.clear();
                    break;
                }
                runCommand(command.data());
                command.clear();
                continue;
            } else if (strcmp(*arg, "&&") == 0) {
                command.push_back(nullptr);
                if (command.at(0) == nullptr){
                    printf("%s: Syntax error\n", *argv);
                    command.clear();
                    break;
                }
                if (runCommand(command.data()) == 0) {
                    command.clear();
                    continue;
                }
                command.clear();
                break;
            } else {
                command.push_back(*arg);
            }
        }
        // run command if no thingiers were found
        if (command.size() > 0 && command.at(0)!=nullptr) {
            command.push_back(nullptr);
            runCommand(command.data());
        }
    }

    return 0;
}