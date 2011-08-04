/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include <limits>
#include "options.hpp"
#include "error.hpp"
#include <algorithm>
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

//TODO code van minisatid 2.3 of later gebruiken om makkelijker opties toe te voegen te doen

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	_booloptions["printtypes"]			= true;
	_booloptions["cpsupport"]			= false;
	_booloptions["trace"]				= false;
	_booloptions["autocomplete"]		= true;
	_booloptions["longnames"]			= false;
	_booloptions["relativepropsteps"]	= true;
	_booloptions["createtranslation"]	= false;

	_intoptions["satverbosity"]			= new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["groundverbosity"]		= new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["propagateverbosity"]	= new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["nrmodels"]				= new IntOption(0,numeric_limits<int>::max(),1);
	_intoptions["nrpropsteps"]			= new IntOption(0,numeric_limits<int>::max(),4);
	_intoptions["longestbranch"]		= new IntOption(0,numeric_limits<int>::max(),8);

	vector<string> ls(4); ls[0] = "idp"; ls[1] = "txt"; ls[2] = "ecnf"; ls[3] = "tptp";
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
	else if(lan == "tptp") return LAN_TPTP;
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

int Options::propagateverbosity() const {
	return _intoptions.find("propagateverbosity")->second->value();
}

int Options::nrpropsteps() const {
	return _intoptions.find("nrpropsteps")->second->value();
}

int Options::longestbranch() const {
	return _intoptions.find("longestbranch")->second->value();
}

bool Options::cpsupport() const {
	return _booloptions.find("cpsupport")->second;
}

bool Options::autocomplete() const {
	return _booloptions.find("autocomplete")->second;
}

bool Options::trace() const {
	return _booloptions.find("trace")->second;
}

bool Options::longnames() const {
	return _booloptions.find("longnames")->second;
}

bool Options::relativepropsteps() const {
	return _booloptions.find("relativepropsteps")->second;
}

bool Options::writeTranslation() const {
	return _booloptions.find("createtranslation")->second;
}

template<class OptionList, class StringList>
void getStringFromOption(const OptionList& list, StringList& newlist){
	for(auto it = list.begin(); it != list.end(); ++it) {
		stringstream ss;
		ss << it->first << " = " << it->second->value();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	getStringFromOption(_stringoptions, optionslines);
	getStringFromOption(_intoptions, optionslines);
	getStringFromOption(_floatoptions, optionslines);
	for(map<string,bool>::const_iterator it = _booloptions.begin(); it != _booloptions.end(); ++it) {
		output << it->first << " = " << (it->second ? "true" : "false") << "\n";
	}

	sort(optionslines.begin(), optionslines.end());
	for(auto i=optionslines.begin(); i<optionslines.end(); ++i){
		output <<*i <<"\n";
	}

	return output;
}

string Options::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}
 
