#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string>
#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (80)
#define COMMAND_MAX_ARGS (20)

class SmallShell;
class Command {
public:
    Command(const char* cmd_line);
    virtual ~Command() {}
    virtual void execute() = 0;
    pid_t pid();
    int _jid;
    const char *cmd_line();

    class CommandError;
private:
    char* _cmd_line;
protected:
    SmallShell *_smash;
    pid_t _pid;
};

class Command::CommandError {
private:
    std::string _message;
public:
    CommandError(const std::string& message);
    const std::string& what() const;
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
    Command* _running_cmd;                          \
                                                    \
public:                                             \
    static SmallShell& getInstance();               \
    SmallShell(SmallShell const&)      = delete;    \
    void operator=(SmallShell const&)  = delete;    \
    ~SmallShell() {}                                \
                                                    \
    Command *CreateCommand(const char* cmd_line);   \
    bool executeCommand(const char* cmd_line);      \
    const std::string& name() const;                \
    void handle_ctrl_z(int sig_num);                \
    void handle_ctrl_c(int sig_num);                \
};


class BuiltInCommand : public Command {
protected:
    std::string& smash_name();
    char *smash_cwd();
    bool &smash_cd_called();
    Command* &smash_running_cmd();
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() {
        delete[] _args;
        delete _command;
    }
    void execute() override;
private:
    bool _background_cmd;
    char** _args;
    char* _command;
};

class ChpromptCommand : public BuiltInCommand {
private:
    std::string _new_name;
public:
    ChpromptCommand(const char* cmd_line, char* args[]);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line, char* args[]);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, char* args[]);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    char *_new_dir;
public:
    ChangeDirCommand(const char* cmd_line, char* args[]);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class JobsList {
public:
    JobsList();
    JobsList(const JobsList& jl)            = delete;
    JobsList& operator=(const JobsList& jl) = delete;
    ~JobsList()                             = default;

    void addJob(Command* cmd, bool stopped = false);
    void removeJobById(int jobId);
    void removeFinishedJobs();
    void printJobsList();
    void killAllJobs();

    class JobEntry;
    JobEntry *getJobById(int jobId);
    JobEntry *getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);

private:
    std::list<JobEntry *> _jobs;
    int _next_jid;
};

class JobsList::JobEntry {
public:
    JobEntry(Command *cmd, bool stopped);
    JobEntry(const JobEntry&)       = delete;
    void operator=(const JobEntry&) = delete;
    ~JobEntry()                     = default;

    Command *cmd();
    bool &stopped();
    pid_t pid() const;

private:
    int _jid;
    pid_t _pid;
    bool _stopped;
    time_t _start;
    Command *_cmd;

    friend JobsList;
};

DECLARE_SMALL_SHELL()

class JobsCommand : public BuiltInCommand {
    JobsList *_jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const char* cmd_line, char* args[], JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
private:
    Command *_cmd;
};

class BackgroundCommand : public BuiltInCommand {
public:
    BackgroundCommand(const char* cmd_line, char* args[], JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
private:
    Command *_cmd;
};

class QuitCommand : public BuiltInCommand {
    bool _kill;
    JobsList* _jobs;
public:
    QuitCommand(const char* cmd_line, char* args[], JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char* cmd_line, char* args[], JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
private:
    pid_t _pid;
    int _signum;
};

class RedirectionCommand : public Command {
public:
    RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
private:
    std::string _filename;
    Command *_cmd;
    bool _append;
};

class PipeCommand : public Command {
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
private:
    Command* _cmds[2];
};

class GetFileTypeCommand : public BuiltInCommand {
public:
    GetFileTypeCommand(const char* cmd_line, char* args[]);
    virtual ~GetFileTypeCommand() {}
    void execute() override;
private:
    char* _path;
};

class ChmodCommand : public BuiltInCommand {
public:
    ChmodCommand(const char* cmd_line, char* args[]);
    virtual ~ChmodCommand() {}
    void execute() override;
private:
    mode_t _new_mode;
    char* _path;
};

class SetcoreCommand : public BuiltInCommand {
public:
    SetcoreCommand(const char* cmd_line, char* args[], JobsList* jobs);
    virtual ~SetcoreCommand() {}
    void execute() override;
private:
    int _core;
    pid_t _pid;
};

#endif //SMASH_COMMAND_H_