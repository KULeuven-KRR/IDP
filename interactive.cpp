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
#else
char* rl_gets() {}
#endif
