#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (80)
#define COMMAND_MAX_ARGS (20)

class Command {
public:
    Command(const char* cmd_line);
    virtual ~Command() {}
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    virtual int pid();
    const char *cmd_line() const;

    class CommandError {
        std::string _message;
    public:
        CommandError(const std::string& message);
        const std::string& what() const;
    };
private:
    char* _cmd_line;
};

#define DECLARE_SMALL_SHELL()                       \
    /* todo: please declare it after JobList */     \
class SmallShell {                                  \
private:                                            \
    SmallShell();                                   \
    friend class BuiltInCommand;                    \
    friend class ExternalCommand;                   \
                                                    \
    std::string _name;                              \
    char *_cwd;                                     \
    bool _cd_called;                                \
    JobsList _job_list;                             \
    int _running_cmd_pid;                           \
    Command* _running_cmd;                          \
                                                    \
public:                                             \
    static SmallShell& getInstance();               \
    SmallShell(SmallShell const&)      = delete;    \
    void operator=(SmallShell const&)  = delete;    \
    ~SmallShell() {}                                \
                                                    \
    Command *CreateCommand(const char* cmd_line);   \
    void executeCommand(const char* cmd_line);      \
    const std::string& name() const;                \
    void handle_ctrl_z(int sig_num);                \
};


class SmallShell;
class BuiltInCommand : public Command {
protected:
    SmallShell *_smash;
    std::string& smash_name();
    char *smash_cwd();
    bool &smash_cd_called();
public:
    BuiltInCommand(const char* cmd_line, SmallShell *smash);
    virtual ~BuiltInCommand() {}
};

class ChpromptCommand : public BuiltInCommand {
private:
    std::string _new_name;
public:
    ChpromptCommand(const char* cmd_line, char* args[], SmallShell *smash);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line, char* args[], SmallShell *smash);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, char* args[], SmallShell *smash);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    char *_new_dir;
public:
    ChangeDirCommand(const char* cmd_line, char* args[], SmallShell *smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
private:
    SmallShell *_smash;
    int _pid;
public:
    ExternalCommand(const char* cmd_line, SmallShell *smash);
    virtual ~ExternalCommand() {}
    void execute() override;
    int pid() override;
};

class JobsList {
public:
    class JobEntry {
    public:
        JobEntry(Command *cmd, bool stopped);
    private:
        int _jid;
        bool _stopped;
        time_t _start;
        Command *_cmd;

        friend JobsList;
    };
    JobsList();
    ~JobsList() {}
    void addJob(Command* cmd, bool stopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
//   TODO: Add extra methods or modify exisitng ones as needed
private:
    std::list<JobEntry *> _jobs;
    int _next_jid;
};

DECLARE_SMALL_SHELL()

class JobsCommand : public BuiltInCommand {
    JobsList *_jobs;
public:
    JobsCommand(const char* cmd_line, SmallShell *smash, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};



#endif //SMASH_COMMAND_H_
