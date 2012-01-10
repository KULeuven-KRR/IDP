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

#include "insert.hpp" // TODO remove inclusion (and make it a pointer)
#include <cstdio>
#include <set>
#include <stack>
#include <string>
#include <map>

#include "options.hpp"

class Namespace;
class DomainElementFactory;
class Options;
class CLConst;

class TerminateMonitor{
public:
	virtual ~TerminateMonitor(){}
	virtual void notifyTerminateRequested() = 0;
};

class GlobalData {
private:
	Namespace* _globalNamespace;
	Insert _inserter;
	std::map<std::string,CLConst*> clconsts;
	DomainElementFactory* _domainelemFactory;
	int _idcounter;

	bool _terminateRequested;

	Options* _options;
	std::stack<size_t> _tabsizestack;

	unsigned int _errorcount;
	std::set<FILE*> _openfiles;

	std::vector<TerminateMonitor*> _monitors;

	GlobalData();

public:
	~GlobalData();

	int getNewID(){
		return ++_idcounter;
	}

	static GlobalData* instance();
	static DomainElementFactory* getGlobalDomElemFactory();
	static Namespace* getGlobalNamespace();
	static void close();

	bool terminateRequested() const { return _terminateRequested; }
	void addTerminationMonitor(TerminateMonitor* m){
		_monitors.push_back(m);
	}
	void reset(){
		_terminateRequested = false;
	}
	void notifyTerminateRequested() {
		_terminateRequested = true;
		for(auto i=_monitors.cbegin(); i<_monitors.cend(); ++i){
			(*i)->notifyTerminateRequested();
		}
	}

	Namespace* getNamespace() {
		return _globalNamespace;
	}

	DomainElementFactory* getDomElemFactory() {
		return _domainelemFactory;
	}

	Insert& getInserter(){
		return _inserter;
	}

	const std::map<std::string,CLConst*>& getConstValues() const{
		return clconsts;
	}

	void setConstValue(const std::string& name1, const std::string& name2);

	Options* getOptions() {
		return _options;
	}
	void setOptions(Options* options);

	void notifyOfError(){
		_errorcount++;
	}

	unsigned int getErrorCount(){
		return _errorcount;
	}

	FILE* openFile(const char* filename, const char* mode);
	void closeFile(FILE* file);

	void setTabSize(size_t);
	void resetTabSize();
	size_t getTabSize() const;
};

GlobalData* getGlobal();

template<typename OptionType>
typename OptionTypeTraits<OptionType>::ValueType getOption(OptionType type){
	return getGlobal()->getOptions()->getValue(type);
}

template<typename OptionType>
void setOption(OptionType type, typename OptionTypeTraits<OptionType>::ValueType value){
	return getGlobal()->getOptions()->setValue(type, value);
}

// TODO improve check by bool flag!
#define CHECKTERMINATION \
	if(GlobalData::instance()->terminateRequested()){\
			throw IdpException("Terminate requested");\
	}

#endif /* GLOBALDATA_HPP_ */
