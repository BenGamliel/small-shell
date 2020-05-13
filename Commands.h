#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

int _parseCommandLine(const char* cmd_line, char** args);
bool _isBackgroundComamnd(const char* cmd_line);
void _removeBackgroundSign(char* cmd_line);
void freeParseArgs(char** args,int numOfArgs);

class Command {
    const char* _cmd_line;
public:
    Command(const char* cmd_line):_cmd_line(cmd_line){};
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    const char* getCmdLine(){
        return _cmd_line;
    }
};

class BuiltInCommand : public Command {
    char* _args[COMMAND_MAX_ARGS];
    int _numArgs;
public:
    BuiltInCommand(const char* cmd_line):Command(cmd_line) {
        if(cmd_line!=NULL) {
            char clean_cmd[COMMAND_ARGS_MAX_LENGTH];
            bool isBackGround = _isBackgroundComamnd(cmd_line);
            if (isBackGround) {
                strcpy(clean_cmd, this->getCmdLine());
                _removeBackgroundSign(clean_cmd);
                _numArgs = _parseCommandLine(clean_cmd, _args);
            } else {
                _numArgs = _parseCommandLine(cmd_line, _args);
            }
        }
    }
    virtual ~BuiltInCommand() {
        if(this->getCmdLine()!=NULL) {
            freeParseArgs(_args, _numArgs);
        }
    }
    char** getArgs() {
        return this->_args;
    }
    int getNumArgs()const{
        return _numArgs;
    }
    virtual void execute() = 0;

};


/*
class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};
*/
class ChangeDirCommand : public BuiltInCommand {
    char** _plastPwd;
public:
    ChangeDirCommand(const char* cmd_line,char** plastPwd):BuiltInCommand(cmd_line)
            ,_plastPwd(plastPwd){};

    ~ChangeDirCommand() = default;
    void execute() override;

    void setPlastPwd(char* path){
//        char** temp = this->_plastPwd;
        *(this->_plastPwd) = (char*)(malloc(strlen(path)+1));
        strcpy(*(this->_plastPwd),path);
//        delete(*(temp));
    }
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line):BuiltInCommand(cmd_line){};
    ~GetCurrDirCommand() = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char* cmd_line):BuiltInCommand(cmd_line){};
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {

public:
// TODO: Add your data members public:
    QuitCommand(const char* cmd_line):BuiltInCommand(cmd_line){};
    virtual ~QuitCommand() = default;
    void execute() override;
};

class CommandsHistory {// this is the functions wrapper

protected:
    class CommandHistoryEntry {//data itself
        const char* _cmd_line;
        int _seqNum;
    public:
        CommandHistoryEntry(const char* cmd_line, int seqNum):_cmd_line(cmd_line),_seqNum(seqNum){}
        ~CommandHistoryEntry() = default;


        const char* getCmdLine() {
            return this->_cmd_line;
        }

        void incSeqNum () {
            this->_seqNum++;
        }

        int getSeqNum () {
            return this->_seqNum;
        }
    };
private:
    std::vector<CommandHistoryEntry> _queue;
    int _counter;
public:
    CommandsHistory():_queue(std::vector<CommandHistoryEntry>()),_counter(1){}
    ~CommandsHistory() {
        _queue.clear();
    }
    void addRecord(const char* cmd_line);
    void printHistory();
};

class HistoryCommand : public BuiltInCommand {// wrapper
    CommandsHistory* history;
public:
    explicit HistoryCommand(CommandsHistory* history):BuiltInCommand(nullptr),history(history)
    {}

    virtual ~HistoryCommand() = default;
    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        int _jobId;
        bool _isStopped;
        time_t _time;
        pid_t _pid;
        const char* cmd_line;
    public:
        JobEntry(int jobId, bool isStopped, time_t time, pid_t procId, const char* cmd_line):
                _jobId(jobId),_isStopped(isStopped),_time(time),_pid(procId),cmd_line(cmd_line){}
        ~JobEntry() = default;
        JobEntry& operator=(const JobEntry& entry) {
            this->_jobId = entry._jobId;
            this->_isStopped = entry._isStopped;
            this->_time = entry._time;
            this->_pid = entry._pid;
            char* entryCmdLine = (char*)(malloc(strlen(entry.cmd_line)+1));
            strcpy(entryCmdLine,entry.cmd_line);
            this->cmd_line = entryCmdLine;
            return *this;
        }
        pid_t getPid() {
            return this->_pid;
        }
        int getJobId() {
            return this->_jobId;
        }
        bool getIsStopped() {
            return this->_isStopped;
        }
        void setIsStopped (bool status) {
            this->_isStopped = status;
        }
        time_t getTime() {
            return this->_time;
        }
        void setTime(time_t newTime) {
            this->_time = newTime;
        }
        const char* getCmdLine() {
            return this->cmd_line;
        }
    };

private:
    std::vector<JobEntry> _jobsList;
    std::vector<JobEntry> _listForPrint;
    int _counter;
public:
    JobsList(): _jobsList(std::vector<JobEntry>()),_listForPrint(std::vector<JobEntry>()),
                _counter(1){}
    ~JobsList() {
        _jobsList.clear();
    }
    void addJob(const char* cmd_line, pid_t pid, bool stopped, int jobId);
    void printJobsList();
    void prepareListForPrint();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    void printBeforeKillAll();
    int getNewJobId() {
        int max = 0;
        for (int i=0;i<(int)(_jobsList.size());i++) {
            if ((_jobsList.begin()+i)->getJobId() > max) {
                max = (_jobsList.begin()+i)->getJobId();
            }
        }
        return (max+1);
    }
};

class ExternalCommand : public Command {
    const char* _cmd_line;
    bool _isBackGround;
    JobsList* _jobs;
    JobsList::JobEntry** _fgJob;
public:
    ExternalCommand(const char* cmd_line,bool isBackGround,JobsList* jobs,
                    JobsList::JobEntry** fgJob):Command(cmd_line),_cmd_line(cmd_line),
                                                _isBackGround(isBackGround),_jobs(jobs),
                                                _fgJob(fgJob){};
    virtual ~ExternalCommand() {}
    void execute() override;
    const char* getCmdLine(){
        return _cmd_line;
    }
    bool isBackGround(){
        return _isBackGround;
    }
};

class JobsCommand : public BuiltInCommand {
    JobsList* _jobs;
public:
    JobsCommand(const char* cmd_line,JobsList* jobs):BuiltInCommand(cmd_line),_jobs(jobs){};
    virtual ~JobsCommand()= default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* _jobs;
public:
    KillCommand(const char* cmd_line,JobsList* jobs):BuiltInCommand(cmd_line),_jobs(jobs){};

    virtual ~KillCommand() = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* _jobs;
    JobsList::JobEntry** _fgJob;
public:
    ForegroundCommand(const char* cmd_line,JobsList* jobs, JobsList::JobEntry** fgJob):
            BuiltInCommand(cmd_line),_jobs(jobs),_fgJob(fgJob){};

    virtual ~ForegroundCommand() = default;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* _jobs;
public:
    BackgroundCommand(const char* cmd_line,JobsList* jobs):BuiltInCommand(cmd_line),_jobs(jobs){};
    virtual ~BackgroundCommand() = default;
    void execute() override;
};

class CopyCommand : public BuiltInCommand {
public:
    CopyCommand(const char* cmd_line):BuiltInCommand(cmd_line){};
    virtual ~CopyCommand()= default;
    void execute() override;
};


class SmallShell {//Smash
private:
    char *_plastPwd;
    CommandsHistory* _commandHistory;
    JobsList* _jobsList;
    JobsList::JobEntry* _fgJob;
    SmallShell();


public:
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    JobsList* getJobsList() {
        return this->_jobsList;
    }
    JobsList::JobEntry* getFgJob () {
        return this->_fgJob;
    }
};

#endif //SMASH_COMMAND_H_
