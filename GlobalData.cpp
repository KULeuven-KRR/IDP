#include "GlobalData.hpp"
#include "structure.hpp"
#include "namespace.hpp"
#include "IdpException.hpp"
#include <iostream>

using namespace std;

GlobalData::GlobalData() :
		_globalNamespace(Namespace::createGlobal()), _inserter(_globalNamespace), _domainelemFactory(DomainElementFactory::createGlobal()), _options(NULL), _errorcount(0) {
}

GlobalData::~GlobalData() {
	delete (_globalNamespace);
	delete (_domainelemFactory);
	for(auto i=_openfiles.cbegin(); i!=_openfiles.cend(); ++i){
		cerr <<"closing " <<*i <<"\n";
		fclose(*i);
	}
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

FILE* GlobalData::openFile(const char* filename, const char* mode){
	auto f = fopen(filename, mode);
	if(f==NULL){
		return f;
		//throw IdpException("Could not open file.\n");
	}
	cerr <<"opened " <<filename <<" as " <<f <<"\n";
	_openfiles.insert(f);
	return f;
}

void GlobalData::closeFile(FILE* filepointer){
	assert(filepointer!=NULL);
	cerr <<"closing " <<filepointer <<"\n";
	_openfiles.erase(filepointer);
	fclose(filepointer);
}
