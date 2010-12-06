/************************************
	options.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

enum WarningTypes { 
	WT_FREE_VARS=0,		// warning if free variables are detected
	WT_VARORCONST=1,	// warning if it is ambiguous whether some term is a variable or a constant
	WT_SORTDERIVE=2,	// warning if some (probably unexpected) sorts are derived for a variable
	WT_STDIN=3			// warning if trying to read from the stdin
};

struct Options {

	public:
		  // Attributes
		  bool				_statistics;		// print statistics on stderr iff _statistics=true
		  bool				_verbose;			// print extra information on stderr iff _verbose=true
		  bool				_readfromstdin;		// expect input from stdin iff _readfromstdin=true
		  bool				_interactive;		// interactive mode if _interactive is true
		  vector<bool>		_warning;			// _warning[n] = true means that warnings of type n are not suppressed

		  // Constructor (default options)
		  Options() {
				_statistics = false;
				_verbose = false;
				_readfromstdin = false;
				_interactive = false;
				_warning = vector<bool>(4,true);
		  }

};

#endif
