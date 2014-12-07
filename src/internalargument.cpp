/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

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
		_value._string = new std::string(*(el->value()._string));
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
Structure* InternalArgument::get<Structure*>() {
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
UserProcedure* InternalArgument::get<UserProcedure*>() {
	return _value._procedure;
}

template<>
SortIterator* InternalArgument::get<SortIterator*>() {
	return _value._sortiterator;
}

template<>
const Compound* InternalArgument::get<const Compound*>() {
	return _value._compound;
}

template<>
TableIterator* InternalArgument::get<TableIterator*>() {
	return _value._tableiterator;
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

template<>
Sort* InternalArgument::get<Sort*>() {
	auto sorts = _value._sort;
	if (sorts == NULL) {
		return NULL;
	}
	if (sorts->size() == 0) {
		throw IdpException("Empty set of sorts passed but sort was requested.");
	} else if (sorts->size() > 1) {
		throw IdpException("Internal error: ambiguous sort.");
	} else {
		return *(sorts->begin());
	}
}

template<>
Function* InternalArgument::get<Function*>() {
	auto funcs = _value._function;
	if (funcs == NULL) {
		return NULL;
	}
	if (funcs->size() == 0) {
		throw IdpException("Empty set of functions passed but function was requested.");
	} else if (funcs->size() > 1) {
		throw IdpException("Internal error: ambiguous function.");
	} else {
		return *(funcs->begin());
	}
}
template<>
Predicate* InternalArgument::get<Predicate*>() {
	auto preds = _value._predicate;
	if (preds == NULL) {
		return NULL;
	}
	if (preds->size() == 0) {
		throw IdpException("Empty set of predicates passed but function was requested.");
	} else if (preds->size() > 1) {
		throw IdpException("Internal error: ambiguous predicate.");
	} else {
		return *(preds->begin());
	}
}
template<>
const FOBDD* InternalArgument::get<const FOBDD*>() {
	return _value._fobdd;
}
template<>
WrapModelIterator* InternalArgument::get<WrapModelIterator*>() {
	return _value._modelIterator;
}

template<>
TwoValuedStructureIterator* InternalArgument::get<TwoValuedStructureIterator*>() {
	return _value._twoValuedIterator;
}
