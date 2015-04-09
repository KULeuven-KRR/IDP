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

class DeleteMe { //All objects that should be deleted on exit should be DeleteMe's and should register with GlobalData
public:
	virtual ~DeleteMe(){
	}
};

class GlobalData {
private:
	Namespace *_globalNamespace, *_stdNamespace;
	Insert* _inserter;
	DomainElementFactory* _domainelemFactory;
	int _idcounter;

	bool _outofresources;

	Options* _options;
	std::stack<size_t> _tabsizestack;

	std::set<std::string> _errors, _warnings;
	std::set<std::string> _parsedfiles;
	std::set<FILE*> _openfiles;
	std::set<char*> _temp_file_names;

	std::vector<TerminateMonitor*> _monitors;
	std::vector<DeleteMe*> _deleteMes;

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
	bool timedout() const {
		return shouldTerminate && _outofresources;
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

	void registerForDeletion(DeleteMe* dm){
		_deleteMes.push_back(dm);
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
	void notifyOutOfResources() {
		_outofresources = true;
		shouldTerminate = true;
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

	Options* getOptions() {
		return _options;
	}
	void setOptions(Options* options);


	void notifyOfError(const std::string& errormessage);
	const std::set<std::string>& getErrors() const {
		return _errors;
	}
	unsigned int getErrorCount() const {
		return _errors.size();
	}
	void notifyOfWarning(const std::string& errormessage);
	const std::set<std::string>& getWarnings() const {
		return _warnings;
	}
	unsigned int getWarningCount() const {
		return _warnings.size();
	}
	void clearStats(){
		_errors.clear();
		_warnings.clear();
	}

	void notifyParsed(const std::string& filename){
		_parsedfiles.insert(filename);
	}
	bool alreadyParsed(const std::string& filename){
		return _parsedfiles.find(filename)!=_parsedfiles.cend();
	}
	FILE* openFile(const char* filename, const char* mode);
	char* getTempFileName();
	void closeFile(FILE* file);
	void removeTempFile(char* file);

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
	setOption(getGlobal()->getOptions(), type, value);
}

template<typename OptionsType>
void setOption(Options* options, OptionsType type, typename OptionTypeTraits<OptionsType>::ValueType value) {
	if (not isVerbosityOption(type)) {
		options->setValue(type, value);
		return;
	}
	options->getValue(OptionType::VERBOSITY)->setValue(type, value);
}

void callTerminate();

#define CHECKTERMINATION callTerminate();

