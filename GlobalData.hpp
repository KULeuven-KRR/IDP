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

	bool _terminateRequested;

	Options* _options;
	std::stack<unsigned int> _tabsizestack;

	unsigned int _errorcount;
	std::set<FILE*> _openfiles;

	std::vector<TerminateMonitor*> monitors;

	GlobalData();

public:
	~GlobalData();

	static GlobalData* instance();
	static DomainElementFactory* getGlobalDomElemFactory();
	static Namespace* getGlobalNamespace();
	static void close();

	bool terminateRequested() const { return _terminateRequested; }
	void addTerminationMonitor(TerminateMonitor* m){
		monitors.push_back(m);
	}
	void notifyTerminateRequested() {
		_terminateRequested = true;
		for(auto i=monitors.cbegin(); i<monitors.cend(); ++i){
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

	void setTabSize(unsigned int);
	void resetTabSize();
	unsigned int getTabSize() const;
};

GlobalData* getGlobal();

template<typename OptionType>
typename OptionTypeTraits<OptionType>::ValueType getOption(OptionType type){
	return getGlobal()->getOptions()->getValue(type);
}

#endif /* GLOBALDATA_HPP_ */
