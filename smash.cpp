#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

using namespace std;


int main(int argc, char* argv[]) {
    if (signal(SIGTSTP , ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    // if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
    //     perror("smash error: failed to set ctrl-C handler");
    // }

    // char* args[] = {"ls", NULL};
    // execvp("ls", args);
    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        cout << smash.name();
        string cmd_line;
        getline(std::cin, cmd_line);
        if (cmd_line.empty()) {
            continue;
        }
        if (cmd_line == "q") {
            break;
        }
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}