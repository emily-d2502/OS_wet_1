#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
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


/* -------------- Small Shell -------------- */
SmallShell::SmallShell():
    _name("smash> ") {
    _cwd = new char[COMMAND_ARGS_MAX_LENGTH]; 
    _cd_called = false;
}

SmallShell::~SmallShell() {}


Command * SmallShell::CreateCommand(const char* cmd_line) {
    
    char* args[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line, args);
    string firstWord(args[0]);

    if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(args, this);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(args, this);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(args, this);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(args, this);
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd) {
        cmd->execute();
    }  
}

/* -------------- BuiltInCommand -------------- */

BuiltInCommand::BuiltInCommand(SmallShell *smash): 
    Command(), _smash(smash) {}

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

ChpromptCommand::ChpromptCommand(char* args[], SmallShell *smash):
    BuiltInCommand(smash) {
    _new_name = args[1];
    _new_name.append("> ");
}

void ChpromptCommand::execute() {
    smash_name() = _new_name;
}

/* -------------- ShowPidCommand -------------- */

ShowPidCommand::ShowPidCommand(char* args[], SmallShell *smash):
    BuiltInCommand(smash) {}

void ShowPidCommand::execute() {
    // TODO: consider changing smash to _smash->name();
    cout << "smash pid is " << getpid() << endl;
}

/* -------------- GetCurrDirCommand -------------- */

GetCurrDirCommand::GetCurrDirCommand(char* args[], SmallShell *smash):
    BuiltInCommand(smash) {}

void GetCurrDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    cout << getcwd(cwd, sizeof(cwd)) << endl;
}

/* -------------- ChangeDirCommand -------------- */

ChangeDirCommand::ChangeDirCommand(char* args[], SmallShell *smash):
    BuiltInCommand(smash) {
    _new_dir = args[1];
    if (strcmp(args[1], "-") == 0) {
        if (!smash_cd_called()) {
            // throw
        }
        strcpy(_new_dir, smash_cwd());
    }
}

void ChangeDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    getcwd(cwd, sizeof(cwd));

    chdir(_new_dir);
    strcpy(smash_cwd(), cwd);
    smash_cd_called() = true;
}