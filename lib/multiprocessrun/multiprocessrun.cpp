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
#include <queue>
#include <iostream>
#include <sstream>
#include <cstdio>

using namespace std;

int runningProcesses = 0;
//bool printing = false;
mutex cs;

std::vector<stringstream*> files;

void run(const char* exec) {
//	cs.lock();
//	auto currentlyprinting = printing;
//	cs.unlock();
//	if (currentlyprinting) {
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
	cs.unlock();
	/*	} else {
	 cs.lock();
	 printing = true;
	 cs.unlock();
	 printing = true;
	 system(exec);
	 cs.lock();
	 printing = false;
	 cs.unlock();
	 }*/
	cs.lock();
	runningProcesses--;
	cs.unlock();
}

int main(int argc, char** argv) {
	argc--;
	argv++;
	int maxnb = atoi(*argv);
	argc--;
	argv++;

	queue<const char*> execs;
//	cerr <<"Queued ";
	while (argc > 0) {
//		cerr <<*argv <<" ";
		execs.push(*argv);
		argv++;
		argc--;
	}
//	cerr <<"\n";

	vector<thread> threads;
	while (true) {
		if (runningProcesses < maxnb && not execs.empty()) {
			runningProcesses++;
			threads.push_back(thread(&run, execs.front()));
			execs.pop();
		} else {
			if (runningProcesses == 0) {
				break;
			}
			sleep(0.25);
		}
	}
	for (auto i = threads.begin(); i < threads.end(); ++i) {
		i->join();
	}
	for (auto i = files.cbegin(); i < files.cend(); ++i) {
		cout << (*i)->str() << "\n";
		delete (*i);
	}
}
