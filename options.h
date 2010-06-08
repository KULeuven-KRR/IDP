/************************************
	options.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

enum WarningTypes { WT_FREE_VARS=0, WT_VARORCONST=1, WT_SORTDERIVE=2, WT_STDIN=3 };

struct Options {

		  // Attributes
		  bool				_statistics;		// print statistics on stderr iff _statistics=true
		  bool				_verbose;			// print extra information on stderr iff _verbose=true
		  bool				_readfromstdin;		// expect input from stdin iff _readfromstdin=true
		  vector<bool>		_warning;			// _warning[n] = true means that warnings of type n are not suppressed

		  // Constructor
		  Options() {
				_statistics = false;
				_verbose = false;
				_readfromstdin = false;
				_warning = vector<bool>(4,true);
		  }

};

#endif
