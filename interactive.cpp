#include <cstdio> // Because of error in cygwin readline library (not including this because of older standard)
#include "interactive.hpp"

/** Interactive mode **/

#ifdef USEINTERACTIVE
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <iostream>
#include <vector>

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

char** completion(const char*, int ,int);
std::vector<std::string> commands;

void idp_rl_start() {
    //enable auto-complete
	rl_attempted_completion_function = completion;
    rl_bind_key('\t',rl_complete);
    commands = {"help()", "quit()", "exit()"};

	using_history();
	int success = read_history(NULL);
	if(success!=0){ // if read fails, we assume no history file exists, so we try to create it by writing to it
		success = write_history(NULL);
	}
	success = read_history(NULL);
	if(success!=0){
		std::clog <<"The interactive shell history could not be saved. No history will be maintained across sessions.\n";
	}
}

void idp_rl_end() {
	saveHistory();
}

int commandindex, len;
char * dupstr (const char* s);
char* my_generator(const char* text, int state){
	if (not state) {
		commandindex = 0;
        len = strlen (text);
    }

    while (commandindex<commands.size()) {
    	const char* name = commands[commandindex].c_str();
    	commandindex++;

        if (strncmp (name, text, len) == 0){
            return dupstr(name);
        }
    }

    /* If no names matched, then return NULL. */
    return ((char *)NULL);

}

char** completion( const char * text , int start,  int end){
	char **matches;

	matches = (char **)NULL;

	if (start == 0)
		matches = rl_completion_matches ((char*)text, &my_generator);
	else
		rl_bind_key('\t',rl_abort);

	return (matches);
}

void * xmalloc (int size){
    void *buf;

    buf = malloc (size);
    if (!buf) {
        fprintf (stderr, "Error: Out of memory. Exiting.'n");
        exit (1);
    }

    return buf;
}
char * dupstr (const char* s) {
  char *r;

  r = (char*) xmalloc ((strlen (s) + 1));
  strcpy (r, s);
  return (r);
}



#else
char*	rl_gets()		{	}
void	idp_rl_start()	{	}
void	idp_rl_end()	{	}
#endif
