/************************************
	interactive.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "interactive.hpp"

/** Interactive mode **/

#ifdef USEINTERACTIVE
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <iostream>

void saveHistory(){
	int success = 0;
	if(success==0){
		success = append_history(20,NULL);
	}
	if(success==0){
		history_truncate_file(NULL,20);
	}
	// if any fails, we just fail silently (the user will never notice anyway)
}

static char *line_read = (char *)NULL;
char* rl_gets() {
	if (line_read) {
		free(line_read);
		line_read = (char *)NULL;
	}
	line_read = readline("> ");	// TODO: doesn't wait if there was already a file on stdin
	if (line_read && *line_read){
		add_history(line_read);
		saveHistory();
	}
	return (line_read);
}

void idp_rl_start() {
	using_history();
	int success = read_history(NULL);
	if(success!=0){ // if read fails, we assume no history file exists, so we try to create it by writing to it
		success = write_history(NULL);
	}
	success = read_history(NULL);
	if(success!=0){
		std::cerr <<"The interactive shell history could not be saved. No history will be maintained across sessions.\n";
	}
}

void idp_rl_end() {
	saveHistory();
}

#else
char*	rl_gets()		{	}
void	idp_rl_start()	{	}
void	idp_rl_end()	{	}
#endif
