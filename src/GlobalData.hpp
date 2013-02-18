/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

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
	DomainElementFactory* _domainelemFactory;
	int _idcounter;

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

	static bool shouldTerminate;
	static GlobalData* instance();
	static DomainElementFactory* getGlobalDomElemFactory();
	static Namespace* getGlobalNamespace();
	static Namespace* getStdNamespace();
	static void close();

	static bool terminateRequested() {
		return shouldTerminate;
	}
	static void reset() {
		shouldTerminate = false;
	}
	void notifyTerminateRequested() {
		shouldTerminate = true;
		for (auto i = _monitors.cbegin(); i < _monitors.cend(); ++i) {
			(*i)->notifyTerminateRequested();
		}
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

#define CHECKTERMINATION \
	if(GlobalData::terminateRequested()){\
		throw IdpException("Terminate requested");\
	}
