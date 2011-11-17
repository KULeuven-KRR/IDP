#ifndef GLOBALDATA_HPP_
#define GLOBALDATA_HPP_

#include "insert.hpp" // TODO remove inclusion (and make it a pointer)
#include <cstdio>
#include <set>

class Namespace;
class DomainElementFactory;
class Options;

class GlobalData {
private:
	Namespace* _globalNamespace;
	Insert _inserter;
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
