#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (80)
#define COMMAND_MAX_ARGS (20)

class Command {
public:
    Command() = default;
    Command(const char* cmd_line);
    virtual ~Command() {}
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
};

class SmallShell {
 private:
    SmallShell();
    std::string _name;
    char *_cwd;
    bool _cd_called;

    friend class BuiltInCommand;
 public:
    static SmallShell& getInstance() {
        static SmallShell instance;
        return instance;
    }
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete;
    void operator=(SmallShell const&)  = delete;
    ~SmallShell();
    void executeCommand(const char* cmd_line);
};

class BuiltInCommand : public Command {
protected:
    SmallShell *_smash;
    std::string& smash_name();
    char *smash_cwd();
    bool &smash_cd_called();
public:
    BuiltInCommand(SmallShell *smash);
    virtual ~BuiltInCommand() {}
};

class ChpromptCommand : public BuiltInCommand {
private:
    std::string _new_name;
public:
    ChpromptCommand(char* args[], SmallShell *smash);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(char* args[], SmallShell *smash);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(char* args[], SmallShell *smash);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    char *_new_dir;
public:
    ChangeDirCommand(char* args[], SmallShell *smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};


#endif //SMASH_COMMAND_H_
