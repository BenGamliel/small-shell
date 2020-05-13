#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <time.h>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include "signals.h"
#include <fstream>
#include <ostream>
#include <cstdio>
#include <fcntl.h>
#define SHELL "/bin/bash"


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";
string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = nullptr;
    }
    return i;

    FUNC_EXIT()
}

void freeParseArgs(char** args,int numOfArgs){
    if(!args){
        return;
    }
    for(int i=0;i<numOfArgs;i++){//if num of args ==1 this should delete the first char* that hold the command pwd,quit, etc..
        delete args[i];
        args[i]=nullptr;
    }
}


bool _isBackgroundComamnd(const char* cmd_line) {
    const string whitespace = " \t\n";
    const string str(cmd_line);
    return str[str.find_last_not_of(whitespace)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string whitespace = " \t\n";
    const string str(cmd_line);
    // find last character other than spaces
    size_t idx = str.find_last_not_of(whitespace);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(whitespace, idx-1) + 1] = 0;
}

SmallShell::SmallShell() {
    _plastPwd = nullptr;
    _commandHistory = new CommandsHistory();
    _jobsList = new JobsList();
    _fgJob = nullptr;
}

SmallShell::~SmallShell() {
    delete _commandHistory;
    delete _jobsList;
    if (this->_plastPwd != nullptr) {
        delete(this->_plastPwd);
    }
    if (this->_fgJob != nullptr) {
        delete this->_fgJob;
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    bool isBackGround=_isBackgroundComamnd(cmd_line);
    int numofArgs=_parseCommandLine(cmd_line, args);
    string cmd_s = args[0];
    if (cmd_s.compare("pwd") == 0) {
        freeParseArgs(args,numofArgs);
        return new GetCurrDirCommand(nullptr);
    } else if (cmd_s.compare("cd") == 0) {
        freeParseArgs(args,numofArgs);
        return new ChangeDirCommand(cmd_line, &(this->_plastPwd));
    } else if (cmd_s.compare("history") == 0) {
        freeParseArgs(args,numofArgs);
        return new HistoryCommand(this->_commandHistory);
    } else if (cmd_s.compare("jobs") == 0) {
        freeParseArgs(args,numofArgs);
        this->_jobsList->removeFinishedJobs();
        return new JobsCommand(cmd_line, this->_jobsList);
    } else if (cmd_s.compare("kill") == 0) {
        freeParseArgs(args,numofArgs);
        this->_jobsList->removeFinishedJobs();
        return new KillCommand(cmd_line, this->_jobsList);
    } else if (cmd_s.compare("showpid") == 0) {
        freeParseArgs(args,numofArgs);
        return new ShowPidCommand(nullptr);
    } else if (cmd_s.compare("fg") == 0) {
        freeParseArgs(args,numofArgs);
        this->_jobsList->removeFinishedJobs();
        return new ForegroundCommand(cmd_line, this->_jobsList,&(this->_fgJob));
    } else if (cmd_s.compare("bg") == 0) {
        freeParseArgs(args,numofArgs);
        this->_jobsList->removeFinishedJobs();
        return new BackgroundCommand(cmd_line, this->_jobsList);
    } else if (cmd_s.compare("quit") == 0) {
        freeParseArgs(args,numofArgs);
        this->_jobsList->removeFinishedJobs();
        return new QuitCommand(cmd_line);
    } else if (cmd_s.find("cp") == 0) {
        freeParseArgs(args,numofArgs);
        return new CopyCommand(cmd_line);
    }
    return new ExternalCommand(cmd_line,isBackGround,this->_jobsList,&(this->_fgJob));


}

void SmallShell::executeCommand(const char *cmd_line) {
    this->_commandHistory->addRecord(cmd_line);
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    this->_jobsList->removeFinishedJobs();
    delete cmd;
}

int strToInt (const char* str) {
    int num = 0;
    for (int i=0;i<(int)(strlen(str));i++) {
        if ((str[i] < '0') || (str[i] > '9')) {
            return -1;
        }
        const char c = (str[i]);
        int adding = 0;
        adding = atoi(&c);
        num = (num*10) + adding;
    }
    return num;

}

void GetCurrDirCommand::execute() {
    char* path = getcwd(nullptr,0);
    if (path == nullptr) {
        perror("smash error: getcwd failed");
        return;
    }
    printf("%s\n", path);
    free(path);
}

void ChangeDirCommand::execute() {
    char** args = this->getArgs();

    if (this->getNumArgs() > 2) {
        printf("smash error: cd: too many arguments\n");
        return;
    }
    char* currentPath = getcwd(nullptr,0);

    if (currentPath == nullptr) {//getcwd failed
        perror("smash error: getcwd failed");
        return;
    }
    if (strcmp(args[1],"-") == 0) {

        if (*(this->_plastPwd) == nullptr) {
            printf("smash error: cd: OLDPWD not set\n");
            free(currentPath);
            return;
        }
        else { //_plastpwd not null
            if (chdir(*(this->_plastPwd)) == -1) {  //changingDir failed
                perror("smash error: chdir failed");
                free(currentPath);
                return;
            }
        }
    }
    else if (chdir(args[1]) == -1) {
        perror("smash error: chdir failed");
        free(currentPath);
        return;
    }
    this->setPlastPwd(currentPath);
}

void ShowPidCommand::execute() {
    printf("smash pid is %d\n", getpid());
}

void CommandsHistory::addRecord(const char *cmd_line) {
    if (!this->_queue.empty()) {
        if (strcmp(this->_queue.back().getCmdLine(),cmd_line) == 0) {
            this->_queue.back().incSeqNum();
            this->_counter++;
            return;
        }
    }
    char* entryCmdLine = (char*)(malloc(strlen(cmd_line)+1));
    strcpy(entryCmdLine,cmd_line);
    CommandHistoryEntry* entry = new CommandHistoryEntry(entryCmdLine,this->_counter);
    this->_queue.push_back(*entry);
    this->_counter++;
    if (this->_queue.size() > HISTORY_MAX_RECORDS) {
        this->_queue.erase(this->_queue.begin());
    }
}

void CommandsHistory::printHistory() {
    for (int i=0;i<(int)(this->_queue.size());i++) {
        cout<<right<<setw(5)<<(this->_queue.begin()+i)->getSeqNum()<<"  "<<(this->_queue.begin()+i)
                ->getCmdLine()<<endl;
    }
}

void HistoryCommand::execute() {
    this->history->printHistory();
}

void JobsList::removeFinishedJobs() {
    pid_t num;
    for (int i=0;i<(int)(this->_jobsList.size());i++) {
        num = waitpid((this->_jobsList.begin()+i)->getPid(),nullptr,WNOHANG);
        if (num == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        if (num > 0) {
            this->_jobsList.erase(this->_jobsList.begin()+i);
        }
    }
}

void JobsList::killAllJobs() {
    std::string s = "-9";
    std::string cmdStr = "kill -9 ";
    KillCommand* cmd;
    for (int i=0;i<(int)(this->_listForPrint.size());i++) {
        cmdStr.append(std::to_string((this->_listForPrint.begin()+i)->getJobId()));
        cmd = new KillCommand(cmdStr.c_str(),this);
        cmd->execute();
        delete cmd;
        cmdStr = "kill -9 ";
    }
    this->_listForPrint.clear();
}

void JobsList::prepareListForPrint() {
    this->_listForPrint = this->_jobsList;
    for(int i=0;i<(int)(this->_listForPrint.size());i++) {
        int min = i;
        for (int j=i;j<(int)(this->_listForPrint.size());j++) {
            if (this->_listForPrint.begin()[min].getJobId() > this->_listForPrint.begin()[j]
                    .getJobId()) {
                min = j;
            }
        }
        if (min != i) {
            /*JobEntry* temp = new JobEntry(this->_listForPrint.begin()[i].getJobId(),
                                          this->_listForPrint.begin()[i].getIsStopped(),
                                          this->_listForPrint.begin()[i].getTime(),
                                          this->_listForPrint.begin()[i].getPid (),
                                          this->_listForPrint.begin()[i].getCmdLine());*/
            JobEntry temp = this->_listForPrint.begin()[i];
            /*cout << this->_listForPrint.begin()[i].getJobId() << " " <<
                    this->_listForPrint.begin()[i].getIsStopped() << " " <<
                    this->_listForPrint.begin()[i].getTime() << " " <<
                    this->_listForPrint.begin()[i].getPid () << " " <<
                    this->_listForPrint.begin()[i].getCmdLine() << endl;
            cout << temp.getJobId() << " " <<
                    temp.getIsStopped() << " " <<
                    temp.getTime() << " " <<
                    temp.getPid () << " " <<
                    temp.getCmdLine() << endl;
            cout << this->_listForPrint.begin()[min].getJobId() << " " <<
                 this->_listForPrint.begin()[min].getIsStopped() << " " <<
                 this->_listForPrint.begin()[min].getTime() << " " <<
                 this->_listForPrint.begin()[min].getPid () << " " <<
                 this->_listForPrint.begin()[min].getCmdLine() << endl;*/
            this->_listForPrint.begin()[i] = this->_listForPrint.begin()[min];
            this->_listForPrint.begin()[min] = temp;

            /*cout << this->_listForPrint.begin()[i].getJobId() << " " <<
                 this->_listForPrint.begin()[i].getIsStopped() << " " <<
                 this->_listForPrint.begin()[i].getTime() << " " <<
                 this->_listForPrint.begin()[i].getPid () << " " <<
                 this->_listForPrint.begin()[i].getCmdLine() << endl;
            cout << temp.getJobId() << " " <<
                 temp.getIsStopped() << " " <<
                 temp.getTime() << " " <<
                 temp.getPid () << " " <<
                 temp.getCmdLine() << endl;
            cout << this->_listForPrint.begin()[min].getJobId() << " " <<
                 this->_listForPrint.begin()[min].getIsStopped() << " " <<
                 this->_listForPrint.begin()[min].getTime() << " " <<
                 this->_listForPrint.begin()[min].getPid () << " " <<
                 this->_listForPrint.begin()[min].getCmdLine() << endl;*/

        }
    }
}

void JobsList::printJobsList() {
    this->prepareListForPrint();
    for (int i=0;i<(int)(this->_listForPrint.size());i++) {
        time_t currTime = time(nullptr);
        if (int(currTime) == -1) {
            perror("smash error: time failed");
            return;
        }
        double diff = difftime(currTime,(this->_listForPrint.begin()+i)->getTime());
        if ((this->_listForPrint.begin()+i)->getIsStopped()) {
            cout<<right<<"["<<(this->_listForPrint.begin()+i)->getJobId()<<"] "<<(this->_listForPrint.begin
                    ()+i)->getCmdLine()<<" : "<<(this->_listForPrint.begin()+i)->getPid()<<" "<<diff<<
                " secs (stopped)"<< endl;
        }
        else {
            cout<<right<<"["<<(this->_listForPrint.begin()+i)->getJobId()<<"] "<<(this->_listForPrint.begin
                    ()+i)->getCmdLine()<<" : "<<(this->_listForPrint.begin()+i)->getPid()<<" "<<diff<<
                " secs"<< endl;
        }
    }
    this->_listForPrint.clear();
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for (int i=0;i<(int)(this->_jobsList.size());i++) {
        if ((this->_jobsList.begin()+i)->getJobId() == jobId) {
            return (&(this->_jobsList.begin()[i]));
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (int i=0;i<(int)(this->_jobsList.size());i++) {
        if ((this->_jobsList.begin()+i)->getJobId() == jobId) {
            this->_jobsList.erase(this->_jobsList.begin()+i);
        }
    }
}

void JobsList::addJob(const char* cmd_line, pid_t pid, bool stopped, int jobId) {
    int newJobId = jobId;
    if (newJobId == -1) {
        newJobId = this->getNewJobId();
    }
    time_t currTime = time(nullptr);
    if (int(currTime) == -1) {
        perror("smash error: time failed");
        return;
    }
    char* entryCmdLine = (char*)(malloc(strlen(cmd_line)+1));
    strcpy(entryCmdLine,cmd_line);
    JobEntry* job = new  JobEntry(newJobId, stopped, currTime, pid, entryCmdLine);
    this->_jobsList.push_back(*job);
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
    if (this->_jobsList.empty()) {
        *lastJobId = -1;
        return nullptr;
    }
    *lastJobId = this->_jobsList.back().getJobId();
    return &(this->_jobsList.back());
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
    for (int i=(int)((this->_jobsList.size())-1);i>=0;i--) {
        if ((this->_jobsList.begin()+i)->getIsStopped()) {
            *jobId = (this->_jobsList.begin()+i)->getJobId();
            return (&(this->_jobsList.begin()[i]));
        }
    }
    *jobId = -1;
    return nullptr;
}

void JobsList::printBeforeKillAll() {
    printf("sending SIGKILL signal to %d jobs:\n",(int)(this->_jobsList.size()));
    this->prepareListForPrint();
    for (int i=0;i<(int)(this->_listForPrint.size());i++) {
        printf("%d: %s\n", (int)(this->_listForPrint[i].getPid()),this->_listForPrint[i].getCmdLine());
    }
}

void JobsCommand::execute() {
    this->_jobs->printJobsList();
}

void KillCommand::execute() {
    char **args = this->getArgs();
    int jobId, signum;
    if (args[3] != nullptr) {
        printf("smash error: kill: invalid arguments\n");
        return;
    }
    if (args[1] == nullptr || args[2] == nullptr) {
        printf("smash error: kill: invalid arguments\n");
        return;
    }
    jobId = strToInt(args[2]);
    if (jobId == -1) {
        printf("smash error: kill: invalid arguments\n");
        return;
    }
    JobsList::JobEntry* job= this->_jobs->getJobById(jobId);
    if (job == nullptr) {
        printf("smash error: kill: job-id %d does not exist\n",jobId);
        return;
    }
    signum = strToInt(args[1]+1);
    string arg1 = "-";
    arg1.append(args[1]+1);
    if (strcmp(args[1],arg1.c_str()) != 0) {
        printf("smash error: kill: invalid arguments\n");
        return;
    }
    if (kill(job->getPid(),signum) == -1) {
        perror("smash error: kill failed");
        return;
    }
    if (signum == 19) {
        job->setIsStopped(true);
    }
    if (signum == 18) {
        job->setIsStopped(false);
    }
    printf("signal number %d was sent to pid %d\n", signum, (int)(job->getPid()));
}

void BackgroundCommand::execute() {
    char **args = this->getArgs();
    if (args[2] != nullptr) {
        printf("smash error: bg: invalid arguments\n");
        return;
    }
    JobsList::JobEntry* job;
    int jobId;
    if (args[1] == nullptr) {
        job= this->_jobs->getLastStoppedJob(&jobId);
        if (job == nullptr) {
            printf("smash error: bg: there is no stopped jobs to resume\n");
            return;
        }
    }
    else {
        jobId = strToInt(args[1]);
        if (jobId == -1) {
            printf("smash error: bg: invalid arguments\n");
            return;
        }
        job = this->_jobs->getJobById(jobId);
        if (job == nullptr) {
            printf("smash error: bg: job-id %d does not exist\n",jobId);
            return;
        }
        if (!job->getIsStopped()) {
            printf("smash error: bg: job-id %d is already running in the background\n",jobId);
            return;
        }
    }
    printf("%s : %d\n",job->getCmdLine(),int(job->getPid()));
    if (kill(job->getPid(),18) == -1) {
        perror("smash error: sigcont failed");
        return;
    }
    job->setIsStopped(false);
}

void ForegroundCommand::execute() {
    char **args = this->getArgs();
    JobsList::JobEntry *job;
    int jobId;
    if (args[2] != nullptr) {//more then 1 arg where provided -> invalid arguments
        printf("smash error: fg: invalid arguments\n");
        return;
    }
    if (args[1] == nullptr) {//no arg where given
        job= this->_jobs->getLastJob(&jobId);
        if (job == nullptr) {
            printf("smash error: fg: jobs list is empty\n");
            return;
        }
    }
    else{ //arg[1]-should be a number of job
        jobId = strToInt(args[1]);
        if (jobId == -1) {
            printf("smash error: fg: invalid arguments\n");
            return;
        }
        job = this->_jobs->getJobById(jobId);
        if (job == nullptr){
            printf("smash error: fg: job-id %d does not exist\n",jobId);
            return;
        }
    }
    printf("%s : %d\n",job->getCmdLine(),int(job->getPid()));
    if (kill(job->getPid(),18) == -1) {
        perror("smash error: sigcont failed");
        return;
    }
    char* entryCmdLine = (char*)(malloc(strlen(job->getCmdLine())+1));
    strcpy(entryCmdLine,job->getCmdLine());
    delete *(this->_fgJob);
    *(this->_fgJob) = new JobsList::JobEntry(job->getJobId(),false,-1,job->getPid(),
                                             entryCmdLine);
    pid_t pid = job->getPid();
    this->_jobs->removeJobById(job->getJobId());
    waitpid(pid,nullptr,WUNTRACED);
    delete *(this->_fgJob);
    *(this->_fgJob) = nullptr;
}

void QuitCommand::execute() {
    SmallShell& shell = SmallShell::getInstance();
    if (this->getArgs()[1] != nullptr) {
        if (strcmp(this->getArgs()[1],"kill") == 0) {
            shell.getJobsList()->printBeforeKillAll();
            shell.getJobsList()->killAllJobs();
        }
    }
    exit(0);
}


void CopyCommand::execute() {

    if ((this->getNumArgs()!=3)||(this->getArgs()[1]==NULL)||(this->getArgs()[2]==NULL)) {
        perror("cp: invalid arguments");
        return;
    }
    string in{this->getArgs()[1]};
    int in_res=open(in.c_str(),O_RDONLY);
    if(in_res==-1){
        perror("smash error: open failed");
        return;
    }
    ifstream input{this->getArgs()[1]};
    remove(this->getArgs()[2]);
    ofstream output{this->getArgs()[2]};
    string out{this->getArgs()[2]};
//    int out_res=open(out.c_str(),O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (!output.is_open()) {
        perror("smash error: open failed");
        return;
    }
    static constexpr std::size_t buffsize{1024};
    char buffer[buffsize];
    while (input.read(buffer, buffsize)) {
        output.write(buffer, buffsize);
    }
    output.write(buffer, input.gcount());
    if(!output.good()){
        perror("smash error: write failed");
    }
    input.close();
    if(input.is_open()){
        perror("smash error: close failed");
        return;
    }
    std::cout << "smash: "<<this->getArgs()[1]<<" was copied to "<<this->getArgs()[2] << '\n';


}

void ExternalCommand::execute(){
    char clean_cmd[COMMAND_ARGS_MAX_LENGTH];
    if(this->isBackGround()){
        strcpy(clean_cmd,this->getCmdLine());
        _removeBackgroundSign(clean_cmd);
    }
    pid_t pid=fork();
    if(pid==0){//child
        setpgrp();
        if(this->isBackGround()){//BG cmd
            execl(SHELL,"bash","-c",clean_cmd,NULL);
        }
        else{
            execl(SHELL,"bash","-c",this->getCmdLine(),NULL);
        }
        exit(0);
    }
    else if(pid>0) {//father
        if(this->_isBackGround){
            this->_jobs->addJob(this->_cmd_line,pid,false,this->_jobs->getNewJobId());
            return;
        }
        else{
            char* entryCmdLine = (char*)(malloc(strlen(this->_cmd_line)+1));
            strcpy(entryCmdLine,this->_cmd_line);
            delete *(this->_fgJob);
            *(this->_fgJob) = new JobsList::JobEntry(-1,false,-1,pid,entryCmdLine);
            waitpid(pid,nullptr,WUNTRACED);
            delete *(this->_fgJob);
            *(this->_fgJob) = nullptr;
        }
    }
    else{
        perror("smash error: fork failed\n");
        return;
    }

}

