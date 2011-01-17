/************************************
	options.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

/** Command-line options **/

enum WarningTypes { 
	WT_FREE_VARS=0,		// warning if free variables are detected
	WT_VARORCONST=1,	// warning if it is ambiguous whether some term is a variable or a constant
	WT_SORTDERIVE=2,	// warning if some (probably unexpected) sorts are derived for a variable
	WT_STDIN=3			// warning if trying to read from the stdin
};

class CLOptions {

	public:
		  // Attributes
		  bool				_statistics;		// print statistics on stderr iff _statistics=true
		  bool				_verbose;			// print extra information on stderr iff _verbose=true
		  bool				_readfromstdin;		// expect input from stdin iff _readfromstdin=true
		  bool				_interactive;		// interactive mode if _interactive is true
		  vector<bool>		_warning;			// _warning[n] = true means that warnings of type n are not suppressed
		  string			_exec;				// the procedure called from the command line

		  // Constructor (default options)
		  CLOptions() {
				_statistics = false;
				_verbose = false;
				_readfromstdin = false;
				_interactive = false;
				_warning = vector<bool>(4,true);
				_exec = "";
		  }

};

/** Inference options **/

class InfOptions {
	
	public:

		// Name and place
		string			_name;		// the name of the options
		ParseInfo		_pi;		

		// Attributes
		unsigned int	_nrmodels;	// the number of models to compute
		
		// Constructor (default options)
		InfOptions(const string& name, const ParseInfo& pi) : 
			_name(name), 
			_pi(pi),
			_nrmodels(1) 
			{ }

		// Setters
		void set(InfOptions* opt) {
			_nrmodels = opt->_nrmodels;
		}

		// Inspectors
		static bool isoption(const string& str) {
			bool r = false;
			if(str == "nrmodels") r = true;
			return r;
		}
		const string& name() const { return _name; }

};

#endif
