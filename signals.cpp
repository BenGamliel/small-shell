#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    printf("smash: got ctrl-Z\n");
    SmallShell& shell = SmallShell::getInstance();
    if(shell.getFgJob() != nullptr) {
        //if (shell.getFgJob()->getPid() != -1) {
        shell.getJobsList()->addJob(shell.getFgJob()->getCmdLine(),shell.getFgJob()->getPid(),
                                    true,shell.getFgJob()->getJobId());
        kill(shell.getFgJob()->getPid(),19);
        printf("smash: process %d was stopped\n",int(shell.getFgJob()->getPid()));
        return;
    }
    //printf("smash: process was stopped\n");
}

void ctrlCHandler(int sig_num) {
    printf("smash: got ctrl-C\n");
    SmallShell& shell = SmallShell::getInstance();
    if(shell.getFgJob() != nullptr) {
        //if (shell.getFgJob()->getPid() != -1) {
        kill(shell.getFgJob()->getPid(),9);
        printf("smash: process %d was killed\n",int(shell.getFgJob()->getPid()));
        return;
    }
    //printf("smash: process was killed\n");
}
