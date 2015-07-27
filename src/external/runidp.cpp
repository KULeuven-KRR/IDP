/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <csetjmp>
#include "common.hpp"
#include "errorhandling/error.hpp"
#include "lua/luaconnection.hpp"
#include "interactive.hpp"
#include "runidp.hpp"
#include "insert.hpp"
#include "GlobalData.hpp"
#include "utils/ResourceMonitor.hpp"
#include "utils/LogAction.hpp"

#include "utils/StringUtils.hpp"

using namespace std;

// Parser stuff
extern void parsefile(const string&);
extern void parsestdin();

std::ostream& operator<<(std::ostream& stream, Status status) {
	stream << (status == Status::FAIL ? "failed" : "success");
	return stream;
}

/**
 * Return the basename of "name".
 *
 * Unlike the libc basename function, this function
 * does not modify its argument.
 **/
static const char *base(const char *name) {
	const char *slash = strrchr(name, '/');
	return slash ? slash + 1 : name;
}

/**
 * Print help message and stop
 **/
static void usage(const char *name) {
	name = base(name);
	cout << "Usage:\n" << "   "
	     << name << " [options] [filename [filename [...]]]\n\n";
	cout << "Options:\n";
#ifdef USEINTERACTIVE
	cout << "    -i, --interactive    run in interactive mode\n";
#endif
	cout << "    -e \"<proc>\"          run procedure <proc> after parsing\n";
	cout << "    -d <dirpath>         search for datafiles in the given directory\n";
	cout << "    --nowarnings         disable warnings\n";
	cout << "    -I                   read from stdin\n";
	cout << "    -v, --version        show version number and stop\n";
	cout << "    -h, --help           show this help message\n\n";
	exit(0);
}

/** 
 * Parse command line options
 **/

struct CLOptions {
	string _exec;
	bool _interactive;
	CLOptions()
			: 	_exec(""),
				_interactive(false) {
	}
};

vector<string> read_options(int argc, char* argv[], CLOptions& cloptions) {
	const char *name = argv[0];
	vector<string> inputfiles;
	argc--;
	argv++;
	while (argc != 0) {
		string str(argv[0]);
		argc--;
		argv++;
		if (str == "-e" || str == "--execute") {
			cloptions._exec = string(argv[0]);
			argc--;
			argv++;
		}
#ifdef USEINTERACTIVE
		else if (str == "-i" || str == "--interactive") {
			cloptions._interactive = true;
		}
#endif
		else if (str == "-d") {
			if (argc == 0) {
				Error::error("-d option should be followed by a directorypath.");
				continue;
			}
			str = argv[0];
			setInstallDirectoryPath(str);
			argc--;
			argv++;
		} else if (str == "--nowarnings") {
			setOption(BoolType::SHOWWARNINGS, false);
		} else if (str == "-v" || str == "--version") {
			cout << GIDLVERSION << " - (git hash: " << GITHASH << ")\n";
			exit(0);
		} else if (str == "-h" || str == "--help") {
			usage(name);
			exit(0);
		} else if (str[0] == '-') {
			Error::unknoption(str);
		} else {
			inputfiles.push_back(str);
		}
	}
	return inputfiles;
}

/**
 * Parse all input files
 */
void parse(const vector<string>& inputfiles) {
	for (unsigned int n = 0; n < inputfiles.size(); ++n) {
		parsefile(inputfiles[n]);
	}
}

// TODO add threading and signal handling code to kill the process by using the signalhandling thread in an infinite loop and some mutexes

bool stoptiming = true;
bool hasStopped = true;
bool running = false;
bool shouldStop() {
	return stoptiming;
}

void setStop(bool value) {
	stoptiming = value;
}

struct RunData {
	string proc;
	const DomainElement** result;
};

void handleAndRun(void* d) {
	RunData* data = (RunData*) d;
	try {
		*data->result = Insert::exec(data->proc);
	} catch (const Exception& ex) {
		Error::error(ex.getMessage());
	} catch (const std::exception& ex) {
		Error::error(ex.what());
	} catch(...){
		Error::error("Third-party, non-descript error thrown.");
	}
	clog.flush();
}

void monitorShutdown(void*) {
	int monitoringtime = 0;
	while (not hasStopped && monitoringtime < 10) { // Wait max 10 seconds
#ifdef __MINGW32__
		Sleep(1000);
#else
		usleep(1000000);
#endif
		monitoringtime += 1;
	}
	if (not hasStopped) {
		clog << "Shutdown failed, aborting.\n";
		abort();
	}
}

void resourceOut() {
	clog << "Out of resources\n";
	getGlobal()->notifyOutOfResources();
	tthread::thread shutdown(&monitorShutdown, NULL);
	shutdown.join();
}

jmp_buf main_loop;
volatile sig_atomic_t abortcode;
volatile sig_atomic_t jumpback; //Only jump back when this is 0
static void SIGABRT_handler(int signum);
static void SIGFPE_handler(int signum);
static void SIGSEGV_handler(int signum);
static void SIGTERM_handler(int signum);
static void SIGINT_handler(int signum);
void handleSignals();

template<typename Handler, typename SIGNAL>
void registerHandler(Handler f, SIGNAL s) {
	signal(s, f); // Note: sigaction objects are cleaner but are not in the ISO standard and poorly portable
}

void setIDPSignalHanders() {
	signal(SIGABRT, SIGABRT_handler);
	signal(SIGFPE, SIGFPE_handler);
	signal(SIGTERM, SIGTERM_handler);
	signal(SIGSEGV, SIGSEGV_handler);
	signal(SIGINT, SIGINT_handler);
#if defined(__linux__)
	signal(SIGHUP, SIGABRT_handler);
	signal(SIGXCPU, SIGABRT_handler);
	signal(SIGXFSZ, SIGABRT_handler);
#endif
}

void unsetSignalHandelers() {
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGINT, SIG_DFL);
#if defined(__linux__)
	signal(SIGHUP, SIG_DFL);
	signal(SIGXCPU, SIG_DFL);
	signal(SIGXFSZ, SIG_DFL);
#endif
}

/**
 * @return the return value of the executed lua procedure if applicable
 */
const DomainElement* executeProcedure(const string& proc) {
	const DomainElement* result = NULL;

	if (proc == "") {
		return result;
	}

	// NOTE: as we allow in lua to replace . with ::, we have to convert the other way here!
	string temp = replaceAllIn(proc, "::", ".");

	setStop(false);
	hasStopped = false;
	running = true;
	getGlobal()->reset();

	setIDPSignalHanders();

	//IMPORTANT: because signals are handled asynchronously, a special mechanism is needed to recover from them (exception throwing does not work)
	//setjmp maintains a jump point to which any stack can jump back, re-executing this statement with different return value,
	//so if this happens, we jump out
	bool stoprunning = false;
	if (setjmp(main_loop)) {
		jumpback = 1;
		handleSignals();
		stoprunning = true;
	}
	if (!stoprunning) {
		jumpback = 0;

		auto t = basicResourceMonitor([](){return getOption(TIMEOUT); }, [](){return getOption(MEMORYOUT); },[](){resourceOut();});
		tthread::thread signalhandling(&resourceMonitorLoop, &t);

		RunData d;
		d.proc = temp;
		d.result = &result;
		tthread::thread execution(&handleAndRun, &d);
		execution.join();

		hasStopped = true;
		running = false;
		setStop(true);
		t.requestStop();
		signalhandling.join();

		jumpback = 1;
	}
	jumpback = 1;
	
	unsetSignalHandelers();

	if (Error::nr_of_errors() + Warning::nr_of_warnings() > 15 && Error::nr_of_errors()>0) {
		clog << "\nFirst critical error encountered:\n"; // NOTE: repeat first error for easy retrieval in the output.
		clog << *getGlobal()->getErrors().cbegin();
	}

	return result;
}

#ifdef USEINTERACTIVE
/** 
 * Interactive mode 
 *
 * Interactive mode terminates when
 * - the user presses ^D (sets idp_terminateInteractive)
 * - the user enters "quit" or "exit"
 * - an error occurred during input (rl_gets returns NULL)
 *
 * Note that rl_gets also returns NULL when the user presses ^C.
 * Since we do not want to terminate on ^C, we check errno, which
 * is set to EAGAIN by linenoise when ^C is pressed.
 **/
void interactive() {
	cout << "Running IDP in interactive mode.\n";

	idp_rl_start();
	while (not idp_terminateInteractive()) {
		errno = 0;
		auto userline = rl_gets();
		if (userline == NULL) {
			if (errno != EAGAIN)
				break;
			cout << "\n";
			continue;
		}

		string command(userline);
		command = trim(command);
		if (command == "exit" || command == "quit" || command == "exit()" || command == "quit()") {
			free(userline);
			break;
		}
		if (command == "help") {
			command = "help()";
		}
		executeProcedure(command);
		getGlobal()->clearStats();
	}
	idp_rl_end();
}
#endif

// TODO manage globaldataobject here instead of singleton?

// Guarantees destruction after return/throw
class DataManager {
public:
	DataManager() {
		LuaConnection::makeLuaConnection();
	}
	~DataManager() {
		LuaConnection::closeLuaConnection();
		GlobalData::close();
	}
};

/**
 * Runs the main method given a number of inputfiles and checks whether the main method returns the int 1.
 * In that case, test return SUCCESS, in all other cases it returns FAIL.
 * NOTE: for any test, it has to have a main() method which returns 1 if success, even for parser tests!
 *
 * If a nonempty execution command is provided, that command is executed instead of main()
 */
Status test(const std::vector<std::string>& inputfileurls, const std::string& executioncommand) {
	DataManager m;

	setOption(BoolType::SHOWWARNINGS, false); //XXX Temporary solution to disable warnings...

	try {
		parse(inputfileurls);
	} catch (const Exception& ex) {
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage();
		Error::error(ss.str());
		clog.flush();
	}

	Status result = Status::FAIL;
	if (Error::nr_of_errors() == 0) {
		stringstream ss;
		if (executioncommand == "") {
			ss << "return " << getGlobalNamespaceName() << ".main()";
		} else {
			ss << "return " << executioncommand;
		}
		auto value = executeProcedure(ss.str());
		if (value != NULL && value->type() == DomainElementType::DET_INT && value->value()._int == 1) {
			result = Status::SUCCESS;
		}
	}

	if (Error::nr_of_errors() > 0) {
		result = Status::FAIL;
	}

	return result;
}

int run(int argc, char* argv[]) {
	CLOptions cloptions;
	vector<string> inputfiles = read_options(argc, argv, cloptions);

	DataManager m;

	bool readfromstdin = false;
	// Only read from stdin if no inputfiles were provided (otherwise, we would have file ordering issues anyway)
	// and we are not running interactively (in that case, the user can provide the files)
	if (inputfiles.size() == 0 && not cloptions._interactive) {
		readfromstdin = true;
	}

	if (cloptions._exec == "") {
		stringstream ss;
		ss << "return " << getGlobalNamespaceName() << ".main()";
		cloptions._exec = ss.str();
	}

	return run(inputfiles, cloptions._interactive, readfromstdin, cloptions._exec);
}

int run(const std::vector<std::string>& inputfiles, bool interact, bool readstdin, const std::string& command){
	try {
		parse(inputfiles);
	} catch (const Exception& ex) {
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage();
		Error::error(ss.str());
		clog.flush();
	}

	if (readstdin) {
		Warning::readingfromstdin();
		parsestdin();
	}

	if (Error::nr_of_errors() == 0) {
#ifdef USEINTERACTIVE
		if (interact) {
			interactive();
		} else {
#endif
			auto value = executeProcedure(command);
			if (value != NULL && value->type() == DomainElementType::DET_INT && Error::nr_of_errors()==0) {
				return value->value()._int;
			}
#ifdef USEINTERACTIVE
		}
#endif
	}

	return Error::nr_of_errors()==0?0:1;
}


static void SIGFPE_handler(int) {
	abortcode = SIGFPE;
	if (not shouldStop() && running) {
		GlobalData::instance()->notifyTerminateRequested();
	} else if (jumpback == 0) {
		longjmp(main_loop, 1);
	}
}

//IMPORTANT: assumed this is always called externally
static void SIGTERM_handler(int) {
	abortcode = SIGTERM;
	if (not shouldStop() && running) {
		GlobalData::instance()->notifyTerminateRequested();
	} else if (jumpback == 0) {
		longjmp(main_loop, 1);
	}
}

static void SIGABRT_handler(int) {
	abortcode = SIGABRT;
	if (not shouldStop() && running) {
		GlobalData::instance()->notifyTerminateRequested();
	} else if (jumpback == 0) {
		longjmp(main_loop, 1);
	}
}

static void SIGSEGV_handler(int) {
	abortcode = SIGSEGV;
	if (jumpback == 0) {
		longjmp(main_loop, 1);
	}else{
		exit(1);
	}
}

void SIGINT_handler(int) {
	if (not shouldStop() && running) {
		GlobalData::instance()->notifyTerminateRequested();
	} else {
		exit(1);
	}
}

void handleSignals() {
	switch (abortcode) {
	case SIGFPE:
		clog << "Exit: Floating point error signal received\n";
		break;
	case SIGABRT:
		clog << "Exit: Abort signal received\n";
		break;
	case SIGINT:
		//clog << ">>> Ctrl-c signal received\n";
		break;
	case SIGSEGV:
		clog << "Exit: Segmentation fault signal received\n";
		break;
	case SIGTERM:
		clog << "Exit: Terminate signal received\n";
		break;
	}
}
