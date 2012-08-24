/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef GLOBALDATA_HPP_
#define GLOBALDATA_HPP_

#include <cstdio>
#include <set>
#include <stack>
#include <string>
#include <map>

#include "options.hpp"

class Insert;
class Namespace;
class DomainElementFactory;
class Options;
class CLConst;

class TerminateMonitor {
public:
	virtual ~TerminateMonitor() {
	}
	virtual void notifyTerminateRequested() = 0;
};

class GlobalData {
private:
	Namespace *_globalNamespace, *_stdNamespace;
	Insert* _inserter;
	std::map<std::string, CLConst*> clconsts;
	DomainElementFactory* _domainelemFactory;
	int _idcounter;

	bool _terminateRequested;

	Options* _options;
	std::stack<size_t> _tabsizestack;

	std::vector<std::string> _errors, _warnings;
	std::set<FILE*> _openfiles;

	std::vector<TerminateMonitor*> _monitors;

	GlobalData();

public:
	~GlobalData();

	int getNewID() {
		return ++_idcounter;
	}

	static GlobalData* instance();
	static DomainElementFactory* getGlobalDomElemFactory();
	static Namespace* getGlobalNamespace();
	static Namespace* getStdNamespace();
	static void close();

	bool terminateRequested() const {
		return _terminateRequested;
	}
	void addTerminationMonitor(TerminateMonitor* m) {
		_monitors.push_back(m);
	}
	void removeTerminationMonitor(TerminateMonitor* m) {
		for(auto i=_monitors.begin(); i<_monitors.end(); ++i) {
			if(*i==m){
				_monitors.erase(i);
				break;
			}
		}
	}
	void reset() {
		_terminateRequested = false;
	}
	void notifyTerminateRequested() {
		_terminateRequested = true;
		for (auto i = _monitors.cbegin(); i < _monitors.cend(); ++i) {
			(*i)->notifyTerminateRequested();
		}
	}

	Namespace* getNamespace() {
		return _globalNamespace;
	}
	Namespace* getStd() {
		return _stdNamespace;
	}

	DomainElementFactory* getDomElemFactory() {
		return _domainelemFactory;
	}

	Insert& getInserter() {
		return *_inserter;
	}

	const std::map<std::string, CLConst*>& getConstValues() const {
		return clconsts;
	}

	void setConstValue(const std::string& name1, const std::string& name2);

	Options* getOptions() {
		return _options;
	}
	void setOptions(Options* options);


	void notifyOfError(const std::string& errormessage) {
		_errors.push_back(errormessage);
	}
	const std::vector<std::string>& getErrors() const {
		return _errors;
	}
	unsigned int getErrorCount() const {
		return _errors.size();
	}
	void notifyOfWarning(const std::string& errormessage) {
		_warnings.push_back(errormessage);
	}
	const std::vector<std::string>& getWarnings() const {
		return _warnings;
	}
	unsigned int getWarningCount() const {
		return _warnings.size();
	}
	void clearStats(){
		_errors.clear();
		_warnings.clear();
	}

	FILE* openFile(const char* filename, const char* mode);
	void closeFile(FILE* file);

	void setTabSize(size_t);
	void resetTabSize();
	size_t getTabSize() const;
};

GlobalData* getGlobal();

template<typename OptionsType>
typename OptionTypeTraits<OptionsType>::ValueType getOption(OptionsType type) {
	if (not isVerbosityOption(type)) {
		return getGlobal()->getOptions()->getValue(type);
	}
	return getGlobal()->getOptions()->getValue(OptionType::VERBOSITY)->getValue(type);
}

template<typename OptionsType>
void setOption(OptionsType type, typename OptionTypeTraits<OptionsType>::ValueType value) {
	if (not isVerbosityOption(type)) {
		getGlobal()->getOptions()->setValue(type, value);
		return;
	}
	getGlobal()->getOptions()->getValue(OptionType::VERBOSITY)->setValue(type, value);
}

// TODO improve check by bool flag!
#define CHECKTERMINATION \
		if(GlobalData::instance()->terminateRequested()){\
			throw IdpException("Terminate requested");\
		}
#endif /* GLOBALDATA_HPP_ */
