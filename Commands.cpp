#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

#define DO_SYS(syscall)                                     \
do {                                                        \
    /* safely invoke a system call */                       \
    if( (syscall) == -1 ) {                                 \
        perror("smash error: #syscall failed"  );           \
        exit(1);                                            \
    }                                                       \
} while (0)

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

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
    args[++i] = NULL;
  }
  FUNC_EXIT()

  return i;
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  int idx = str.find_last_not_of(WHITESPACE);
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
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

/* -------------- Command -------------- */

Command::Command(const char* cmd_line):
    _cmd_line(cmd_line) {}

Command::CommandError::CommandError(const std::string& message) {
    _message = message;
}

const std::string& Command::CommandError::what() const {
    return _message;
}

int Command::pid() {
    return getpid();
}

const char *Command::cmd_line() const {
    return _cmd_line;
}

/* -------------- Small Shell -------------- */
SmallShell::SmallShell():
    _name("smash> ") {
    _cwd = new char[COMMAND_ARGS_MAX_LENGTH]; 
    _cd_called = false;
}

SmallShell &SmallShell::getInstance() {
    static SmallShell instance;
    return instance;
}


Command *SmallShell::CreateCommand(const char* cmd_line) {
    
    char* args[COMMAND_MAX_ARGS + 1];
    _parseCommandLine(cmd_line, args);
    string firstWord(args[0]);

    if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(cmd_line, args, this);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line, args, this);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line, args, this);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, args, this);
    }
    return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line) {
    try {
        Command* cmd = CreateCommand(cmd_line);
        if (cmd) {
            cmd->execute();
        }
    } catch (const Command::CommandError& e) {
        cerr << "smash error: " << e.what() << endl;
    }
}

const std::string& SmallShell::name() {
    return _name;
}

/* -------------- BuiltInCommand -------------- */

BuiltInCommand::BuiltInCommand(const char* cmd_line, SmallShell *smash): 
    Command(cmd_line), _smash(smash) {}

std::string& BuiltInCommand::smash_name() {
    return _smash->_name;
}

char *BuiltInCommand::smash_cwd() {
    return _smash->_cwd;
}

bool &BuiltInCommand::smash_cd_called() {
    return _smash->_cd_called;
}

/* -------------- ChpromptCommand -------------- */

ChpromptCommand::ChpromptCommand(const char* cmd_line, char* args[], SmallShell *smash):
    BuiltInCommand(cmd_line, smash) {
    _new_name = args[1];
    _new_name.append("> ");
}

void ChpromptCommand::execute() {
    smash_name() = _new_name;
}

/* -------------- ShowPidCommand -------------- */

ShowPidCommand::ShowPidCommand(const char* cmd_line, char* args[], SmallShell *smash):
    BuiltInCommand(cmd_line, smash) {}

void ShowPidCommand::execute() {
    // TODO: consider changing smash to _smash->name();
    cout << "smash pid is " << getpid() << endl;
}

/* -------------- GetCurrDirCommand -------------- */

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line, char* args[], SmallShell *smash):
    BuiltInCommand(cmd_line, smash) {}

void GetCurrDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    cout << getcwd(cwd, sizeof(cwd)) << endl;
}

/* -------------- ChangeDirCommand -------------- */

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char* args[], SmallShell *smash):
    BuiltInCommand(cmd_line, smash) {
    if (args[2]) {
        throw Command::CommandError("cd: too many arguments");
    }
    _new_dir = args[1];
    if (strcmp(args[1], "-") == 0) {
        if (!smash_cd_called()) {
            throw Command::CommandError("cd: OLDPWD not set");
        }
        strcpy(_new_dir, smash_cwd());
    }
}

void ChangeDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    getcwd(cwd, sizeof(cwd));

    if (chdir(_new_dir) != 0) {
        perror("smash error: chdir failed");
        return;
    }
    strcpy(smash_cwd(), cwd);
    smash_cd_called() = true;
}

/* -------------- JobEntry -------------- */

JobsList::JobEntry::JobEntry(Command *cmd, bool stopped, JobsList* jobs) {
    _job_id = jobs->_last_job_id++;
    _start = time(nullptr);
    _cmd = cmd;
    _stopped = stopped;
}

/* -------------- JobsList -------------- */

void JobsList::addJob(Command* cmd, bool stopped) {
    JobEntry *job = new JobEntry(cmd, stopped, this);
    _jobs.push_back(job);
}

void JobsList::printJobsList() {
    for (JobEntry *job : _jobs) {
        cout << "[" << job->_job_id << "]" << job->_cmd->cmd_line();
        cout << " : " << job->_cmd->pid() << " ";
        cout << difftime(time(nullptr), job->_start) << " secs";
        if (job->_stopped) {
            cout << " (stopped)";
        }
    }
}

/* -------------- ExternalCommand -------------- */
ExternalCommand::ExternalCommand(const char* cmd_line):
    Command(cmd_line) {}

void ExternalCommand::execute() {
    char cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_line, this->cmd_line());

    bool bg_cmd = _isBackgroundComamnd(cmd_line);
    if (bg_cmd) {
        _removeBackgroundSign(cmd_line);
    }
    char* args[COMMAND_MAX_ARGS + 1];
    _parseCommandLine(cmd_line, args);

    int pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("smash error: execvp failed");
    } else {
        if (!bg_cmd) {
            if (waitpid(pid, nullptr, WUNTRACED) < 0) {
                perror("smash error: fork failed");
            }
        }
    }
}