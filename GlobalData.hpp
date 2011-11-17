#ifndef GLOBALDATA_HPP_
#define GLOBALDATA_HPP_

#include "insert.hpp" // TODO remove inclusion (and make it a pointer)
#include <cstdio>
#include <set>
#include <string>
#include <map>

class Namespace;
class DomainElementFactory;
class Options;
class CLConst;

class GlobalData {
private:
	Namespace* _globalNamespace;
	Insert _inserter;
	std::map<std::string,CLConst*> clconsts;
	DomainElementFactory* _domainelemFactory;
	Options* _options;
	unsigned int _errorcount;
	std::set<FILE*> _openfiles;

	GlobalData();

public:
	~GlobalData();

	static GlobalData* instance();
	static DomainElementFactory* getGlobalDomElemFactory();
	static Namespace* getGlobalNamespace();
	static void close();

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
	void setOptions(Options* options) {
		_options = options;
	}

	void notifyOfError(){
		_errorcount++;
	}

	unsigned int getErrorCount(){
		return _errorcount;
	}

	FILE* openFile(const char* filename, const char* mode);
	void closeFile(FILE* file);
};

#endif /* GLOBALDATA_HPP_ */
