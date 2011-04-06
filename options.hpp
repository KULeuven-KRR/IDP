/************************************
	options.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

struct TypedInfArg;

/** Command-line options **/

enum WarningTypes { 
	WT_FREE_VARS=0,		// warning if free variables are detected
	WT_VARORCONST=1,	// warning if it is ambiguous whether some term is a variable or a constant
	WT_SORTDERIVE=2,	// warning if some (probably unexpected) sorts are derived for a variable
	WT_STDIN=3,			// warning if trying to read from the stdin
	WT_AUTOCOMPL=4		// warning if structure is completed automatically
};

enum StyleOptions {
	SO_CASE=0	// variables start with lowercase, all other symbols with uppercase
};

class CLOptions {

	public:
		  // Attributes
		  bool				_statistics;		// print statistics on stderr iff _statistics=true
		  bool				_verbose;			// print extra information on stderr iff _verbose=true
		  bool				_readfromstdin;		// expect input from stdin iff _readfromstdin=true
		  bool				_interactive;		// interactive mode if _interactive is true
		  vector<bool>		_warning;			// _warning[n] = true means that warnings of type n are not suppressed
		  vector<bool>		_style;				// _style[n] = true means that style option n is enforced
		  string			_exec;				// the procedure called from the command line
		  int				_satverbosity;

		  // Constructor (default options)
		  CLOptions() {
				_statistics = false;
				_verbose = false;
				_readfromstdin = false;
				_interactive = false;
				_warning = vector<bool>(5,true);
				_style = vector<bool>(1,false);
				_exec = "";
		  }

};

/** Inference options **/

enum OutputFormat	{ OF_TXT, OF_IDP, OF_ECNF };
enum ModelFormat	{ MF_THREEVAL, MF_TWOVAL, MF_ALL };

class InfOptions {
	
	public:

		// Name and place
		string			_name;		// the name of the options
		ParseInfo		_pi;		

		// Attributes
		unsigned int	_nrmodels;		// the number of models to compute
		OutputFormat	_format;		// use specified format for the output
		ModelFormat		_modelformat;	// make results of MX twovalued
		int				_satverbosity;
		bool			_printtypes;
		bool			_usingcp;
		bool			_trace;
		
		// Constructor (default options)
		InfOptions(const string& name, const ParseInfo& pi) : 
			_name(name), 
			_pi(pi),
			_nrmodels(1),
			_format(OF_IDP),
			_modelformat(MF_ALL),
			_satverbosity(0),
			_printtypes(true),
			_usingcp(true),
			_trace(false)
			{ }
		InfOptions(InfOptions* opts) : _name(""), _pi() { set(opts);	}

		// Setters
		void set(InfOptions* opt) {
			_nrmodels		= opt->_nrmodels;
			_format			= opt->_format;
			_modelformat	= opt->_modelformat;
			_satverbosity	= opt->_satverbosity;
			_printtypes		= opt->_printtypes;
			_usingcp		= opt->_usingcp;
			_trace			= opt->_trace;
		}
		void set(const string& optname,const string& val, ParseInfo* pi = 0);
		void set(const string& optname,double, ParseInfo* pi = 0);
		void set(const string& optname,bool, ParseInfo* pi = 0);
		void set(const string& optname,int, ParseInfo* pi = 0);

		// Inspectors
		static bool isoption(const string& str) {
			bool r = false;
			if(str == "nrmodels") r = true;
			else if(str == "modelformat") r = true;
			else if(str == "language") r = true;
			else if(str == "satverbosity") r = true;
			else if(str == "printtypes") r = true;
			else if(str == "usingcp") r = true;
			else if(str == "trace") r = true;
			return r;
		}
		const string& name() const { return _name; }

		const ParseInfo& pi() const { return _pi;	}

		TypedInfArg	get(const string& optname) const;

};

#endif
