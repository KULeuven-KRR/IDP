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
		  bool				_statistics;
		  bool				_verbose;
		  bool				_readfromstdin;
		  vector<bool>		_warning;

		  // Constructor
		  Options() {
				_statistics = false;
				_verbose = false;
				_readfromstdin = false;
				_warning = vector<bool>(4,true);
		  }

};

#endif
