#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>

/** Interactive mode **/

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


