#ifndef GLOBALDATA_HPP_
#define GLOBALDATA_HPP_

class Namespace;
class DomainElementFactory;
class Options;

class GlobalData {
private:
	Namespace* _globalNamespace;
	DomainElementFactory* _domainelemFactory;
	Options* _options;
	unsigned int _errorcount;

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
};

#endif /* GLOBALDATA_HPP_ */
