#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <iomanip>
#include <fcntl.h>
#include "Commands.h"
#include <algorithm>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

// #define DBUG

#if defined(DBUG)
#define FUNC_ENTRY()  \
  func_entry_c __func(__PRETTY_FUNCTION__);
#else
#define FUNC_ENTRY()
#endif

class func_entry_c {
    const char* _func;
public:
    func_entry_c(const char* func): _func(func) {
        cerr << _func << " --> " << endl;
    }
    ~func_entry_c() {
        cerr << _func << " <-- " << endl;
    }
};

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
//   FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;
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

void _removeBackgroundSign(string& cmd_line) {
    // find last character other than spaces
    unsigned int idx = cmd_line.find_last_not_of(WHITESPACE);
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
    cmd_line[cmd_line.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _isComplex(const std::string& s) {
    FUNC_ENTRY()
    char ch1 = '*';
    char ch2 = '?';
    size_t pos1 = s.find(ch1);
    size_t pos2 = s.find(ch2);
    return pos1 != std::string::npos || pos2 != std::string::npos;
}

bool _isNumber(const std::string& s) {
    if (s.empty()) {
        return false;
    }

    // Check for a leading '-' character if present
    size_t start = 0;
    if (s[0] == '-') {
        if (s.length() == 1) {
            // The string contains only a '-' character
            return false;
        }
        start = 1;
    }

    // Check if all remaining characters are digits
    return std::all_of(s.begin() + start, s.end(),
                       [](unsigned char c) { return std::isdigit(c); });
}

bool _isNumber(const char* s) {
    return _isNumber(string(s));
}

bool _isRegularRedirection(const std::string& s) {
    char ch = '>';
    size_t pos = s.find(ch);
    return pos != std::string::npos;
}

bool _isAppendRedirection(const std::string& s) {
    std::string ch = ">>";
    size_t pos = s.find(ch);
    return pos != std::string::npos;
}

bool _isRedirectionCommand(const char* cmd_line) {
    string s(cmd_line);
    return _isRegularRedirection(s) || _isAppendRedirection(s);
}

bool _isRegularPipe(const std::string& s) {
    char ch = '|';
    size_t pos = s.find(ch);
    return pos != std::string::npos;
}

bool _isStderrPipe(const std::string& s) {
    std::string ch = "|&";
    size_t pos = s.find(ch);
    return pos != std::string::npos;
}

bool _isPipeCommand(const char* cmd_line) {
    string s(cmd_line);
    return _isRegularPipe(s) || _isStderrPipe(s);
}

/* -------------- Command -------------- */

Command::Command(const char* cmd_line) {
    _smash = &SmallShell::getInstance();
    _cmd_line = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(_cmd_line, cmd_line);
    _jid = -1;
}

const char *Command::cmd_line() {
    return _cmd_line;
}

int Command::pid() {
    return _pid;
}

/* -------------- Command::CommandError -------------- */

Command::CommandError::CommandError(const std::string& message) {
    _message = message;
}

const std::string& Command::CommandError::what() const {
    return _message;
}

/* -------------- SmallShell -------------- */
SmallShell::SmallShell():
    _name("smash> ") {
    _cwd = new char[COMMAND_ARGS_MAX_LENGTH];
    _cd_called = false;
    _running_cmd = nullptr;
}

SmallShell &SmallShell::getInstance() {
    static SmallShell instance;
    return instance;
}

Command *SmallShell::CreateCommand(const char* cmd_line) {

    if (_isPipeCommand(cmd_line)) {
        return new PipeCommand(cmd_line);
    } else if (_isRedirectionCommand(cmd_line)) {
        return new RedirectionCommand(cmd_line);
    }

    char* args[COMMAND_MAX_ARGS];
    _parseCommandLine(cmd_line, args);
    string firstWord(args[0]);
    if (firstWord.back() == '&') {
        firstWord.pop_back();
    }

    if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(cmd_line, args);
    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line, args);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line, args);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, args);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, &_job_list);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, args, &_job_list);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, args, &_job_list);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, args, &_job_list);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, args, &_job_list);
    } else if (firstWord.compare("getfileinfo") == 0) {
        return new GetFileTypeCommand(cmd_line, args);
    } else if (firstWord.compare("chmod") == 0) {
        return new ChmodCommand(cmd_line, args);
    } else if (firstWord.compare("setcore") == 0) {
        return new SetcoreCommand(cmd_line, args, &_job_list);
    }
    return new ExternalCommand(cmd_line);
}

bool SmallShell::executeCommand(const char *cmd_line) {
    _job_list.removeFinishedJobs();
    try {
        Command* cmd = CreateCommand(cmd_line);
        cmd->execute();

        if (dynamic_cast<QuitCommand *>(cmd)) {
            return false;
        }
    } catch (const Command::CommandError& e) {
        cerr << "smash error: " << e.what() << endl;
    }
    return true;
}

const std::string& SmallShell::name() const {
    return _name;
}

void SmallShell::handle_ctrl_z(int sig_num) {
	cout << "smash: got ctrl-Z" << endl;
    if (_running_cmd) {
		pid_t pid = _running_cmd->pid();
        kill(pid, sig_num);
        _job_list.addJob(_running_cmd, true);
        _running_cmd = nullptr;
        cout << "smash: process " << pid << " was stopped" << endl;
    }
}

void SmallShell::handle_ctrl_c(int sig_num) {
	cout << "smash: got ctrl-C" << endl;
    if (_running_cmd) {
	    int pid = _running_cmd->pid();
        kill(_running_cmd->pid(), sig_num);
        _running_cmd = nullptr;
        cout << "smash: process " << pid << " was killed" << endl;
    }
}


/* -------------- BuiltInCommand -------------- */

BuiltInCommand::BuiltInCommand(const char* cmd_line):
    Command(cmd_line) {
    _pid = getpid();
}

std::string& BuiltInCommand::smash_name() {
    return _smash->_name;
}

char *BuiltInCommand::smash_cwd() {
    return _smash->_cwd;
}

bool &BuiltInCommand::smash_cd_called() {
    return _smash->_cd_called;
}

Command* &BuiltInCommand::smash_running_cmd() {
    return _smash->_running_cmd;
}

/* -------------- ExternalCommand -------------- */

ExternalCommand::ExternalCommand(const char* cmd_line):
    Command(cmd_line),
    _background_cmd(false) {
	FUNC_ENTRY()
    _command = new char[COMMAND_ARGS_MAX_LENGTH];
    _args = new char*[COMMAND_MAX_ARGS + 2];
    strcpy(_command, cmd_line);

    if (_isBackgroundComamnd(cmd_line)) {
        _background_cmd = true;
        _removeBackgroundSign(_command);
	    _smash->_job_list.addJob(this);
    }
    if (_isComplex(cmd_line)) {
        _parseCommandLine("/bin/bash -c ", _args);
        _args[2] = _command;
    } else {
        _parseCommandLine(_command, _args);
    }
}

void ExternalCommand::execute() {
	FUNC_ENTRY()
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) {
        execvp(_args[0], _args);
        perror("smash error: execvp failed");
    } else {
        _pid = pid;
        if (!_background_cmd) {
            _smash->_running_cmd = this;
            if (waitpid(pid, nullptr, WUNTRACED) < 0) {
                perror("smash error: waitpid failed");
            }
            _smash->_running_cmd = nullptr;
        }
    }
}

/* -------------- ChpromptCommand -------------- */

ChpromptCommand::ChpromptCommand(const char* cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {
    _new_name = "smash";
    if (args[1]) {
        _new_name = args[1];
    }
    _new_name.append("> ");
}

void ChpromptCommand::execute() {
    smash_name() = _new_name;
}

/* -------------- ShowPidCommand -------------- */

ShowPidCommand::ShowPidCommand(const char* cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << _pid << endl;
}

/* -------------- GetCurrDirCommand -------------- */

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[COMMAND_ARGS_MAX_LENGTH];
    cout << getcwd(cwd, sizeof(cwd)) << endl;
}

/* -------------- ChangeDirCommand -------------- */

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {
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

/* -------------- JobsList::JobEntry -------------- */

JobsList::JobEntry::JobEntry(Command *cmd, bool stopped) {
    _start = time(nullptr);
    _cmd = cmd;
    _pid = cmd->pid();
    _stopped = stopped;
}

Command *JobsList::JobEntry::cmd() {
    return _cmd;
}

bool &JobsList::JobEntry::stopped() {
    return _stopped;
}

pid_t JobsList::JobEntry::pid() const {
    return _pid;
}

/* -------------- JobsList -------------- */

JobsList::JobsList() {
    FUNC_ENTRY()
    _next_jid = 1;
}

void JobsList::addJob(Command* cmd, bool stopped) {
    FUNC_ENTRY()
    removeFinishedJobs();
    JobEntry *job = new JobEntry(cmd, stopped);

    if (cmd->_jid == -1){
        //JobEntry *job = new JobEntry(cmd, stopped);
        job->_jid = _next_jid++;
        cmd->_jid = job->_jid;
        _jobs.push_back(job);
    }

    else{
        //JobEntry *job = new JobEntry(cmd, stopped);
        job->_jid = cmd->_jid;

        auto it = _jobs.begin();
        for (; it != _jobs.end(); ++it){
            auto job2 = *it;
            if (job2->_jid > job->_jid){
                break;
            }
        }
        _jobs.insert(it, job);

        //  _jobs.push_back(job);
    }

}

void JobsList::removeJobById(int jid) {
    FUNC_ENTRY()
    if (_jobs.empty()) {
        return;
    }
    for (std::list<JobEntry *>::iterator it = _jobs.begin(); it != _jobs.end();) {
        if ((*it)->_jid == jid) {
            it = _jobs.erase(it);
        } else {
            ++it;
        }
    }
}

void JobsList::removeFinishedJobs() {
    FUNC_ENTRY()
    if (_jobs.empty()) {
		_next_jid = 1;
		return;
	}
    for (std::list<JobEntry *>::iterator it = _jobs.begin(); it != _jobs.end();) {
        if (waitpid((*it)->cmd()->pid(), nullptr, WNOHANG) > 0) {
            it = _jobs.erase(it);
        } else {
            ++it;
        }
    }

    if (_jobs.empty()) {
		_next_jid = 1;
		return;
	}
    _next_jid = _jobs.back()->_jid + 1;
}

void JobsList::printJobsList() {
    FUNC_ENTRY()
    removeFinishedJobs();
    for (const JobEntry *job : _jobs) {
        cout << "[" << job->_jid << "] " << job->_cmd->cmd_line();
        cout << " : " << job->_cmd->pid() << " ";
        cout << difftime(time(nullptr), job->_start) << " secs";
        if (job->_stopped) {
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    FUNC_ENTRY()
    cout << "smash: sending SIGKILL signal to " << _jobs.size() << " jobs:" << endl;
    for (const JobEntry *job : _jobs) {
        cout << job->_cmd->pid() << ": " << job->_cmd->cmd_line() << endl;
        kill(job->_cmd->pid(), SIGKILL);
    }

    for (std::list<JobEntry *>::iterator it = _jobs.begin(); it != _jobs.end();) {
        it = _jobs.erase(it);
    }
}

JobsList::JobEntry *JobsList::getJobById(int jid) {
    FUNC_ENTRY()
    for (JobEntry *job : _jobs) {
        if (job->_jid == jid) {
            return job;
        }
    }
    throw Command::CommandError("job-id " + to_string(jid) + " does not exist");
}

JobsList::JobEntry *JobsList::getLastJob(int* lastJobId) {
    FUNC_ENTRY()
    if (_jobs.empty()) {
        throw Command::CommandError("jobs list is empty");
    }
    JobEntry *ret = _jobs.back();
    _jobs.pop_back();
    if (lastJobId) {
        *lastJobId = ret->_jid;
    }
    return ret;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int* lastJobId) {
    FUNC_ENTRY()
    if (_jobs.empty()) {
        throw Command::CommandError("there is no stopped jobs to resume");
    }
    for (auto it = _jobs.rbegin(); it != _jobs.rend(); ++it) {
        if ((*it)->_stopped) {
            JobEntry *ret = *it;
            if (lastJobId) {
                *lastJobId = ret->_jid;
            }
            return ret;
        }
    }
    throw Command::CommandError("there is no stopped jobs to resume");
}

/* -------------- JobsCommand -------------- */

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs):
    BuiltInCommand(cmd_line) {
    _jobs = jobs;
}

void JobsCommand::execute() {
    _jobs->printJobsList();
}

/* -------------- ForegroundCommand -------------- */

ForegroundCommand::ForegroundCommand(const char* cmd_line, char* args[], JobsList* jobs):
    BuiltInCommand(cmd_line) {

    int jid;
    JobsList::JobEntry *job;
	if (!args[1]) {
		try {
		 job = jobs->getLastJob(&jid);
		} catch (Command::CommandError& e) {
			throw Command::CommandError("fg: " + e.what());
        }
	} else {
        if (!_isNumber(string(args[1])) || args[2]) {
            throw Command::CommandError("fg: invalid arguments");
        }

        jid = stoi(args[1]);
        try {
            job = jobs->getJobById(jid);
        } catch (Command::CommandError& e) {
            throw Command::CommandError("fg: " + e.what());
        }
    }
    jobs->removeJobById(jid);
    _cmd = job->cmd();
}

void ForegroundCommand::execute() {
    cout << _cmd->cmd_line() << " : " << _cmd->pid() << endl;
    kill(_cmd->pid(), SIGCONT);
    smash_running_cmd() = _cmd;
    waitpid(_cmd->pid(), nullptr, WUNTRACED);
    smash_running_cmd() = nullptr;
}

/* -------------- BackgroundCommand -------------- */

BackgroundCommand::BackgroundCommand(const char* cmd_line, char* args[], JobsList* jobs):
    BuiltInCommand(cmd_line) {
    FUNC_ENTRY()

    int jid;
    JobsList::JobEntry *job;
	if (!args[1]) {
		try {
		 job = jobs->getLastStoppedJob(&jid);
		} catch (Command::CommandError& e) {
			throw Command::CommandError("bg: " + e.what());
        }
	} else {
        if (!_isNumber(string(args[1])) || args[2]) {
            throw Command::CommandError("bg: invalid arguments");
        }

        jid = stoi(args[1]);
        try {
            job = jobs->getJobById(jid);
        } catch (Command::CommandError& e) {
            throw Command::CommandError("bg: " + e.what());
        }
        if (!job->stopped()) {
            throw Command::CommandError("bg: job-id " + to_string(jid)
            + " is already running in the background");
        }
    }
    job->stopped() = false;
    _cmd = job->cmd();
}

void BackgroundCommand::execute() {
    cout << _cmd->cmd_line() << " : " << _cmd->pid() << endl;
    kill(_cmd->pid(), SIGCONT);
}

/* -------------- QuitCommand -------------- */

QuitCommand::QuitCommand(const char* cmd_line, char* args[], JobsList* jobs):
    BuiltInCommand(cmd_line) {
    _kill = false;
    _jobs = jobs;
    if (args[1] && strcmp(args[1], "kill") == 0) {
        _kill = true;
    }
}

void QuitCommand::execute() {
    if (_kill) {
        _jobs->killAllJobs();
    }
}

/* -------------- KillCommand -------------- */

KillCommand::KillCommand(const char* cmd_line, char* args[], JobsList* jobs):
    BuiltInCommand(cmd_line) {
    FUNC_ENTRY()

    if (!args[1] || !args[2] || args[1][0] != '-' || !_isNumber(string(args[1] + 1))
    || !_isNumber(string(args[2])) || args[3]) {
        throw Command::CommandError("kill: invalid arguments");
    }

    try {
        _pid = jobs->getJobById(stoi(args[2]))->cmd()->pid();
    } catch (const CommandError& e) {
        throw CommandError("kill: " + e.what());
    }
    _signum = stoi(args[1] + 1);
    if (_signum > 31 || _signum < 1){
        throw Command::CommandError("kill: invalid arguments");
    }
}

void KillCommand::execute() {
    FUNC_ENTRY()
    cout << "signal number " << _signum << " was sent to pid " << _pid << endl;
    kill(_pid, _signum);
}

/* -------------- RedirectionCommand -------------- */

RedirectionCommand::RedirectionCommand(const char* cmd_line):
    Command(cmd_line) {
    FUNC_ENTRY()
    size_t pos;
    string _cmd_line(cmd_line);
    _removeBackgroundSign(_cmd_line);
    _append = _isAppendRedirection(_cmd_line);
    if (!_append) {
        pos = _cmd_line.find('>');
        _filename = _cmd_line.substr(pos + 1, _cmd_line.length());
    } else {
        pos = _cmd_line.find(">>");
        _filename = _cmd_line.substr(pos + 2, _cmd_line.length());
    }
    _cmd = _smash->CreateCommand(_trim(_cmd_line.substr(0, pos)).c_str());
    _filename = _trim(_filename);
}

void RedirectionCommand::execute() {
    FUNC_ENTRY()
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) {
        close(1);
        if (_append) {
            open(_filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        } else {
            open(_filename.c_str(), O_WRONLY | O_CREAT, 0666);
        }
        _cmd->execute();
        exit(0);
    } else {
        if (waitpid(pid, nullptr, WUNTRACED) < 0) {
            perror("smash error: waitpid failed");
        }
    }
}

/* -------------- PipeCommand -------------- */

PipeCommand::PipeCommand(const char* cmd_line):
    Command(cmd_line) {
    FUNC_ENTRY()

    size_t pos;
    string _cmd_line_2;
    string _cmd_line(cmd_line);
    _removeBackgroundSign(_cmd_line);
    if (_isRegularPipe(_cmd_line)) {
        pos = _cmd_line.find('|');
        _cmd_line_2 = _cmd_line.substr(pos + 1, _cmd_line.length());
    } else {
        pos = _cmd_line.find("|&");
        _cmd_line_2 = _cmd_line.substr(pos + 2, _cmd_line.length());
    }
    _cmds[0] = _smash->CreateCommand(_trim(_cmd_line.substr(0, pos)).c_str());
    _cmds[1] = _smash->CreateCommand(_trim(_cmd_line_2).c_str());
}

void PipeCommand::execute() {
    FUNC_ENTRY()

    int _pipe[2];
    pipe(_pipe);
    int _out = _isRegularPipe(cmd_line()) ? 1 : 2;
    pid_t pid_1 = fork();
    if (pid_1 < 0) {
        perror("smash error: fork failed");
    } else if (pid_1 == 0) {
        dup2(_pipe[1], _out);
        close(_pipe[0]);
        close(_pipe[1]);
        _cmds[0]->execute();
        exit(0);
    }
    pid_t pid_2 = fork();
    if (pid_2 < 0) {
        perror("smash error: fork failed");
    } else if (pid_2 == 0) {
        dup2(_pipe[0], 0);
        close(_pipe[0]);
        close(_pipe[1]);
        _cmds[1]->execute();
        exit(0);
    }
    close(_pipe[0]);
    close(_pipe[1]);
    if (waitpid(pid_1, nullptr, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
    }
    if (waitpid(pid_2, nullptr, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
    }
}

/* -------------- GetFileTypeCommand -------------- */

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {
    FUNC_ENTRY()
    if (args[2]) {
        throw Command::CommandError("gettype: invalid arguments");
    }
    _path = args[1];
}

void GetFileTypeCommand::execute() {
    FUNC_ENTRY()
    struct stat stats;
    stat(_path, &stats);
    std::string type;

    if (S_ISREG(stats.st_mode)) {
        type = "\"regular file\"";
    } else if (S_ISDIR(stats.st_mode)) {
        type = "\"directory\"";
    } else if (S_ISCHR(stats.st_mode)) {
        type = "\"character device\"";
    } else if (S_ISBLK(stats.st_mode)) {
        type = "\"block device\"";
    } else if (S_ISFIFO(stats.st_mode)) {
        type = "\"FIFO\"";
    } else if (S_ISLNK(stats.st_mode)) {
        type = "\"symbolic link\"";
    } else if (S_ISSOCK(stats.st_mode)) {
        type = "\"socket\"";
    } else {
        throw Command::CommandError("gettype: invalid arguments");
    }
    cout << _path << "'s type is " << type;
    cout << " and takes up " << stats.st_size << " bytes" << endl;
}

/* -------------- ChmodCommand -------------- */

ChmodCommand::ChmodCommand(const char *cmd_line, char* args[]):
    BuiltInCommand(cmd_line) {
    FUNC_ENTRY()
    if (args[3] || !_isNumber(string(args[1]))) {
        throw Command::CommandError("chmod: invalid arguments");
    }
    try {
        _new_mode = stoi(args[1], 0 , 8);
    } catch (...) {
        throw Command::CommandError("chmod: invalid arguments");
    }
    _path = args[2];
}

void ChmodCommand::execute() {
    FUNC_ENTRY()
    if (chmod(_path, _new_mode) < 0) {
        perror("smash error: chmod failed");
    }
}

/* -------------- SetcoreCommand -------------- */

SetcoreCommand::SetcoreCommand(const char *cmd_line, char* args[], JobsList* jobs):
    BuiltInCommand(cmd_line) {
    FUNC_ENTRY()
    if (args[3] || !_isNumber(args[1]) || !_isNumber(args[2])) {
        throw Command::CommandError("setcore: invalid arguments");
    }
    _core = stoi(args[2]);
    try {
        _pid = jobs->getJobById(stoi(args[1]))->pid();

    } catch (const CommandError& e) {
        throw CommandError("setcore: " + e.what());
    }
}

void SetcoreCommand::execute(){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(_core, &cpuset);
    if (sched_setaffinity(_pid, sizeof(cpuset), &cpuset) == -1){
        throw Command::CommandError("setcore: invalid core number");
    }
}