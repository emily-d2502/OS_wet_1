#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
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
    std::string _name;
    SmallShell();

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
    std::string& name(){
        return _name;
    }
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(): Command() {}
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ChpromptCommand : public BuiltInCommand {
private:
    SmallShell *_smash;
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


#endif //SMASH_COMMAND_H_
