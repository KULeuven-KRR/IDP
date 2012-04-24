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
#include "structure/DomainElement.hpp"
#include "internalargument.hpp"

// Note: following containers are used for garbage collection.
std::set<std::vector<InternalArgument>*> lists;

InternalArgument::InternalArgument(const DomainElement* el) {
	_type = AT_NIL;
	switch (el->type()) {
	case DET_INT:
		_type = AT_INT;
		_value._int = el->value()._int;
		break;
	case DET_DOUBLE:
		_type = AT_DOUBLE;
		_value._double = el->value()._double;
		break;
	case DET_STRING:
		_type = AT_STRING;
		_value._string = StringPointer(*(el->value()._string));
		break;
	case DET_COMPOUND:
		_type = AT_COMPOUND;
		_value._compound = el->value()._compound;
		break;
	}
}

void addToGarbageCollection(std::vector<InternalArgument>* list) {
	lists.insert(list);
}

void garbageCollectInternalArgumentVectors() {
	for (auto i=lists.cbegin(); i!=lists.cend(); ++i) {
		delete(*i);
	}
	lists.clear();
}

template<>
AbstractTheory* InternalArgument::get<AbstractTheory*>() {
	return theory();
}

template<>
AbstractStructure* InternalArgument::get<AbstractStructure*>() {
	return structure();
}

template<>
Vocabulary* InternalArgument::get<Vocabulary*>() {
	return vocabulary();
}

template<>
int InternalArgument::get<int>() {
	return _value._int;
}

template<>
double InternalArgument::get<double>() {
	return _value._double;
}

template<>
bool InternalArgument::get<bool>() {
	return _value._boolean;
}

template<>
SortIterator* InternalArgument::get<SortIterator*>() {
	return _value._sortiterator;
}

template<>
TableIterator* InternalArgument::get<TableIterator*>() {
	return _value._tableiterator;
}

template<>
const DomainAtom* InternalArgument::get<const DomainAtom*>() {
	return _value._domainatom;
}

template<>
Options* InternalArgument::get<Options*>() {
	return options();
}

template<>
SortTable* InternalArgument::get<SortTable*>() {
	return _value._domain;
}

template<>
std::string* InternalArgument::get<std::string*>() {
	return _value._string;
}

template<>
const PredTable* InternalArgument::get<const PredTable*>() {
	return _value._predtable;
}

template<>
Formula* InternalArgument::get<Formula*>() {
	return _value._formula;
}

template<>
LuaTraceMonitor* InternalArgument::get<LuaTraceMonitor*>() {
	return _value._tracemonitor;
}

template<>
Namespace* InternalArgument::get<Namespace*>() {
	return space();
}

template<>
PredInter* InternalArgument::get<PredInter*>() {
	return _value._predinter;
}

template<>
ElementTuple* InternalArgument::get<ElementTuple*>() {
	return _value._tuple;
}

template<>
std::vector<InternalArgument>* InternalArgument::get<std::vector<InternalArgument>*>() {
	return _value._table;
}

template<>
Query* InternalArgument::get<Query*>() {
	return _value._query;
}

template<>
Term* InternalArgument::get<Term*>() {
	return _value._term;
}
