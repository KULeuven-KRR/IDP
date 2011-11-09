#include "GlobalData.hpp"
#include "structure.hpp"
#include "namespace.hpp"

using namespace std;

GlobalData::GlobalData() :
		_globalNamespace(Namespace::createGlobal()), _domainelemFactory(DomainElementFactory::createGlobal()), _options(NULL), _errorcount(0) {
}

GlobalData::~GlobalData() {
	delete (_globalNamespace);
	delete (_domainelemFactory);
}

GlobalData* _instance = NULL;

GlobalData* GlobalData::instance() {
	if (_instance == NULL) {
		_instance = new GlobalData();
	}
	return _instance;
}

DomainElementFactory* GlobalData::getGlobalDomElemFactory(){
	return instance()->getDomElemFactory();
}
Namespace* GlobalData::getGlobalNamespace(){
	return instance()->getNamespace();
}

void GlobalData::close() {
	assert(_instance!=NULL);
	delete (_instance);
	_instance = NULL;
}
