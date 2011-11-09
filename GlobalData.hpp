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
};

#endif /* GLOBALDATA_HPP_ */
