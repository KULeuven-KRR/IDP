/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include "options.hpp"
#include "error.hpp"
using namespace std;

bool IntOption::value(int v) {
	if(v >= _lower && v <= _upper) {
		_value = v;
		return true;
	}
	else return false;
}

bool FloatOption::value(double v) {
	if(v >= _lower && v <= _upper) {
		_value = v;
		return true;
	}
	else return false;
}

class EnumeratedStringOption : public StringOption {
	private:
		unsigned int	_value;
		vector<string>	_possvalues;
	public:
		EnumeratedStringOption(const vector<string>& possvalues, const string& val) : _possvalues(possvalues) {
			for(unsigned int n = 0; n < _possvalues.size(); ++n) {
				if(_possvalues[n] == val) { _value = n; break;	}
			}
		}
		~EnumeratedStringOption() { }
		const string& value()	const { return _possvalues[_value];	}
		bool value(const string& val) {
			for(unsigned int n = 0; n < _possvalues.size(); ++n) {
				if(_possvalues[n] == val) { _value = n; return true;	}
			}
			return false;
		}
};

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	_booloptions["printtypes"]		= true;
	_booloptions["usingcp"]			= true;
	_booloptions["trace"]			= false;

	_intoptions["satverbosity"]		= new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["groundverbosity"]	= new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["nrmodels"]			= new IntOption(0,numeric_limits<int>::max(),1);

	vector<string> ls(3); ls[0] = "idp"; ls[1] = "txt"; ls[2] = "ecnf";
	vector<string> mf(3); mf[0] = "threevalued"; mf[1] = "twovalued"; mf[2] = "all";
	_stringoptions["language"]		= new EnumeratedStringOption(ls,"idp");
	_stringoptions["modelformat"]	= new EnumeratedStringOption(mf,"threevalued");
}

Options::~Options() {
	for(map<string,StringOption*>::const_iterator it = _stringoptions.begin(); it != _stringoptions.end(); ++it) {
		delete(it->second);
	}
	for(map<string,IntOption*>::const_iterator it = _intoptions.begin(); it != _intoptions.end(); ++it) {
		delete(it->second);
	}
	for(map<string,FloatOption*>::const_iterator it = _floatoptions.begin(); it != _floatoptions.end(); ++it) {
		delete(it->second);
	}
}

bool Options::isoption(const string& optname) const {
	if(_stringoptions.find(optname) != _stringoptions.end())	return true;
	if(_booloptions.find(optname) != _booloptions.end())		return true;
	if(_intoptions.find(optname) != _intoptions.end())			return true;
	if(_floatoptions.find(optname) != _floatoptions.end())		return true;
	return false;
}

void Options::setvalues(Options* opts) {
	for(map<string,StringOption*>::const_iterator it = opts->stringoptions().begin(); it != opts->stringoptions().end(); ++it) {
		setvalue(it->first,it->second->value());
	}
	for(map<string,IntOption*>::const_iterator it = opts->intoptions().begin(); it != opts->intoptions().end(); ++it) {
		setvalue(it->first,it->second->value());
	}
	for(map<string,FloatOption*>::const_iterator it = opts->floatoptions().begin(); it != opts->floatoptions().end(); ++it) {
		setvalue(it->first,it->second->value());
	}
	for(map<string,bool>::const_iterator it = opts->booloptions().begin(); it != opts->booloptions().end(); ++it) {
		setvalue(it->first,it->second);
	}
}

bool Options::setvalue(const string& opt, int val) {
	map<string,IntOption*>::iterator it = _intoptions.find(opt);
	if(it != _intoptions.end()) {
		return it->second->value(val);
	}
	else return false;
}

bool Options::setvalue(const string& opt, double val) {
	map<string,FloatOption*>::iterator it = _floatoptions.find(opt);
	if(it != _floatoptions.end()) {
		return it->second->value(val);
	}
	else return false;
}

bool Options::setvalue(const string& opt, const string& val) {
	map<string,StringOption*>::iterator it = _stringoptions.find(opt);
	if(it != _stringoptions.end()) {
		return it->second->value(val);
	}
	else return false;
}

bool Options::setvalue(const string& opt, bool val) {
	map<string,bool>::iterator it = _booloptions.find(opt);
	if(it != _booloptions.end()) {
		_booloptions[opt] = val;
		return true;
	}
	else return false;
}

Language Options::language() const {
	string lan = (_stringoptions.find("language")->second)->value();
	if(lan == "idp") return LAN_IDP;
	else if(lan == "txt") return LAN_TXT;
	else if(lan == "ecnf") return LAN_ECNF;
	else return LAN_IDP;
}

bool Options::printtypes() const {
	return _booloptions.find("printtypes")->second;
}

int Options::nrmodels() const {
	return _intoptions.find("nrmodels")->second->value();
}

int Options::satverbosity() const {
	return _intoptions.find("satverbosity")->second->value();
}

int Options::groundverbosity() const {
	return _intoptions.find("groundverbosity")->second->value();
}

bool Options::cpsupport() const {
	return _booloptions.find("usingcp")->second;
}

ostream& Options::put(ostream& output) const {
	for(map<string,StringOption*>::const_iterator it = _stringoptions.begin(); it != _stringoptions.end(); ++it) {
		output << it->first << " = " << it->second->value() << endl;
	}
	for(map<string,IntOption*>::const_iterator it = _intoptions.begin(); it != _intoptions.end(); ++it) {
		output << it->first << " = " << it->second->value() << endl;
	}
	for(map<string,FloatOption*>::const_iterator it = _floatoptions.begin(); it != _floatoptions.end(); ++it) {
		output << it->first << " = " << it->second->value() << endl;
	}
	for(map<string,bool>::const_iterator it = _booloptions.begin(); it != _booloptions.end(); ++it) {
		output << it->first << " = " << (it->second ? "true" : "false") << endl;
	}
	return output;
}

string Options::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}
