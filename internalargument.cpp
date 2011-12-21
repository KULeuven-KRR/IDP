/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <set>
#include "structure.hpp"
#include "internalargument.hpp"

std::set<SortTable*> domains;
std::set<std::vector<InternalArgument>* > arguments;
std::set<AbstractStructure*> structures;

void addToGarbageCollection(SortTable* table){
	domains.insert(table);
}

void addToGarbageCollection(std::vector<InternalArgument>* list){
	arguments.insert(list);
}

void addToGarbageCollection(AbstractStructure* structure){
	structures.insert(structure);
}

void garbageCollect(SortTable* table){
	auto it = domains.find(table);
	if(it != domains.cend()) {
		delete(*it);
		domains.erase(table);
	}
}

void garbageCollect(std::vector<InternalArgument>* list){
	auto it = arguments.find(list);
	if(it != arguments.cend()) {
		delete(*it);
		arguments.erase(list);
	}
}

void garbageCollect(AbstractStructure* list){
	auto it = structures.find(list);
	if(it != structures.cend()) {
		delete(*it);
		structures.erase(list);
	}
}

template<>
AbstractTheory* InternalArgument::get<AbstractTheory*>(){
	return theory();
}

template<>
AbstractStructure* InternalArgument::get<AbstractStructure*>(){
	return structure();
}

template<>
Vocabulary* InternalArgument::get<Vocabulary*>(){
	return vocabulary();
}

// TODO mults?

template<>
int InternalArgument::get<int>(){
	return _value._int;
}

template<>
bool InternalArgument::get<bool>(){
	return _value._boolean;
}

template<>
SortIterator* InternalArgument::get<SortIterator*>(){
	return _value._sortiterator;
}

template<>
TableIterator* InternalArgument::get<TableIterator*>(){
	return _value._tableiterator;
}

template<>
const DomainAtom* InternalArgument::get<const DomainAtom*>(){
	return _value._domainatom;
}

template<>
Options* InternalArgument::get<Options*>(){
	return options();
}

template<>
SortTable* InternalArgument::get<SortTable*>(){
	return _value._domain;
}

template<>
std::string* InternalArgument::get<std::string*>(){
	return _value._string;
}

template<>
const PredTable* InternalArgument::get<const PredTable*>(){
	return _value._predtable;
}

template<>
Formula* InternalArgument::get<Formula*>(){
	return _value._formula;
}

template<>
LuaTraceMonitor* InternalArgument::get<LuaTraceMonitor*>(){
	return _value._tracemonitor;
}

template<>
Namespace* InternalArgument::get<Namespace*>(){
	return space();
}

template<>
PredInter* InternalArgument::get<PredInter*>(){
	return _value._predinter;
}

template<>
ElementTuple* InternalArgument::get<ElementTuple*>(){
	return _value._tuple;
}

template<>
std::vector<InternalArgument>* InternalArgument::get<std::vector<InternalArgument>*>(){
	return _value._table;
}

template<>
Query* InternalArgument::get<Query*>(){
	return _value._query;
}
