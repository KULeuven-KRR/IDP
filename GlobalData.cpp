/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "GlobalData.hpp"
#include "structure.hpp"
#include "namespace.hpp"
#include "parser/clconst.hpp"
#include "IdpException.hpp"
#include <iostream>
#include "options.hpp"

using namespace std;

GlobalData::GlobalData() :
		_globalNamespace(Namespace::createGlobal()), _inserter(_globalNamespace), _domainelemFactory(DomainElementFactory::createGlobal()),
		_terminateRequested(false),
		_options(new Options()), _tabsizestack(), _errorcount(0) {
	_tabsizestack.push(0);
}

GlobalData::~GlobalData() {
	delete (_globalNamespace);
	delete (_domainelemFactory);
	for(auto i=_openfiles.cbegin(); i!=_openfiles.cend(); ++i){
		fclose(*i);
	}
}

void GlobalData::setOptions(Options* options) {
	Assert(_options!=NULL);
	_options = options;
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
	Assert(_instance!=NULL);
	delete (_instance);
	_instance = NULL;
}

void GlobalData::setConstValue(const std::string& name1, const std::string& name2){
	CLConst* c;
	if(isInt(name2))
		c = new IntClConst(toInt(name2));
	else if(isDouble(name2))
		c = new DoubleClConst(toDouble(name2));
	else if(name2.size() == 1)
		c = new CharCLConst(name2[0],false);
	else if(name2.size() == 3 && name2[0] == '\'' && name2[2] == '\'')
		c = new CharCLConst(name2[1],true);
	else if(name2.size() >= 2 && name2[0] == '"' && name2[name2.size()-1] == '"')
		c = new StrClConst(name2.substr(1,name2.size()-2),true);
	else c = new StrClConst(name2,false);
	clconsts[name1] = c;
}

FILE* GlobalData::openFile(const char* filename, const char* mode){
	auto f = fopen(filename, mode);
	if(f==NULL){
		return f;
		//throw IdpException("Could not open file.\n");
	}
	_openfiles.insert(f);
	return f;
}

void GlobalData::closeFile(FILE* filepointer){
	Assert(filepointer!=NULL);
	_openfiles.erase(filepointer);
	fclose(filepointer);
}

void GlobalData::setTabSize(unsigned int size){
	_tabsizestack.push(size);
}
void GlobalData::resetTabSize(){
	if(_tabsizestack.size()>0){
		_tabsizestack.pop();
	}
}
unsigned int GlobalData::getTabSize() const{
	return _tabsizestack.top();
}

GlobalData* getGlobal(){
	return GlobalData::instance();
}
