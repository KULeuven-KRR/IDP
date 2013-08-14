/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include <thread>
#include <mutex>
#include <queue>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <mutex>

using namespace std;

int runningProcesses = 0;
mutex cs;

std::vector<stringstream*> files;

void run(const char* exec) {
	stringstream ss;
	ss << exec << " " << "2>&1";
	cs.lock();
	cerr << "Running " << exec << "\n";
	cs.unlock();

	auto file = popen(ss.str().c_str(), "r");

	auto output = new stringstream();
	*output << "[EXECUTABLE]  " << exec << "\n";
	std::string cur_string = "";
	const int SIZEBUF = 1234;
	char buf[SIZEBUF];
	while (fgets(buf, sizeof(buf), file) != NULL) {
		cur_string += buf;
	}
	*output << cur_string; //.substr(0, cur_string.size() - 1); //FIXME: Why substring ?

	pclose(file);

	cs.lock();
	files.push_back(output);
	runningProcesses--;
	cs.unlock();
}

int main(int argc, char** argv) {
	argc--;
	argv++;
	auto maxnb = atoi(*argv);
	if(maxnb<1){
		cerr <<"Error: should be allowed to use at least one thread ;-), aborting...\n";
		return -1;
	}

	argc--;
	argv++;

	queue<const char*> execs;
	while (argc > 0) {
		execs.push(*argv);
		argv++;
		argc--;
	}

	vector<thread> threads;
	while (true) {
		if (runningProcesses < maxnb && not execs.empty()) {
			cs.lock();
			runningProcesses++;
			cs.unlock();
			threads.push_back(thread(&run, execs.front()));
			execs.pop();
		} else {
			cs.lock();
			auto nb = runningProcesses;
			cs.unlock();
			if (nb == 0 && execs.empty()) {
				break;
			}
			sleep(1);
		}
	}
	for (auto i = threads.begin(); i<threads.end(); ++i) {
		i->join();
	}
	for (auto file : files) {
		cout << file->str() << "\n";
		delete (file);
	}
	return 0;
}
