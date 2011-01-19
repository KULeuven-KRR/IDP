#include "interactive.hpp"

/** Interactive mode **/

#ifdef USEREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>

static char *line_read = (char *)NULL;
char* rl_gets() {
	if (line_read) {
		free(line_read);
		line_read = (char *)NULL;
    }
	line_read = readline ("> ");
	if (line_read && *line_read) add_history(line_read);
	return (line_read);
}
#else
char* rl_gets() {}
#endif