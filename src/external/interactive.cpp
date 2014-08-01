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

#include <cstdio> // Because of error in cygwin readline library (not including this because of older standard)
#include "interactive.hpp"

/** Interactive mode **/

#ifdef USEINTERACTIVE
extern "C"{
	#include "linenoise.h"
}
#ifdef UNIX
#include <sys/stat.h>
#endif UNIX
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <cstring>

#include "commands/allcommands.hpp"

using namespace std;

char* historyfilename;


bool idp_terminateInteractive() {
	return requestedInteractiveTermination();
}

static char *line_read = (char *)NULL;
char* rl_gets() {
	if (line_read) {
		free(line_read);
		line_read = (char *)NULL;
	}
	line_read = linenoise("> ");
	if (line_read!=NULL) {
		linenoiseHistoryAdd(line_read);
		linenoiseHistoryAppend(historyfilename, line_read); /* Save every new entry */
	}
	return (line_read);
}

std::set<std::string> commands;

void completion(const char *buf, linenoiseCompletions *lc) {
	if(buf==NULL) {
		return;
	}
	int size = strlen(buf);
	auto begin = commands.lower_bound(buf);
	while(begin!=commands.cend() && strcmp((*begin).substr(0,size).c_str(),buf)==0) {
		linenoiseAddCompletion(lc,begin->c_str());
		++begin;
	}
}

void idp_rl_start() {
	//enable auto-complete
	linenoiseSetCompletionCallback(completion);
	commands = {"help()", "quit()", "exit()"};
	// TODO ask lua also what are the user defined parsed commands
	for(auto i = getAllInferences().cbegin(); i<getAllInferences().cend(); ++i) {
		if((*i)->getArgumentTypes().size()==0) {
			commands.insert((*i)->getName() + "()");
		} else {
			stringstream ss;
			ss <<(*i)->getName() <<"(";
			bool begin = true;
			for(auto j=(*i)->getArgumentTypes().cbegin(); j<(*i)->getArgumentTypes().cend(); ++j) {
				if(not begin) {
					ss <<",";
				}
				begin = false;
				ss <<toCString(*j);
			}
			ss <<")";
			commands.insert(ss.str());
			commands.insert((*i)->getName() + "(");
		}
	}

	linenoiseHistorySetMaxLen(30);

	bool success = false;
	/* Load the history at startup */
	historyfilename = ".idp3_history";

	// Create it first if it does not yet exist
	FILE* f = fopen(historyfilename, "r");
	if(f==NULL) {
		f = fopen(historyfilename, "w");
		if(f!=NULL) {
			fclose(f);
		}
	}
	if(linenoiseHistoryLoad(historyfilename)==0) {
		success = true;
	}

	if(not success) {
		std::clog <<"The interactive shell history could not be saved. No history will be maintained across sessions.\n";
	}
}

void idp_rl_end() {
}

#else
char* rl_gets() {
}
void idp_rl_start() {
}
void idp_rl_end() {
}
#endif
