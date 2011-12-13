#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <map>
#include "common.hpp"
#include "error.hpp"
#include "luaconnection.hpp"
#include "interactive.hpp"
#include "rungidl.hpp"
#include "insert.hpp"
#include "GlobalData.hpp"
#include <csignal>

#include "GeneralUtils.hpp"

#include "external/TerminationManagement.hpp"

using namespace std;

// seed
int global_seed; //!< seed used for bdd estimators

// Parser stuff
extern void parsefile(const string&);
extern void parsestdin();

std::ostream& operator<<(std::ostream& stream, Status status) {
	stream << (status == Status::FAIL ? "failed" : "success");
	return stream;
}

/**
 * Print help message and stop
 **/
void usage() {
	cout << "Usage:\n" << "   gidl [options] [filename [filename [...]]]\n\n";
	cout << "Options:\n";
	cout << "    -i, --interactive    run in interactive mode\n";
	cout << "    -e \"<proc>\"          run procedure <proc> after parsing\n";
	cout << "    -c <name1>=<name2>   substitute <name2> for <name1> in the input\n";
	cout << "    --nowarnings         disable warnings\n";
	cout << "    --seed=N             use N as seed for the random generator\n"; 
	cout << "    -I                   read from stdin\n";
	cout << "    -v, --version        show version number and stop\n";
	cout << "    -h, --help           show this help message\n\n";
	exit(0);
}

/** 
 * Parse command line constants 
 **/

struct CLOptions {
	string _exec;
#ifdef USEINTERACTIVE
	bool _interactive;
#endif
	bool _readfromstdin;
	CLOptions() :
			_exec("")
#ifdef USEINTERACTIVE
					, _interactive(false)
#endif
					, _readfromstdin(false) {
	}
};

/** 
 * Parse command line options 
 **/
vector<string> read_options(int argc, char* argv[], CLOptions& cloptions) {
	vector<string> inputfiles;
	argc--;
	argv++;
	while (argc) {
		string str(argv[0]);
		argc--;
		argv++;
		if (str == "-e" || str == "--execute") {
			cloptions._exec = string(argv[0]);
			argc--;
			argv++;
		}
#ifdef USEINTERACTIVE
		else if(str == "-i" || str == "--interactive") {
			cloptions._interactive = true;
		}
#endif
		else if (str == "-c") {
			str = argv[0];
			if (argc && (str.find('=') != string::npos)) {
				int p = str.find('=');
				string name1 = str.substr(0, p);
				string name2 = str.substr(p + 1, str.size());
				GlobalData::instance()->setConstValue(name1, name2);
			} else
				Error::constsetexp();
			argc--;
			argv++;
		} else if (str == "--nowarnings") {
			setOption(BoolType::SHOWWARNINGS,false);
		} else if (str.substr(0, 7) == "--seed=") {
			global_seed = toInt(str.substr(7, str.size()));
		} else if (str == "-I") {
			cloptions._readfromstdin = true;
		} else if (str == "-v" || str == "--version") {
			cout << "GidL 2.0.1\n";
			exit(0);
		} else if (str == "-h" || str == "--help") {
			usage();
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
#include <thread>

//TODO willen we een run van een lua procedure timen of eigenlijk een command op zich?
// is misschien nogal vreemd om lua uitvoering te timen?

bool throwfromexecution = false;
void handleAndRun(const string& proc, const DomainElement** result) {
	try {
		*result = Insert::exec(proc);
	} catch (const Exception& ex) {
		clog.flush();
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage() << ".\n";
		Error::error(ss.str());
		*result = NULL;
	}catch(const std::exception& ex){
		clog.flush();
		stringstream ss;
		ss << "Exception caught: " << ex.what() << ".\n";
		Error::error(ss.str());
		throwfromexecution = true;
	}
}

std::thread::native_handle_type executionhandle;
bool stoptiming = true;
bool hasStopped = true;
bool running = false;
bool shouldStop() {
	return stoptiming;
}

void setStop(bool value) {
	stoptiming = value;
}

void monitorShutdown() {
	int monitoringtime = 0;
//	setOption(IntType::GROUNDVERBOSITY, 10);
//	setOption(IntType::SATVERBOSITY, 10);
	while(not hasStopped && monitoringtime<3000000){
		usleep(10000);
		monitoringtime+=10000;
	}
	if(not hasStopped){
		// TODO add for debugging (need execution thread id)
		pthread_kill(executionhandle, SIGUSR1);
		clog <<"Shutdown failed, aborting.\n";
		abort();
	}
}

void timeout() {
	int time = 0;
	int sleep = 10;
	//clog <<"Timeout: " <<getOption(IntType::TIMEOUT) <<", currently at " <<time/1000 <<"\n";
	while (not shouldStop()) {
		time += sleep;
		usleep(sleep * 1000);
		if (sleep < 1000) {
			if (sleep < 100) {
				sleep += 10;
			} else {
				sleep += 100;
			}
		}
		//clog <<"Timeout: " <<getOption(IntType::TIMEOUT) <<", currently at " <<time/1000 <<"\n";
		if (getOption(IntType::TIMEOUT) < time / 1000) {
			clog << "Timed-out\n";
			getGlobal()->notifyTerminateRequested();
			thread shutdown(monitorShutdown);
			shutdown.join();
			break;
		}
		if (getOption(IntType::TIMEOUT) == 0) {
			return;
		}
	}
}

void SIGINT_handler(int) {
	if(not shouldStop() && running){
		GlobalData::instance()->notifyTerminateRequested();
	}else{
		// TODO conform to other shells, should just go to a new line in the shell.
		// TODO ctrl-d should exit
		exit(1);
	}
}

void SIGUSR1_handler(int) {
	sleep(1000000);
}

template<typename Handler, typename SIGNAL>
void registerHandler(Handler f, SIGNAL s){
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = f;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(s, &sigIntHandler, NULL);
}

/**
 * @return the return value of the executed lua procedure if applicable
 */
const DomainElement* executeProcedure(const string& proc) {
	const DomainElement* result = NULL;

	if (proc != "") {
		// NOTE: as we allow in lua to replace . with ::, we have to convert the other way here!
		string temp = proc;
		replaceAll<std::string>(temp, "::", ".");

		setStop(false);
		hasStopped = false;
		running = true;
		throwfromexecution = false;
		getGlobal()->reset();
		startInference(); // NOTE: have to tell the solver to reset its instance
		// FIXME should not be here, but in a less error-prone place. Or should pass an adapated time-out to the solver?

		registerHandler(SIGINT_handler,SIGINT);
		registerHandler(SIGUSR1_handler,SIGUSR1);

		thread signalhandling(timeout);

		thread execution(handleAndRun, temp, &result);
		executionhandle = execution.native_handle();
		execution.join();

		hasStopped = true;
		running = false;
		setStop(true);
		signalhandling.join();
		if(throwfromexecution){
			throw std::exception();
		}
	}

	return result;
}

#ifdef USEINTERACTIVE
/** 
 * Interactive mode 
 **/
void interactive() {
	string help1 = "help", help2 = "help()", exit1 = "exit", exit2 = "exit()";

	cout << "Running GidL in interactive mode.\n"
	<< "  Type 'exit' to quit.\n"
	<< "  Type 'help' for help\n\n";

	idp_rl_start();
	while(true) {
		char* userline = rl_gets();
		if(userline==NULL) {
			cout << "\n";
			continue;
		}

		string command(userline);
		if(command==exit1 || command==exit2) {
			free(userline);
			idp_rl_end();
			return;
		}
		if(command==help1) {
			command = help2;
		}
		executeProcedure(command);
	}
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
 */
Status test(const std::vector<std::string>& inputfileurls) {
	DataManager m;
	
	setOption(BoolType::SHOWWARNINGS,false); //XXX Temporary solution to disable warnigns...

	try {
		parse(inputfileurls);
	} catch (const Exception& ex) {
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage() << ".\n";
		Error::error(ss.str());
		clog.flush();
	}

	Status result = Status::FAIL;
	if (Error::nr_of_errors() == 0) {
		stringstream ss;
		ss << "return " << getTablenameForInternals() << ".main()";
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
	DataManager m;

	CLOptions cloptions;
	vector<string> inputfiles = read_options(argc, argv, cloptions);

	try {
		parse(inputfiles);
	} catch (const Exception& ex) {
		stringstream ss;
		ss << "Exception caught: " << ex.getMessage() << ".\n";
		Error::error(ss.str());
		clog.flush();
	}

	if (cloptions._readfromstdin)
		parsestdin();
	if (cloptions._exec == "") {
		stringstream ss;
		ss << "return " << getTablenameForInternals() << ".main()";
		cloptions._exec = ss.str();
	}

	if (Error::nr_of_errors() == 0) {
#ifdef USEINTERACTIVE
		if(cloptions._interactive) {
			interactive();
		} else {
			executeProcedure(cloptions._exec);
		}
#else
		executeProcedure(cloptions._exec);
#endif
	}

	return Error::nr_of_errors();
}
