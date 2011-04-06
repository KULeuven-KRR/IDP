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

static char *line_read = (char *)NULL;
char* rl_gets() {
	if (line_read) {
		free(line_read);
		line_read = (char *)NULL;
    }
	line_read = readline("> ");	// TODO: doesn't wait if there was already a file on stdin
	if (line_read && *line_read) add_history(line_read);
	return (line_read);
}

void idp_rl_start() {
	read_history(NULL);
}

void idp_rl_end() {
	append_history(20,NULL);
	history_truncate_file(NULL,20);
}

#else
char*	rl_gets()		{	}
void	idp_rl_start()	{	}
void	idp_rl_end()	{	}
#endif
