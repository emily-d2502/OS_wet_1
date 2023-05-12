#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell::getInstance().handle_ctrl_z(sig_num);
}

void ctrlCHandler(int sig_num) {
    SmallShell::getInstance().handle_ctrl_c(sig_num);
}

void alarmHandler(int sig_num) {

}
