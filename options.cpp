/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
		const string& value()	const { return _possvalues[_value];	}
		bool value(const string& val) {
			for(unsigned int n = 0; n < _possvalues.size(); ++n) {
				if(_possvalues[n] == val) { _value = n; return true;	}
			}
			return false;
		}
};

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	_booloptions["printtypes"]	= true;
	_booloptions["usingcp"]		= true;
	_booloptions["trace"]		= false;

	_intoptions["satverbosity"] = new IntOption(0,numeric_limits<int>::max(),0);
	_intoptions["nrmodels"]		= new IntOption(0,numeric_limits<int>::max(),1);

	vector<string> ls(3); ls[0] = "idp"; ls[1] = "txt"; ls[2] = "ecnf";
	vector<string> mf(3); mf[0] = "threevalued"; mf[1] = "twovalued"; mf[2] = "all";
	_stringoptions["language"]		= new EnumeratedStringOption(ls,"idp");
	_stringoptions["modelformat"]	= new EnumeratedStringOption(mf,"threevalued");
}

bool Options::setvalue(const string& opt, const string& val) {
	map<string,StringOption*>::iterator it = _stringoptions.find(opt);
	if(it != _stringoptions.end()) {
		return it->second->value(val);
	}
	else return false;
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

bool Options::setvalue(const string& opt, bool val) {
	map<string,bool>::iterator it = _booloptions.find(opt);
	if(it != _booloptions.end()) {
		_booloptions[opt] = val;
		return true;
	}
	else return false;
}
