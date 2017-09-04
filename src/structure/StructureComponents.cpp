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

#include <cmath> // double std::abs(double) and double std::pow(double,double)
#include <cstdlib> // int std::abs(int)
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "utils/ListUtils.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/Estimations.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "lua/luaconnection.hpp" //TODO break connection with lua!
#include "visitors/StructureVisitor.hpp"
#include "information/EnumerateSymbolicTable.hpp"

#include "generators/GeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/ComparisonGenerator.hpp"

#include "NumericOperations.hpp"

#include "printers/idpprinter.hpp" //TODO only for debugging
using namespace std;

bool PredTable::finite() const {
	return _table->finite(_universe);
}
bool PredTable::empty() const {
	return _table->empty(_universe);
}
unsigned int PredTable::arity() const {
	return _universe.arity();
}
bool PredTable::approxFinite() const {
	return _table->approxFinite(_universe);
}
bool PredTable::approxEmpty() const {
	return _table->approxEmpty(_universe);
}
bool PredTable::approxEqual(const PredTable* pt) const{
	return _table->approxEqual(pt->internTable(),_universe);
}
bool PredTable::approxInverse(const PredTable* pt) const{
	return _table->approxInverse(pt->internTable(),_universe);
}
bool PredTable::contains(const ElementTuple& tuple) const {
	return _table->contains(tuple, _universe);
}
tablesize PredTable::size() const {
	return _table->size(_universe);
}

const Universe& PredTable::universe() const {
	return _universe;
}

InternalPredTable* PredTable::internTable() const {
	return _table;
}

bool SortTable::finite() const {
	return _table->finite();
}
bool SortTable::empty() const {
	return _table->empty();
}
bool SortTable::approxFinite() const {
	return _table->approxFinite();
}
bool SortTable::approxEmpty() const {
	return _table->approxEmpty();
}
unsigned int SortTable::arity() const {
	return 1;
}
tablesize SortTable::size() const {
	return _table->size();
}
bool SortTable::contains(const ElementTuple& tuple) const {
	return _table->contains(tuple);
}
bool SortTable::contains(const DomainElement* el) const {
	return _table->contains(el);
}

// Returns true if non-empty and a range
bool SortTable::isRange() const {
	return _table->isRange();
}
// NOTE: first and last are guaranteed NOT NULL if the table is not empty
const DomainElement* SortTable::first() const {
	Assert(not empty());
	return _table->first();
}
const DomainElement* SortTable::last() const {
	Assert(not empty());
	return _table->last();
}

InternalSortTable* SortTable::internTable() const {
	return _table;
}

bool FuncTable::finite() const {
	return _table->finite(_universe);
}
bool FuncTable::empty() const {
	return _table->empty(_universe);
}
unsigned int FuncTable::arity() const {
	return _universe.arity() - 1;
}
bool FuncTable::approxFinite() const {
	return _table->approxFinite(_universe);
}
bool FuncTable::approxEmpty() const {
	return _table->approxEmpty(_universe);
}
tablesize FuncTable::size() const {
	return _table->size(_universe);
}

// !!! RETURNS NULL iff the given tuple does not map to a domainelement within the range sort
const DomainElement* FuncTable::operator[](const ElementTuple& tuple) const {
	Assert(tuple.size()==arity());
#ifdef DEBUG
	for (auto i = tuple.cbegin(); i < tuple.cend(); ++i) {
		Assert(*i!=NULL); // TODO this should be allowed (e.g. from a deeper function call, but crashes unexpectedly later in the program).
	}
#endif
	auto result = _table->operator[](tuple);
	if (universe().tables().back()->contains(result)) {
		return result;
	} else {
		return NULL;
	}
}

InternalFuncTable* FuncTable::internTable() const {
	return _table;
}

const Universe& FuncTable::universe() const {
	return _universe;
}

SortTable* TableUtils::createSortTable() {
	return new SortTable(new EnumeratedInternalSortTable());
}
SortTable* TableUtils::createSortTable(int start, int end) {
	return new SortTable(new IntRangeInternalSortTable(min(start, end), max(start, end)));
}
PredTable* TableUtils::createPredTable(const Universe& universe) {
	return new PredTable(new EnumeratedInternalPredTable(), universe);
}
PredTable* TableUtils::createFullPredTable(const Universe& universe) {
	return new PredTable(new FullInternalPredTable(), universe);
}

/**********************
 Domain elements
 **********************/

ostream& operator<<(ostream& out, const DomainElement& domelem) {
	return out <<print(domelem);
}

ostream& operator<<(ostream& out, const DomainElementType& domeltype) {
	string DomElTypeStrings[4] = { "int", "double", "string", "compound" };
	return out << DomElTypeStrings[domeltype];
}

DomainElement::DomainElement()
		: _type(DET_INT), _value(1) {
}

/**
 *	Constructor for domain elements that are integers
 */
DomainElement::DomainElement(int value)
		: _type(DET_INT), _value(value) {
}

/**
 *	Constructor for domain elements that are floating point numbers but not integers
 */
DomainElement::DomainElement(double value)
		: _type(DET_DOUBLE), _value(value) {
	Assert(not isInt(value));
	// TODO check rest of code for such errors
}

/**
 *	Constructor for domain elements that are strings but not floating point numbers
 */
DomainElement::DomainElement(const string* value)
		: _type(DET_STRING), _value(value) {
	
}

/**
 *	Constructor for domain elements that are compounds
 */
DomainElement::DomainElement(const Compound* value)
		: _type(DET_COMPOUND), _value(value) {
}

ostream& DomainElement::put(ostream& output) const {
	switch (_type) {
	case DET_INT:
		output << convertToString(_value._int);
		break;
	case DET_DOUBLE:
		output << convertToString(_value._double);
		break;
	case DET_STRING:
		output << '\"'<< *(_value._string)<< '\"';
		break;
	case DET_COMPOUND:
		_value._compound->put(output);
		break;
	}
	return output;
}

std::vector<const DomElemContainer*> DomElemContainer::containers;
void DomElemContainer::deleteAllContainers() {
	for (auto i = containers.cbegin(); i != containers.cend(); ++i) {
		if (*i != NULL) {
			delete (*i);
		}
	}
	containers.clear();
}

ostream& operator<<(ostream& output, const ElementTuple& tuple) {
	output << '(';
	printList(output, tuple, ",", true);
	output << ')';
	return output;
}

bool operator<(const DomainElement& d1, const DomainElement& d2) {
	if (d1.type() == DET_INT && d2.type() == DET_INT) {
		return d1.value()._int < d2.value()._int; // NOTE: Try speedup of most occurring comparison
	}
	switch (d1.type()) {
	case DET_INT:
		switch (d2.type()) {
		case DET_INT:
			return d1.value()._int < d2.value()._int;
		case DET_DOUBLE:
			return double(d1.value()._int) < d2.value()._double;
		case DET_STRING:
		case DET_COMPOUND:
			return true;
		}
		break;
	case DET_DOUBLE:
		switch (d2.type()) {
		case DET_INT:
			return d1.value()._double < double(d2.value()._int);
		case DET_DOUBLE:
			return d1.value()._double < d2.value()._double;
		case DET_STRING:
		case DET_COMPOUND:
			return true;
		}
		break;
	case DET_STRING:
		switch (d2.type()) {
		case DET_INT:
		case DET_DOUBLE:
			return false;
		case DET_STRING:
			return *(d1.value()._string) < *(d2.value()._string);
		case DET_COMPOUND:
			return true;
		}
		break;
	case DET_COMPOUND:
		switch (d2.type()) {
		case DET_INT:
		case DET_DOUBLE:
		case DET_STRING:
			return false;
		case DET_COMPOUND:
			return *(d1.value()._compound) < *(d2.value()._compound);
		}
		break;
	}
	return false;
}

Compound::Compound(Function* function, const ElementTuple& arguments)
		: _function(function), _arguments(arguments) {
	Assert(function != NULL);
}

/**
 *	\brief Destructor for compound domain element values. Does not delete its arguments.
 */
Compound::~Compound() {
}

Function* Compound::function() const {
	return _function;
}

const DomainElement* Compound::arg(unsigned int n) const {
	return _arguments[n];
}

ostream& Compound::put(ostream& output) const {
	output << *_function;
	if (_function->arity() > 0) {
		output << '(' << *_arguments[0];
		for (size_t n = 1; n < _function->arity(); ++n) {
			output << ',' << *_arguments[n];
		}
		output << ')';
	}
	return output;
}

/**
 * \brief Comparison of two compound domain element values
 */
bool operator<(const Compound& c1, const Compound& c2) {
    Compare<std::vector<Sort*> > svc{}; 
	if (c1.function()->name() < c2.function()->name()) {
		return true;
	} else if (c1.function()->name() > c2.function()->name()) {
		return false;
	} else if(svc(c1.function()->sorts(), c2.function()->sorts())){
            return true;
	} else if(svc(c2.function()->sorts(), c1.function()->sorts())){
            return false;
        } else {
		for (unsigned int n = 0; n < c1.function()->arity(); ++n) {
			if (*c1.arg(n) < *c2.arg(n)) {
				return true;
			} else if (*c1.arg(n) > *c2.arg(n)) {
				return false;
			}
		}
	}
	return false;
}

bool operator>(const Compound& c1, const Compound& c2) {
	return c2 < c1;
}

bool operator==(const Compound& c1, const Compound& c2) {
	return &c1 == &c2;
}

bool operator!=(const Compound& c1, const Compound& c2) {
	return &c1 != &c2;
}

bool operator<=(const Compound& c1, const Compound& c2) {
	return (c1 == c2 || c1 < c2);
}

bool operator>=(const Compound& c1, const Compound& c2) {
	return (c1 == c2 || c2 < c1);
}

bool isFinite(const tablesize& tsize) {
	return tsize._type == TST_EXACT || tsize._type == TST_APPROXIMATED;
}


/****************
 Iterators
 ****************/

TableIterator::TableIterator(const TableIterator& tab) {
	_iterator = tab.iterator()->clone();
}

TableIterator& TableIterator::operator=(const TableIterator& tab) {
	if (this != &tab) {
		delete (_iterator);
		_iterator = tab.iterator()->clone();
	}
	return *this;
}

bool TableIterator::isAtEnd() const {
	return not _iterator->hasNext();
}

const ElementTuple& TableIterator::operator*() const {
	return _iterator->operator*();
}

TableIterator::~TableIterator() {
	delete (_iterator);
}

void TableIterator::operator++() {
	_iterator->operator++();
}

SortIterator::SortIterator(const SortIterator& si) {
	_iterator = si.iterator()->clone();
}

SortIterator& SortIterator::operator=(const SortIterator& si) {
	if (this != &si) {
		delete (_iterator);
		_iterator = si.iterator()->clone();
	}
	return *this;
}

bool SortIterator::isAtEnd() const {
	return not _iterator->hasNext();
}

const DomainElement* SortIterator::operator*() const {
	return _iterator->operator*();
}

SortIterator::~SortIterator() {
	delete (_iterator);
}

SortIterator& SortIterator::operator++() {
	CHECKTERMINATION;
	_iterator->operator++();
	return *this;
}

//TODO remove bool h (hasnext)
CartesianInternalTableIterator::CartesianInternalTableIterator(const vector<SortIterator>& vsi, const vector<SortIterator>& low, bool h)
		: _iterators(vsi), _lowest(low), _hasNext(h) {
	if (h) {
		for (auto it = _iterators.cbegin(); it != _iterators.cend(); ++it) {
			if (it->isAtEnd()) {
				_hasNext = false;
				break;
			}
		}
	}
}

CartesianInternalTableIterator* CartesianInternalTableIterator::clone() const {
	return new CartesianInternalTableIterator(_iterators, _lowest, hasNext());
}

bool CartesianInternalTableIterator::hasNext() const {
	return _hasNext;
}

const ElementTuple& CartesianInternalTableIterator::operator*() const {
	ElementTuple tup;
	for (auto it = _iterators.cbegin(); it != _iterators.cend(); ++it)
		tup.push_back(*(*it));
	_deref.push_back(tup);
	return _deref.back();
}

void CartesianInternalTableIterator::operator++() {
	int n = _iterators.size() - 1;
	for (; n >= 0; --n) {
		++(_iterators[n]);
		if (_iterators[n].isAtEnd()) {
			_iterators[n] = _lowest[n];
		} else {
			break;
		}
	}
	if (n < 0)
		_hasNext = false;
}

// TODO bool flags
GeneratorInternalTableIterator::GeneratorInternalTableIterator(InstGenerator* generator, const vector<const DomElemContainer*>& vars, bool reset, bool h)
		: _generator(generator), _vars(vars) {
	if (reset) {
		_generator->begin();
		_hasNext = not _generator->isAtEnd();
	} else {
		_hasNext = h;
	}
}

void GeneratorInternalTableIterator::operator++() {
	_generator->operator++();
	_hasNext = not _generator->isAtEnd();
}

const ElementTuple& GeneratorInternalTableIterator::operator*() const {
	ElementTuple tup;
	for (auto it = _vars.cbegin(); it != _vars.cend(); ++it) {
		tup.push_back((*it)->get());
	}
	_deref.push_back(tup);
	return _deref.back();
}

const ElementTuple& InternalFuncIterator::operator*() const {
	ElementTuple e = *_curr;
	e.push_back(_function->operator[](e));
	_deref.push_back(e);
	return _deref.back();
}

void InternalFuncIterator::operator++() {
	++_curr;
	while (not _curr.isAtEnd() && !(_function->operator[](*_curr)))
		++_curr;
}

InternalFuncIterator::InternalFuncIterator(const InternalFuncTable* f, const Universe& univ)
		: _function(f) {
	vector<SortIterator> vsi1;
	vector<SortIterator> vsi2;
	for (unsigned int n = 0; n < univ.arity() - 1; ++n) {
		vsi1.push_back(univ.tables()[n]->sortBegin());
		vsi2.push_back(univ.tables()[n]->sortBegin());
	}
	_curr = TableIterator(new CartesianInternalTableIterator(vsi1, vsi2, true));
	if (not _curr.isAtEnd() && !_function->operator[](*_curr))
		operator++();
}

const ElementTuple& ProcInternalTableIterator::operator*() const {
	return *_curr;
}

void ProcInternalTableIterator::operator++() {
	++_curr;
	while (not _curr.isAtEnd() && !(_predicate->contains(*_curr, _univ)))
		++_curr;
}

ProcInternalTableIterator::ProcInternalTableIterator(const InternalPredTable* p, const Universe& univ)
		: _univ(univ), _predicate(p) {
	vector<SortIterator> vsi1;
	vector<SortIterator> vsi2;
	for (unsigned int n = 0; n < univ.arity(); ++n) {
		vsi1.push_back(univ.tables()[n]->sortBegin());
		vsi2.push_back(univ.tables()[n]->sortBegin());
	}
	_curr = TableIterator(new CartesianInternalTableIterator(vsi1, vsi2, true));
	if(_curr.isAtEnd()){
		return;
	}
	if (!_predicate->contains(*_curr, _univ))
		operator++();
}

SortInternalTableIterator::~SortInternalTableIterator() {
	delete (_iter);
}

inline bool SortInternalTableIterator::hasNext() const {
	return _iter->hasNext();
}

inline const ElementTuple& SortInternalTableIterator::operator*() const {
	ElementTuple tuple(1, *(*_iter));
	_deref.push_back(tuple);
	return _deref.back();
}

inline void SortInternalTableIterator::operator++() {
	_iter->operator++();
}

SortInternalTableIterator* SortInternalTableIterator::clone() const {
	return new SortInternalTableIterator(_iter->clone());
}

EnumInternalIterator* EnumInternalIterator::clone() const {
	return new EnumInternalIterator(_iter, _end);
}

const ElementTuple& EnumInternalFuncIterator::operator*() const {
	ElementTuple tuple = _iter->first;
	tuple.push_back(_iter->second);
	_deref.push_back(tuple);
	return _deref.back();
}

EnumInternalFuncIterator* EnumInternalFuncIterator::clone() const {
	return new EnumInternalFuncIterator(_iter, _end);
}

bool UnionInternalIterator::contains(const ElementTuple& tuple) const {
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		if ((*it)->contains(tuple, _universe))
			return false;
	}
	return true;
}

void UnionInternalIterator::setcurriterator() {
	for (_curriterator = _iterators.begin(); _curriterator != _iterators.end();) {
		if (not ((*_curriterator).isAtEnd()) && contains(*(*_curriterator))) {
			break;
		}
		++_curriterator;
	}
	if (_curriterator == _iterators.end()) {
		return;
	}
	auto jt = _curriterator;
	++jt;
	for (; jt != _iterators.cend();) {
		if (jt->isAtEnd()) {
			++jt;
			continue;
		}
		if (contains(*(*jt))) {
			Compare<ElementTuple> swto;
			if (swto(*(*jt), *(*_curriterator))) {
				_curriterator = jt;
			} else if (not swto(*(*_curriterator), *(*jt))) {
				if (jt != _curriterator) {
					++(*jt);
				}
				++jt;
				if (_curriterator->isAtEnd()) {
				} else {
				}
			} else {
				++jt;
			}
		} else {
			++(*jt);
		}
	}
}

UnionInternalIterator::UnionInternalIterator(const vector<TableIterator>& its, const vector<InternalPredTable*>& outs, const Universe& univ)
		: _iterators(its), _universe(univ), _outtables(outs) {
	setcurriterator();
}

UnionInternalIterator* UnionInternalIterator::clone() const {
	return new UnionInternalIterator(_iterators, _outtables, _universe);
}

bool UnionInternalIterator::hasNext() const {
	return _curriterator != _iterators.cend();
}

const ElementTuple& UnionInternalIterator::operator*() const {
	Assert(not _curriterator->isAtEnd());
	return *(*_curriterator);
}

void UnionInternalIterator::operator++() {
	++(*_curriterator);
	setcurriterator();
	Assert(_curriterator == _iterators.cend()|| not _curriterator->isAtEnd());
}

InverseInternalIterator::InverseInternalIterator(const vector<SortIterator>& its, InternalPredTable* out, const Universe& univ)
		: _curr(its), _lowest(its), _universe(univ), _outtable(out), _end(false), _currtuple(its.size()) {
	for (size_t n = 0; n < _curr.size(); ++n) {
		if (_curr[n].isAtEnd()) {
			_end = true;
			break;
		}
		_currtuple[n] = *(_curr[n]);
	}
	if (_outtable->size(_universe)._size == _universe.size()._size) {
		_end = true;
	}
	if (not _end) {
		if (_outtable->contains(_currtuple, _universe)) {
			operator++();
		}
	}
}

InverseInternalIterator::InverseInternalIterator(const vector<SortIterator>& curr, const vector<SortIterator>& low, InternalPredTable* out,
		const Universe& univ, bool end)
		: _curr(curr), _lowest(low), _universe(univ), _outtable(out), _end(end), _currtuple(curr.size()) {
	for (size_t n = 0; n < _curr.size(); ++n) {
		if (not _curr[n].isAtEnd()) {
			_currtuple[n] = *(_curr[n]);
		}
	}
	if (_outtable->size(_universe)._size == _universe.size()._size) {
		_end = true;
	}
}

InverseInternalIterator* InverseInternalIterator::clone() const {
	return new InverseInternalIterator(_curr, _lowest, _outtable, _universe, _end);
}

bool InverseInternalIterator::hasNext() const {
	return !_end;
}

const ElementTuple& InverseInternalIterator::operator*() const {
	_deref = _currtuple;
	return _deref;
}

void InverseInternalIterator::operator++() {
	do {
		int pos = _curr.size() - 1;
		for (auto i = _curr.rbegin(); i != _curr.rend(); ++i, --pos) {
			Assert(not i->isAtEnd());
			++(*i);
			if (not i->isAtEnd()) {
				_currtuple[pos] = *(*i);
				break;
			} else {
				_curr[pos] = _lowest[pos];
				Assert(not _curr[pos].isAtEnd());
				_currtuple[pos] = *(_curr[pos]);
			}
		}
		if (pos < 0) {
			_end = true;
		}
	} while ((not _end) && _outtable->contains(_currtuple, _universe));
}

UNAInternalIterator::UNAInternalIterator(const vector<SortIterator>& its, Function* f)
		: cartIt(its,its), _function(f) {
}

UNAInternalIterator::UNAInternalIterator(const vector<SortIterator>& curr, const vector<SortIterator>& low, Function* f, bool end)
		: cartIt(curr,low,end), _function(f) {
}

UNAInternalIterator* UNAInternalIterator::clone() const {
	return new UNAInternalIterator(cartIt.getIterators(), cartIt.getLowest(), _function, hasNext());
}

bool UNAInternalIterator::hasNext() const{
	return cartIt.hasNext();
}

const ElementTuple& UNAInternalIterator::operator*() const {
	auto tmp = *cartIt;
	tmp.push_back(createDomElem(_function, tmp));
	_deref2.push_back(tmp);
	return _deref2.back();
}

void UNAInternalIterator::operator++(){
	++cartIt;
}

EqualInternalIterator::EqualInternalIterator(const SortIterator& iter)
		: _iterator(iter) {
}

bool EqualInternalIterator::hasNext() const {
	return not _iterator.isAtEnd();
}

const ElementTuple& EqualInternalIterator::operator*() const {
	_deref.push_back(ElementTuple(2, *_iterator));
	return _deref.back();
}

void EqualInternalIterator::operator++() {
	++_iterator;
}

EqualInternalIterator* EqualInternalIterator::clone() const {
	return new EqualInternalIterator(_iterator);
}

UnionInternalSortIterator::UnionInternalSortIterator(const vector<SortIterator>& vsi, const vector<SortTable*>& tabs)
		: _iterators(vsi), _outtables(tabs) {
	setcurriterator();
}

UnionInternalSortIterator* UnionInternalSortIterator::clone() const {
	return new UnionInternalSortIterator(_iterators, _outtables);
}

bool UnionInternalSortIterator::contains(const DomainElement* d) const {
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		if ((*it)->contains(d)) {
			return false;
		}
	}
	return true;
}

void UnionInternalSortIterator::setcurriterator() {
	for (_curriterator = _iterators.begin(); _curriterator != _iterators.end();) {
		if (not _curriterator->isAtEnd() && contains(*(*_curriterator))) {
			break;
		}
		++_curriterator;
	}
	if (_curriterator->isAtEnd()) {
		return;
	}

	auto jt = _curriterator;
	++jt;
	for (; jt != _iterators.cend();) {
		if (jt->isAtEnd()) {
			++jt;
			break;
		}
		if (contains(*(*jt))) {
			if (*(*(*jt)) < *(*(*_curriterator)))
				_curriterator = jt;
			else if (*(*_curriterator) == *(*jt)) {
				++(*jt);
				++jt;
			}
		} else
			++(*jt);
	}
}

bool UnionInternalSortIterator::hasNext() const {
	return _curriterator != _iterators.cend();
}

const DomainElement* UnionInternalSortIterator::operator*() const {
	Assert(not _curriterator->isAtEnd());
	return *(*_curriterator);
}

void UnionInternalSortIterator::operator++() {
	++(*_curriterator);
	setcurriterator();
}

void StringInternalSortIterator::operator++() {
	int n = _iter.size() - 1;
	for (; n >= 0; --n) {
		++_iter[n];
		if (_iter[n] != getMinElem<char>()) {
			break;
		}
	}
	if (n < 0) {
		_iter = string(_iter.size() + 1, getMinElem<char>());
	}
}

const DomainElement* StringInternalSortIterator::operator*() const {
	auto r = GlobalData::getGlobalDomElemFactory()->create(_iter);
	return r;
}

const DomainElement* CharInternalSortIterator::operator*() const {
	if ('0' <= _iter && _iter <= '9') {
		int i = _iter - '0';
		return createDomElem(i);
	} else {
		return createDomElem(string(1, _iter));
	}
}

void CharInternalSortIterator::operator++() {
	if (_iter == getMaxElem<char>()) {
		_end = true;
	} else {
		++_iter;
	}
}

bool ConstructedInternalSortIterator::hasNext() const {
	return _constructors_it != _constructors.cend();
}

const DomainElement* ConstructedInternalSortIterator::operator*() const {
	return (*_table_it).back();
}

void ConstructedInternalSortIterator::operator++() {
	++_table_it;
	skipToNextElement();
}

void ConstructedInternalSortIterator::skipToNextElement() {
	if (_constructors_it == _constructors.cend()) {
		return;
	}
	while (_table_it.isAtEnd()) {
		_constr_index++;
		++_constructors_it;
		if (_constructors_it == _constructors.cend()) {
			return;
		}
		_table_it = _struct->inter(*_constructors_it)->funcTable()->begin();
	}
}

void ConstructedInternalSortIterator::initialize(const std::vector<Function*>& constructors){
	// make sure finite constructors are in front
	_constructors=std::vector<Function*>();
	std::vector<Function*> rem;
	for(auto c:constructors){
		Assert(_struct->inter(c)->funcTable()!=NULL); // It is a constructor function, which has to be interpreted two-valued.
		auto table = _struct->inter(c)->funcTable();
		if(table->empty()){ // Correctness depends on the fact that "empty" and "approxFinite" of constructed functions do NOT work by requesting an iterator
							//		(which would lead to an infinite loop)
			continue;
		}
		if(table->approxFinite()){
			_constructors.push_back(c);
		}else{
			rem.push_back(c);
		}
	}
	if(_constructors.empty()){
		_constr_index=-1;
		_constructors_it = _constructors.cend();
		return;
	}
	insertAtEnd(_constructors, rem);
	_constr_index = 0;
	_constructors_it=_constructors.cbegin();
	if(_constructors_it!=_constructors.cend()){
		_table_it= _struct->inter(*_constructors_it)->funcTable()->begin();
	}
	skipToNextElement();
}

ConstructedInternalSortIterator::ConstructedInternalSortIterator(): _constr_index(-1), _struct(NULL){

}
ConstructedInternalSortIterator::ConstructedInternalSortIterator(const std::vector<Function*>& constructors, const Structure* struc)
		: 	_constructors(std::vector<Function*>()),
			_struct(struc) {
	initialize(constructors);
}

ConstructedInternalSortIterator::ConstructedInternalSortIterator(const std::vector<Function*>& constructors, const Structure* struc, const DomainElement* domel)
		: 	_constructors(std::vector<Function*>()),
			_struct(struc) {
	initialize(constructors);
	if(domel->type()!=DET_COMPOUND){
		_constr_index = -1;
		_constructors_it=_constructors.cend();
	}else{
		Function* func = domel->value()._compound->function();
		for(; _constructors_it!=_constructors.cend(); ++_constructors_it){
			if(*_constructors_it==func){
				_table_it = _struct->inter(*_constructors_it)->funcTable()->begin();
				while(not _table_it.isAtEnd() && not((*_table_it).back() == domel)){
					++_table_it;
				}
				if(_table_it.isAtEnd()){ // element not present
					_constr_index = -1;
					_constructors_it=_constructors.cend();
				}
				break;
			}
		}
	}
}

ConstructedInternalSortIterator* ConstructedInternalSortIterator::clone() const {
	auto newit = new ConstructedInternalSortIterator();
	newit->_constructors = _constructors;
	newit->_constr_index = _constr_index;
	if (_constr_index != -1) {
		newit->_constructors_it = newit->_constructors.cbegin() + _constr_index;
		newit->_table_it = TableIterator(_table_it);
	} else {
		newit->_constructors_it = newit->_constructors.cend();
	}
	newit->_struct = _struct;
	return newit;
}

/*************************
 Internal predtable
 *************************/

bool FirstNElementsEqual::operator()(const ElementTuple& t1, const ElementTuple& t2) const {
	for (size_t n = 0; n < _arity; ++n) {
		if (t1[n] != t2[n]) {
			return false;
		}
	}
	return true;
}

bool StrictWeakNTupleOrdering::operator()(const ElementTuple& t1, const ElementTuple& t2) const {
	for (size_t n = 0; n < _arity; ++n) {
		if (*(t1[n]) < *(t2[n])) {
			return true;
		} else if (*(t1[n]) > *(t2[n])) {
			return false;
		}
	}
	return false;
}

FuncInternalPredTable::FuncInternalPredTable(FuncTable* table, bool linked)
		: InternalPredTable(), _table(table), _linked(linked) {
}

FuncInternalPredTable::~FuncInternalPredTable() {
	if (not _linked) {
		delete (_table);
	}
}

inline bool FuncInternalPredTable::finite(const Universe&) const {
	return _table->finite();
}

inline bool FuncInternalPredTable::empty(const Universe&) const {
	return _table->empty();
}

inline bool FuncInternalPredTable::approxFinite(const Universe&) const {
	return _table->approxFinite();
}

inline bool FuncInternalPredTable::approxEmpty(const Universe&) const {
	return _table->approxEmpty();
}

tablesize FuncInternalPredTable::size(const Universe&) const {
	return _table->size();
}

bool FuncInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if (_table->contains(tuple)) {
		return univ.contains(tuple);
	} else
		return false;
}

InternalPredTable* FuncInternalPredTable::add(const ElementTuple& tuple) {
	ElementTuple in = tuple;
	in.pop_back();
	const DomainElement* out = _table->operator[](in);
	if (out == 0) {
		if (_nrRefs > 1 && !_linked) {
			FuncTable* nft = new FuncTable(_table->internTable(), _table->universe());
			nft->add(tuple);
			return new FuncInternalPredTable(nft, false);
		} else {
			_table->add(tuple);
			return this;
		}
	} else if (_table->approxFinite()) {
		EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
		for (TableIterator it = _table->begin(); not it.isAtEnd(); ++it) {
			eipt->add(*it);
		}
		eipt->add(tuple);
		return eipt;
	} else {
		UnionInternalPredTable* uipt = new UnionInternalPredTable();
		FuncTable* nft = new FuncTable(_table->internTable(), _table->universe());
		uipt->addInTable(new FuncInternalPredTable(nft, false));
		uipt->add(tuple);
		return uipt;
	}
}

InternalPredTable* FuncInternalPredTable::remove(const ElementTuple& tuple) {
	if (_nrRefs > 1 && !_linked) {
		FuncTable* nft = new FuncTable(_table->internTable(), _table->universe());
		nft->remove(tuple);
		return new FuncInternalPredTable(nft, false);
	} else {
		_table->remove(tuple);
		return this;
	}
}

InternalTableIterator* FuncInternalPredTable::begin(const Universe&) const {
	return _table->internTable()->begin(_table->universe());
}

FullInternalPredTable::~FullInternalPredTable() {
}

bool FullInternalPredTable::finite(const Universe& univ) const {
	return univ.finite();
}

bool FullInternalPredTable::empty(const Universe& univ) const {
	return univ.empty();
}

bool FullInternalPredTable::approxFinite(const Universe& univ) const {
	return univ.approxFinite();
}

bool FullInternalPredTable::approxEmpty(const Universe& univ) const {
	return univ.approxEmpty();
}

bool FullInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	return univ.contains(tuple);
}

InternalPredTable* FullInternalPredTable::add(const ElementTuple& tuple) {
	UnionInternalPredTable* uipt = new UnionInternalPredTable();
	uipt->addInTable(this);
	uipt->add(tuple);
	return uipt;
}

InternalPredTable* FullInternalPredTable::remove(const ElementTuple& tuple) {
	UnionInternalPredTable* uipt = new UnionInternalPredTable();
	uipt->addInTable(this);
	uipt->remove(tuple);
	return uipt;
}

InternalTableIterator* FullInternalPredTable::begin(const Universe& univ) const {
	vector<SortIterator> vsi;
	for (auto it = univ.tables().cbegin(); it != univ.tables().cend(); ++it) {
		vsi.push_back((*it)->sortBegin());
	}
	return new CartesianInternalTableIterator(vsi, vsi);
}

UnionInternalPredTable::UnionInternalPredTable()
		: InternalPredTable() {
	_intables.push_back(new EnumeratedInternalPredTable());
	_intables[0]->incrementRef();
	_outtables.push_back(new EnumeratedInternalPredTable());
	_outtables[0]->incrementRef();
}

UnionInternalPredTable::UnionInternalPredTable(const vector<InternalPredTable*>& intabs, const vector<InternalPredTable*>& outtabs)
		: InternalPredTable(), _intables(intabs), _outtables(outtabs) {
	for (auto it = intabs.cbegin(); it != intabs.cend(); ++it) {
		(*it)->incrementRef();
	}
	for (auto it = outtabs.cbegin(); it != outtabs.cend(); ++it) {
		(*it)->incrementRef();
	}
}

tablesize UnionInternalPredTable::size(const Universe& univ) const {
	unsigned int result = 0;
	TableSizeType type = TST_APPROXIMATED;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		tablesize tp = (*it)->size(univ);
		switch (tp._type) {
		case TST_APPROXIMATED:
		case TST_EXACT:
			result += tp._size;
			break;
		case TST_INFINITE:
			type = tp._type;
			break;
		}
	}
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		tablesize tp = (*it)->size(univ);
		switch (tp._type) {
		case TST_APPROXIMATED:
		case TST_EXACT:
			if (result > tp._size)
				result -= tp._size;
			else
				result = 0;
			break;
		case TST_INFINITE:
			type = TST_INFINITE; // Is possibly infinite
			break;
		}
	}
	tablesize univsize = univ.size();
	if (univsize._type == TST_APPROXIMATED || univsize._type == TST_EXACT) {
		result = univsize._size < result ? univsize._size : result;
	}
	return tablesize(type, result);
}

/**
 *	Destructor for union predicate tables
 */
UnionInternalPredTable::~UnionInternalPredTable() {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		(*it)->decrementRef();
	}
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		(*it)->decrementRef();
	}
}

bool UnionInternalPredTable::finite(const Universe& univ) const {
	if (approxFinite(univ)) {
		return true;
	} else {
		throw notyetimplemented("Exact finiteness test on union predicate tables");
		return approxFinite(univ);
	}
}

bool UnionInternalPredTable::empty(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	} else {
		throw notyetimplemented("Exact emptyness test on union predicate tables");
		return approxEmpty(univ);
	}
}

bool UnionInternalPredTable::approxFinite(const Universe& univ) const {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if (not (*it)->approxFinite(univ))
			return false;
	}
	return true;
}

bool UnionInternalPredTable::approxEmpty(const Universe& univ) const {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if (not (*it)->approxEmpty(univ))
			return false;
	}
	return true;
}

/**
 * \brief	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool UnionInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	bool in = false;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if ((*it)->contains(tuple, univ)) {
			in = true;
			break;
		}
	}
	if (not in) {
		return false;
	}

	bool out = false;
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		if ((*it)->contains(tuple, univ)) {
			out = true;
			break;
		}
	}
	return !out;
}

/**
 * \brief Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* UnionInternalPredTable::add(const ElementTuple& tuple) {
	if (_nrRefs > 1) {
		UnionInternalPredTable* newtable = new UnionInternalPredTable(_intables, _outtables);
		InternalPredTable* temp = newtable->add(tuple);
		Assert(temp == newtable);
		return temp;
	} else {
		InternalPredTable* temp = _intables[0]->add(tuple);
		if (temp != _intables[0]) {
			_intables[0]->decrementRef();
			temp->incrementRef();
			_intables[0] = temp;
		}
		for (size_t n = 0; n < _outtables.size(); ++n) {
			InternalPredTable* temp2 = _outtables[n]->remove(tuple);
			if (temp2 != _outtables[n]) {
				_outtables[n]->decrementRef();
				temp2->incrementRef();
				_outtables[n] = temp2;
			}
		}
		return this;
	}
}

/**
 * \brief Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* UnionInternalPredTable::remove(const ElementTuple& tuple) {
	Assert(!_outtables.empty());
	if (_nrRefs > 1) {
		UnionInternalPredTable* newtable = new UnionInternalPredTable(_intables, _outtables);
		InternalPredTable* temp = newtable->remove(tuple);
		Assert(temp == newtable);
		return temp;
	} else {
		InternalPredTable* temp = _outtables[0]->add(tuple);
		if (temp != _outtables[0]) {
			_outtables[0]->decrementRef();
			temp->incrementRef();
			_outtables[0] = temp;
		}
		return this;
	}
}

InternalTableIterator* UnionInternalPredTable::begin(const Universe& univ) const {
	vector<TableIterator> vti;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		vti.push_back(TableIterator((*it)->begin(univ)));
	}
	return new UnionInternalIterator(vti, _outtables, univ);
}

BDDInternalPredTable::BDDInternalPredTable(const FOBDD* bdd, std::shared_ptr<FOBDDManager> manager, const vector<Variable*>& vars, const Structure* str)
		: _manager(manager), _bdd(bdd), _vars(vars), _structure(str) {
#ifdef DEBUG
	for(auto fobddvar: variables(bdd,manager)){
		Assert(::contains(vars, fobddvar->variable()));
	}
#endif
}

BDDInternalPredTable::~BDDInternalPredTable(){
}

bool BDDInternalPredTable::finite(const Universe& univ) const {
	if (univ.finite()) {
		return true;
	} else {
		return approxFinite(univ);
	}
}

bool BDDInternalPredTable::empty(const Universe& univ) const {
	if (univ.empty()) {
		return true;
	} else {
		TableIterator ti(begin(univ));
		return ti.isAtEnd();
	}
}

bool BDDInternalPredTable::approxFinite(const Universe& univ) const {
	if (univ.approxFinite()) {
		return true;
	} else {
		fobddindexset indices;
		varset fovars;
		fovars.insert(_vars.cbegin(), _vars.cend());
		auto bddvars = _manager->getVariables(fovars);
		double estimate = BddStatistics::estimateNrAnswers(_bdd, bddvars, indices, _structure, _manager);
		return estimate < getMaxElem<double>();
	}
}

bool BDDInternalPredTable::approxEmpty(const Universe& univ) const {
	if (univ.approxEmpty()) {
		return true;
	} else if (_manager->isFalsebdd(_bdd)) {
		return true;
	} else {
		return false;
	}
}

// Returns NULL if not substitutable
const FOBDD* substituteSuchThatBddHasVars(const BDDInternalPredTable* bipt, const std::vector<Variable*>& vars, shared_ptr<FOBDDManager> manager){
	auto othervars = bipt->vars();
	if(othervars.size() != vars.size()){
		return NULL;
	}

	//Set the bdds to the same variables
	auto othervar = othervars.cbegin();
	map<const FOBDDVariable*, const FOBDDVariable*> otherToThisVars;
	for(auto var: vars){
		auto varone = manager->getVariable(*othervar);
		auto vartwo = manager->getVariable(var);
		otherToThisVars[varone] = vartwo;
		othervar++;
	}
	auto otherbdd = manager->getBDD(bipt->bdd(), bipt->manager());
	otherbdd = manager->substitute(otherbdd,otherToThisVars);
	return otherbdd;
}

bool BDDInternalPredTable::approxEqual(const InternalPredTable* ipt, const Universe& u) const{
	if(ipt == this){
		return true;
	}
	if (_manager->isFalsebdd(_bdd)) {
		return ipt->approxEmpty(u);
	}
	auto bipt = dynamic_cast<const BDDInternalPredTable* >(ipt);
	if(bipt == NULL){
		return InternalPredTable::approxEqual(ipt,u);
	}

	auto otherbdd = substituteSuchThatBddHasVars(bipt,vars(), _manager);
	if (otherbdd!=NULL && _bdd == otherbdd) {
		return true;
	}
	return false;
}

bool BDDInternalPredTable::approxInverse(const InternalPredTable* ipt, const Universe& u) const{
	if(_manager->isTruebdd(_bdd)){
		return ipt->approxEmpty(u);
	}
	auto bipt = dynamic_cast<const BDDInternalPredTable*>(ipt);
	if (bipt == NULL) {
		return InternalPredTable::approxInverse(ipt,u);
	}

	auto otherbdd = substituteSuchThatBddHasVars(bipt, vars(), _manager);
	if(otherbdd==NULL){
		return false;
	}
	if (_bdd == _manager->negation(otherbdd)) {
		return true;
	}

	//Two tables are inverse if their intersection is empty (conjunction is false)
	//And their union is everything (disjunction is true)
	auto conj = _manager->conjunction(_bdd, otherbdd);
	auto disj = _manager->disjunction(_bdd, otherbdd);
	if(_manager->isFalsebdd(conj) && _manager->isTruebdd(disj)){
		return true;
	}
	return false;
}



tablesize BDDInternalPredTable::size(const Universe&) const {
	fobddindexset indices;
	varset fovars;
	fovars.insert(_vars.cbegin(), _vars.cend());
	auto bddvars = _manager->getVariables(fovars);
	double estimate = BddStatistics::estimateNrAnswers(_bdd, bddvars, indices, _structure, _manager);
	if (estimate < getMaxElem<double>()) {
		unsigned int es = estimate;
		return tablesize(TST_APPROXIMATED, es);
	} else
		return tablesize(TST_INFINITE, 0);
}

std::vector<const DomElemContainer*> createVarSubstitutionFrom(const ElementTuple& tuple) {
	vector<const DomElemContainer*> vars;
	for (unsigned int n = 0; n < tuple.size(); ++n) {
		const DomElemContainer* v = new const DomElemContainer();
		*v = tuple[n];
		vars.push_back(v);
	}
	return vars;
}

// TODO univ?
bool BDDInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	BDDInternalPredTable* temporary = new BDDInternalPredTable(_bdd, _manager, _vars, _structure);
	PredTable temptable(temporary, univ);
	InstChecker* checker = GeneratorFactory::create(&temptable, vector<Pattern>(univ.tables().size(), Pattern::INPUT), createVarSubstitutionFrom(tuple), univ);
	bool result = checker->check();
	delete (checker);
	return result;
}

InternalPredTable* BDDInternalPredTable::add(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->add(tuple);
	if (temp != upt) {
		delete (upt);
	}
	return temp;
}

InternalPredTable* BDDInternalPredTable::remove(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->remove(tuple);
	if (temp != upt) {
		delete (upt);
	}
	return temp;
}

InternalTableIterator* BDDInternalPredTable::begin(const Universe& univ) const {
	vector<const DomElemContainer*> doms;
	for (auto it = _vars.cbegin(); it != _vars.cend(); ++it) {
		doms.push_back(new const DomElemContainer());
	}
	auto temporary = new BDDInternalPredTable(_bdd, _manager, _vars, _structure);
	PredTable temptable(temporary, univ);
	InstGenerator* generator = GeneratorFactory::create(&temptable, vector<Pattern>(univ.tables().size(), Pattern::OUTPUT), doms, univ);
	return new GeneratorInternalTableIterator(generator, doms, true);
}

/**
 * \brief	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EnumeratedInternalPredTable::contains(const ElementTuple& tuple, const Universe&) const {
	if (_table.empty()) {
		return false;
	} else {
		return _table.find(tuple) != _table.cend();
	}
}

/**
 * Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
EnumeratedInternalPredTable* EnumeratedInternalPredTable::add(const ElementTuple& tuple) {
	if (_nrRefs <= 1) {
		_table.insert(tuple);
		return this;
	} else {
		SortedElementTable newtable = _table;
		newtable.insert(tuple);
		return new EnumeratedInternalPredTable(newtable);
	}
}

/**
 * Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
EnumeratedInternalPredTable* EnumeratedInternalPredTable::remove(const ElementTuple& tuple) {
	SortedElementTable::iterator it = _table.find(tuple);
	if (it != _table.cend()) {
		if (_nrRefs == 1) {
			_table.erase(it);
			return this;
		} else {
			SortedElementTable newtable = _table;
			SortedElementTable::iterator nt = newtable.find(tuple);
			newtable.erase(nt);
			return new EnumeratedInternalPredTable(newtable);
		}
	} else {
		return this;
	}
}

/**
 * \brief Returns an iterator on the first tuple of the table
 */
InternalTableIterator* EnumeratedInternalPredTable::begin(const Universe&) const {
	return new EnumInternalIterator(_table.cbegin(), _table.cend());
}

ComparisonInternalPredTable::ComparisonInternalPredTable() {
}

ComparisonInternalPredTable::~ComparisonInternalPredTable() {
}

InternalPredTable* ComparisonInternalPredTable::add(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->add(tuple);
	if (temp != upt) {
		delete (upt);
	}
	return temp;
}

InternalPredTable* ComparisonInternalPredTable::remove(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->remove(tuple);
	if (temp != upt) {
		delete (upt);
	}
	return temp;
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EqualInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	Assert(tuple.size() == 2);
	return (tuple[0] == tuple[1] && univ.contains(tuple));
}

/**
 * \brief	Returns true iff the table is finite
 */
bool EqualInternalPredTable::finite(const Universe& univ) const {
	if (approxFinite(univ)) {
		return true;
	} else if (univ.finite()) {
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Returns true iff the table is empty
 */
bool EqualInternalPredTable::empty(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	} else if (univ.empty()) {
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool EqualInternalPredTable::approxFinite(const Universe& univ) const {
	return univ.approxFinite();
}

/**
 * \brief Returns false if the table is not empty. May return true if the table is empty.
 */
inline bool EqualInternalPredTable::approxEmpty(const Universe& univ) const {
	return univ.approxEmpty();
}

tablesize EqualInternalPredTable::size(const Universe& univ) const {
	return univ.tables()[0]->size();
}

InternalTableIterator* EqualInternalPredTable::begin(const Universe& univ) const {
	return new EqualInternalIterator(univ.tables()[0]->sortBegin());
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrLessInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	Assert(tuple.size() == 2);
	return (*(tuple[0]) < *(tuple[1]) && univ.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrLessInternalPredTable::finite(const Universe& univ) const {
	if (approxFinite(univ)) {
		return true;
	} else {
		return univ.finite();
	}
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrLessInternalPredTable::empty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortBegin();
	if (not isi.isAtEnd()) {
		++isi;
		if (not isi.isAtEnd()) {
			return false;
		}
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxFinite(const Universe& univ) const {
	return (univ.approxFinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxEmpty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortBegin();
	if (not isi.isAtEnd()) {
		++isi;
		if (not isi.isAtEnd()) {
			return false;
		}
	}
	return true;
}

tablesize StrLessInternalPredTable::size(const Universe& univ) const {
	tablesize ts = univ.tables()[0]->size();
	if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
		if (ts._size == 1) {
			return tablesize(ts._type, 0);
		} else {
			size_t n = ts._size * (ts._size - 1) / 2;
			return tablesize(ts._type, n);
		}
	} else {
		return ts;
	}
}

InternalTableIterator* StrLessInternalPredTable::begin(const Universe& univ) const {
	vector<const DomElemContainer*> vars { new DomElemContainer(), new DomElemContainer() };
	return new GeneratorInternalTableIterator(
			new ComparisonGenerator(univ.tables()[0], univ.tables()[0], new DomElemContainer(), new DomElemContainer(), Input::NONE, CompType::LT), vars);
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrGreaterInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	Assert(tuple.size() == 2);
	return (*(tuple[0]) > *(tuple[1]) && univ.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrGreaterInternalPredTable::finite(const Universe& univ) const {
	if (approxFinite(univ)) {
		return true;
	} else {
		return univ.finite();
	}
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrGreaterInternalPredTable::empty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortBegin();
	if (not isi.isAtEnd()) {
		++isi;
		if (not isi.isAtEnd()) {
			return false;
		}
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxFinite(const Universe& univ) const {
	return (univ.approxFinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxEmpty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortBegin();
	if (not isi.isAtEnd()) {
		++isi;
		if (not isi.isAtEnd()) {
			return false;
		}
	}
	return true;
}

tablesize StrGreaterInternalPredTable::size(const Universe& univ) const {
	tablesize ts = univ.tables()[0]->size();
	if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
		if (ts._size == 1) {
			return tablesize(ts._type, 0);
		} else {
			size_t n = ts._size * (ts._size - 1) / 2;
			return tablesize(ts._type, n);
		}
	} else {
		return ts;
	}
}

InternalTableIterator* StrGreaterInternalPredTable::begin(const Universe& univ) const {
	vector<const DomElemContainer*> vars { new DomElemContainer(), new DomElemContainer() };
	return new GeneratorInternalTableIterator(
			new ComparisonGenerator(univ.tables()[0], univ.tables()[0], new DomElemContainer(), new DomElemContainer(), Input::NONE, CompType::GT), vars);

}

/*************************
 Internal sorttable
 *************************/

InternalTableIterator* InternalSortTable::begin() const {
	return new SortInternalTableIterator(sortBegin());
}

bool EnumeratedInternalSortTable::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	return _table.find(d) != _table.cend();
}

bool EnumeratedInternalSortTable::isRange() const {
	if (_table.empty() || nbNotIntElements > 0) {
		return false;
	}
	auto f = first();
	auto l = last();
	Assert(f->type() == DET_INT && l->type() == DET_INT);
	// (and in fact all in between)
	return l->value()._int - f->value()._int == (int) _table.size() - 1;
}

void EnumeratedInternalSortTable::addNonRange(const DomainElement* elem, int start, int end) {
	if (elem->type() != DomainElementType::DET_INT) {
		nbNotIntElements++;
	}
	_table.insert(elem);
	for (auto i = start; i <= end; ++i) {
		_table.insert(createDomElem(i));
	}
}
void EnumeratedInternalSortTable::addNonRange(int start1, int end1, int start2, int end2) {
	for (auto i = start1; i <= end1; ++i) {
		_table.insert(createDomElem(i));
	}
	for (auto i = start2; i <= end2; ++i) {
		_table.insert(createDomElem(i));
	}
}

InternalSortIterator* EnumeratedInternalSortTable::sortBegin() const {
	return new EnumInternalSortIterator(_table.cbegin(), _table.cend());
}

InternalSortIterator* EnumeratedInternalSortTable::sortIterator(const DomainElement* d) const {
	return new EnumInternalSortIterator(_table.find(d), _table.cend());
}

InternalSortTable* EnumeratedInternalSortTable::add(int i1, int i2) {
	if (_table.empty()) {
		return new IntRangeInternalSortTable(i1, i2);
	}
	if (isRange()
			&& ((i1 >= first()->value()._int - 1 && i1 <= last()->value()._int + 1) || (i2 >= first()->value()._int - 1 && i2 <= last()->value()._int + 1))) { // TODO overflow checks
		auto intst = new IntRangeInternalSortTable(first()->value()._int, last()->value()._int);
		return intst->add(i1, i2);
	}
	for (auto i = min(i1, i2); i <= max(i1, i2); ++i) {
		_table.insert(createDomElem(i));
	}
	return this;
}

void EnumeratedInternalSortTable::countNBNotIntElements(){
	nbNotIntElements = 0;
	for(auto el: _table){
		if(el->type() != DET_INT){
			nbNotIntElements++;
		}
	}
}

InternalSortTable* EnumeratedInternalSortTable::add(const DomainElement* d) {
	if (contains(d)) {
		return this;
	}
	if (d->type() == DomainElementType::DET_INT) {
		if (_table.empty()) {
			return new IntRangeInternalSortTable(d->value()._int, d->value()._int);
		}
		if (isRange() && d->value()._int >= first()->value()._int - 1 && d->value()._int <= last()->value()._int + 1) { // TODO overflow checks
			auto intst = new IntRangeInternalSortTable(first()->value()._int, last()->value()._int);
			return intst->add(d);
		}
	}

	if (_nrRefs > 1) {
		auto ist = new EnumeratedInternalSortTable(_table);
		ist->add(d);
		return ist;
	} else {
		if (d->type() != DomainElementType::DET_INT) {
			nbNotIntElements++;
		}
		_table.insert(d);
		return this;
	}
}

InternalSortTable* EnumeratedInternalSortTable::remove(const DomainElement* d) {
	if (not contains(d)) {
		return this;
	}
	if (_nrRefs > 1) {
		auto ist = new EnumeratedInternalSortTable(_table);
		ist->remove(d);
		return ist;
	} else {
		if (d->type() != DomainElementType::DET_INT) {
			nbNotIntElements--;
		}
		_table.erase(d);
		return this;
	}
}

const DomainElement* EnumeratedInternalSortTable::first() const {
	if (_table.empty()) {
		return NULL;
	}
	return *(_table.cbegin());
}

const DomainElement* EnumeratedInternalSortTable::last() const {
	if (_table.empty()) {
		return NULL;
	}
	return *(_table.rbegin());
}

bool recursivelyConstructed(Sort* sort, const std::set<Sort*>& seensorts) {
	if(not sort->isConstructed()){
		return false;
	}
	if(contains(seensorts, sort)){
		return true;
	}
	for (auto f : sort->getConstructors()) {
		auto seen2 = seensorts;
		seen2.insert(f->outsort());
		for(auto s: f->insorts()){
			if(s==f->outsort()){
				return true;
			}else if(recursivelyConstructed(s, seen2)){
				return true;
			}
		}
	}
	return false;
}

bool ConstructedInternalSortTable::isRecursive() const {
	for (auto f : _constructors) {
		for(auto s: f->insorts()){
			if(recursivelyConstructed(s, {f->outsort()})){
				return true;
			}
		}
	}
	return false;
}

bool ConstructedInternalSortTable::finite() const {
	if(isRecursive()){
		return false;
	}
	for(auto f:_constructors){
		if(not getTable(f)->finite()){
			return false;
		}
	}
	return true;
}
bool ConstructedInternalSortTable::empty() const {
	if(isRecursive()){
		for(auto f:_constructors){
			auto allnonrec = true;
			for(uint i=0; i<f->arity(); ++i){
				if(f->insorts()[i]==f->outsort()){
					allnonrec = false;
					break;
				}
			}
			if(allnonrec){
				return false;
			}
		}
		return true;
	}
	for(auto f:_constructors){
		if(not getTable(f)->empty()){
			return false;
		}
	}
	return true;
}
bool ConstructedInternalSortTable::approxFinite() const {
	if(isRecursive()){
		return false;
	}
	for(auto f:_constructors){
		if(not getTable(f)->approxFinite()){
			return false;
		}
	}
	return true;
}
bool ConstructedInternalSortTable::approxEmpty() const {
	if(isRecursive()){
		for(auto f:_constructors){
			auto allnonrec = true;
			for(uint i=0; i<f->arity(); ++i){
				if(f->insorts()[i]==f->outsort()){
					allnonrec = false;
					break;
				}
			}
			if(allnonrec){
				return false;
			}
		}
		return true;
	}
	for(auto f:_constructors){
		if(not getTable(f)->approxEmpty()){
			return false;
		}
	}
	return true;
}
tablesize ConstructedInternalSortTable::size() const{
	if(isRecursive()){
		return tablesize(TST_INFINITE, 0);
	}
	auto sum = tablesize(TST_EXACT, 0);
	for (auto f : _constructors) {
		sum = sum+getTable(f)->size();
	}
	return sum;
}

const DomainElement* ConstructedInternalSortTable::first() const {
	return *(*sortBegin());
}

const DomainElement* ConstructedInternalSortTable::last() const {
	auto tmp = sortBegin();
	auto result = **tmp;
	while(tmp->hasNext()){
		++(*tmp);
		result = **tmp;
	}
	return result;
}

bool ConstructedInternalSortTable::contains(const DomainElement* d) const{
	if (d == NULL) {
		return false;
	}
	if(d->type()!=DET_COMPOUND){
		return false;
	}
	for(auto f: _constructors){
		if(f==d->value()._compound->function()){
			auto tuple = d->value()._compound->args();
			tuple.push_back(d);
			return getTable(f)->internTable()->contains(tuple); // NOTE: HACK! Universe check is on the table itself, but which would be checking contains on the constructed type too, so infinite loop
		}
	}
	return false;
}

InternalSortIterator* ConstructedInternalSortTable::sortBegin() const {
	return new ConstructedInternalSortIterator(_constructors, _struc);
}

InternalSortIterator* ConstructedInternalSortTable::sortIterator(const DomainElement* d) const {
	return new ConstructedInternalSortIterator(_constructors, _struc, d);
}

InternalSortTable* IntRangeInternalSortTable::add(const DomainElement* d) {
	if (contains(d)) {
		return this;
	}
	if (d->type() == DET_INT) {
		if (d->value()._int == _first - 1) {
			if (_nrRefs < 2) {
				_first = d->value()._int;
				return this;
			}
		} else if (d->value()._int == _last + 1) {
			if (_nrRefs < 2) {
				_last = d->value()._int;
				return this;
			}
		}
	}
	auto eist = new EnumeratedInternalSortTable();
	eist->addNonRange(d, _first, _last);
	return eist;
}

InternalSortTable* IntRangeInternalSortTable::remove(const DomainElement* d) {
	if (not contains(d) || d->type()!=DomainElementType::DET_INT) {
		return this;
	}

	if(_nrRefs>1){
		return (new IntRangeInternalSortTable(_first, _last))->remove(d);
	}
	auto value = d->value()._int;
	if (value == _first) {
		_first = _first + 1;
		return this;
	} else if (value == _last) {
		_last = _last - 1;
		return this;
	}
	// TODO create new <multi range> table and replace code here with that!
	auto eist = new EnumeratedInternalSortTable();
	InternalSortTable* ist = eist;
	for (int n = _first; n <= _last; ++n) {
		if(value==n){
			continue;
		}
		ist = ist->add(createDomElem(n));
	}
	if (ist != eist) {
		delete (eist);
	}
	return ist;
}

InternalSortTable* IntRangeInternalSortTable::add(int i1, int i2) {
	if (i1 <= _last + 1 && i2 >= _first - 1) {
		if (_nrRefs > 1) {
			int f = i1 < _first ? i1 : _first;
			int l = i2 > _last ? i2 : _last;
			return new IntRangeInternalSortTable(f, l);
		} else {
			_first = i1 < _first ? i1 : _first;
			_last = i2 > _last ? i2 : _last;
			return this;
		}
	} else {
		auto eist = new EnumeratedInternalSortTable();
		eist->addNonRange(i1, i2, _first, _last);
		return eist;
	}
}

const DomainElement* IntRangeInternalSortTable::first() const {
	return createDomElem(_first);
}

const DomainElement* IntRangeInternalSortTable::last() const {
	return createDomElem(_last);
}

inline bool IntRangeInternalSortTable::contains(const DomainElement* d) const {
	if (d == NULL || d->type() != DomainElementType::DET_INT) {
		return false;
	}
	const auto& val = d->value()._int;
	return d->type() == DET_INT && _first <= val && val <= _last;
}

InternalSortIterator* IntRangeInternalSortTable::sortBegin() const {
	return new RangeInternalSortIterator(_first, _last);
}

InternalSortIterator* IntRangeInternalSortTable::sortIterator(const DomainElement* d) const {
	Assert(d->type()==DomainElementType::DET_INT);
	return new RangeInternalSortIterator(d->value()._int, _last);
}

UnionInternalSortTable::UnionInternalSortTable() {
	_intables.push_back(new SortTable(new EnumeratedInternalSortTable()));
	_outtables.push_back(new SortTable(new EnumeratedInternalSortTable()));
}

UnionInternalSortTable::~UnionInternalSortTable() {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		delete (*it);
	}
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		delete (*it);
	}
}

tablesize UnionInternalSortTable::size() const {
	long long result = 0;
	TableSizeType type = TST_APPROXIMATED;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		tablesize tp = (*it)->size();
		switch (tp._type) {
		case TST_APPROXIMATED:
		case TST_EXACT:
			result += tp._size;
			break;
		case TST_INFINITE:
			type = tp._type;
			break;
		}
	}
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		tablesize tp = (*it)->size();
		switch (tp._type) {
		case TST_APPROXIMATED:
		case TST_EXACT:
			if (result > tp._size) {
				result -= tp._size;
			} else {
				result = 0;
			}
			break;
		case TST_INFINITE:
			type = TST_INFINITE; // Is possibly infinite
			break;
		}
	}
	return tablesize(type, result);
}

bool UnionInternalSortTable::finite() const {
	if (approxFinite()) {
		return true;
	} else {
		throw notyetimplemented("Exact finiteness test on union sort tables");
		return approxFinite();
	}
}

bool UnionInternalSortTable::empty() const {
	if (approxEmpty()) {
		return true;
	} else {
		throw notyetimplemented("Exact emptyness test on union sort tables");
		return approxEmpty();
	}
}

bool UnionInternalSortTable::approxFinite() const {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if (not (*it)->approxFinite()) {
			return false;
		}
	}
	return true;
}

bool UnionInternalSortTable::approxEmpty() const {
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if (not (*it)->approxEmpty()) {
			return false;
		}
	}
	return true;
}

bool UnionInternalSortTable::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	bool in = false;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		if ((*it)->contains(d)) {
			in = true;
			break;
		}
	}
	if (not in) {
		return false;
	}

	bool out = false;
	for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
		if ((*it)->contains(d)) {
			out = true;
			break;
		}
	}
	return not out;
}

InternalSortTable* UnionInternalSortTable::add(int i1, int i2) {
	InternalSortTable* temp = this;
	for (int n = i1; n <= i2; ++n) {
		temp = temp->add(createDomElem(n));
	}
	return temp;
}

InternalSortTable* UnionInternalSortTable::add(const DomainElement* d) {
	if (not contains(d)) {
		if (_nrRefs > 1) {
			vector<SortTable*> newin;
			vector<SortTable*> newout;
			for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
				newin.push_back(new SortTable((*it)->internTable()));
			}
			for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
				newout.push_back(new SortTable((*it)->internTable()));
			}
			UnionInternalSortTable* newtable = new UnionInternalSortTable(newin, newout);
			InternalSortTable* newtableWithExtra = newtable->add(d);
			Assert(newtableWithExtra == newtable);
			return newtableWithExtra;
		} else {
			bool in = false;
			for (size_t n = 0; n < _intables.size(); ++n) {
				if (_intables[n]->contains(d)) {
					in = true;
					break;
				}
			}
			if (in) {
				for (size_t n = 0; n < _outtables.size(); ++n) {
					_outtables[n]->remove(d);
				}
			} else {
				Assert(not _intables.empty());
				_intables[0]->add(d);
			}
			return this;
		}
	} else {
		return this;
	}
}

InternalSortTable* UnionInternalSortTable::remove(const DomainElement* d) {
	Assert(not _outtables.empty());
	if (_nrRefs > 1) {
		vector<SortTable*> newin;
		vector<SortTable*> newout;
		for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
			newin.push_back(new SortTable((*it)->internTable()));
		}
		for (auto it = _outtables.cbegin(); it != _outtables.cend(); ++it) {
			newout.push_back(new SortTable((*it)->internTable()));
		}
		UnionInternalSortTable* newtable = new UnionInternalSortTable(newin, newout);
		InternalSortTable* newtableWithoutExtra = newtable->remove(d);
		Assert(newtableWithoutExtra == newtable);
		return newtableWithoutExtra;
	} else {
		_outtables[0]->add(d);
		return this;
	}
}

InternalSortIterator* UnionInternalSortTable::sortBegin() const {
	vector<SortIterator> vsi;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		vsi.push_back((*it)->sortBegin());
	}
	return new UnionInternalSortIterator(vsi, _outtables);
}

InternalSortIterator* UnionInternalSortTable::sortIterator(const DomainElement*) const {
	throw notyetimplemented("intermediate sortiterator for UnionInternalSortTable");
	return NULL;
}

const DomainElement* UnionInternalSortTable::first() const {
	auto isi = sortBegin();
	if (isi->hasNext()) {
		auto f = *(*isi);
		delete (isi);
		return f;
	} else {
		delete (isi);
		return NULL;
	}
}

const DomainElement* UnionInternalSortTable::last() const {
	const DomainElement* result = NULL;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		auto temp = (*it)->last();
		if (temp && contains(temp)) {
			if (result) {
				result = *result < *temp ? temp : result;
			} else {
				result = temp;
			}
		}
	}
	if (not result) {
		throw notyetimplemented("Computation of last element of a UnionInternalSortTable");
	}
	return result;
}

bool UnionInternalSortTable::isRange() const {
	throw notyetimplemented("Exact range test of a UnionInternalSortTable");
	return false;
}

InternalSortTable* InfiniteInternalSortTable::add(const DomainElement* d) {
	if (not contains(d)) {
		UnionInternalSortTable* upt = new UnionInternalSortTable();
		upt->addInTable(new SortTable(this));
		InternalSortTable* temp = upt->add(d);
		if (temp != upt) {
			delete (new SortTable(upt));
		}
		return temp;
	} else {
		return this;
	}
}

InternalSortTable* InfiniteInternalSortTable::remove(const DomainElement* d) {
	if (contains(d)) {
		UnionInternalSortTable* upt = new UnionInternalSortTable();
		upt->addOutTable(new SortTable(this));
		InternalSortTable* temp = upt->remove(d);
		if (temp != upt) {
			delete (new SortTable(upt));
		}
		return temp;
	} else {
		return this;
	}
}

bool AllNaturalNumbers::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	if (d->type() == DET_INT) {
		return d->value()._int >= 0;
	} else {
		return false;
	}
}

InternalSortIterator* AllNaturalNumbers::sortBegin() const {
	return new NatInternalSortIterator();
}

InternalSortIterator* AllNaturalNumbers::sortIterator(const DomainElement* d) const {
	return new NatInternalSortIterator(d->value()._int);
}

const DomainElement* AllNaturalNumbers::first() const {
	return createDomElem(0);
}

const DomainElement* AllNaturalNumbers::last() const {
	return createDomElem(getMaxElem<int>());
}

InternalSortTable* AllNaturalNumbers::add(int i1, int i2) {
	if (i1 >= 0) {
		return this;
	} else {
		int stop = i2 > 0 ? 0 : i2;
		InternalSortTable* temp = this;
		for (int n = i1; n < stop; ++n) {
			temp = temp->add(createDomElem(n));
		}
		return temp;
	}
}

bool AllIntegers::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	return (d->type() == DET_INT);
}

InternalSortIterator* AllIntegers::sortBegin() const {
	return new IntInternalSortIterator();
}

InternalSortIterator* AllIntegers::sortIterator(const DomainElement* d) const {
	return new IntInternalSortIterator(d->value()._int);
}

InternalSortTable* AllIntegers::add(int, int) {
	return this;
}

const DomainElement* AllIntegers::first() const {
	return createDomElem(getMinElem<int>());
}

const DomainElement* AllIntegers::last() const {
	return createDomElem(getMaxElem<int>());
}

bool AllFloats::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	return (d->type() == DET_INT || d->type() == DET_DOUBLE);
}

InternalSortIterator* AllFloats::sortBegin() const {
	return new FloatInternalSortIterator();
}

InternalSortIterator* AllFloats::sortIterator(const DomainElement* d) const {
	double dou = d->type() == DET_DOUBLE ? d->value()._double : double(d->value()._int);
	return new FloatInternalSortIterator(dou);
}

InternalSortTable* AllFloats::add(int, int) {
	return this;
}

const DomainElement* AllFloats::first() const {
	return createDomElem(getMinElem<double>());
}

const DomainElement* AllFloats::last() const {
	return createDomElem(getMaxElem<double>());
}

bool AllStrings::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	return d->type() != DET_COMPOUND;
}

InternalSortTable* AllStrings::add(int, int) {
	return this;
}

InternalSortIterator* AllStrings::sortBegin() const {
	return new StringInternalSortIterator();
}

InternalSortIterator* AllStrings::sortIterator(const DomainElement* d) const {
	string str;
	if (d->type() == DET_INT) {
		str = convertToString(d->value()._int);
	} else if (d->type() == DET_DOUBLE) {
		str = convertToString(d->value()._double);
	} else {
		str = *(d->value()._string);
	}
	return new StringInternalSortIterator(str);
}

const DomainElement* AllStrings::first() const {
	return createDomElem(string(""));
}

const DomainElement* AllStrings::last() const {
	throw notyetimplemented("impossible to get the largest string");
	return NULL;
}

bool AllChars::contains(const DomainElement* d) const {
	if (d == NULL) {
		return false;
	}
	if (d->type() == DET_INT) {
		return (d->value()._int >= 0 && d->value()._int < 10);
	} else if (d->type() == DET_STRING) {
		return (d->value()._string->size() == 1);
	} else {
		return false;
	}
}

InternalSortTable* AllChars::add(int i1, int i2) {
	InternalSortTable* temp = this;
	for (int n = i1; n <= i2; ++n) {
		temp = temp->add(createDomElem(n));
	}
	return temp;
}

InternalSortTable* AllChars::add(const DomainElement* d) {
	if (contains(d)) {
		return this;
	} else {
		auto ist = new EnumeratedInternalSortTable;
		for (char c = getMinElem<char>(); c <= getMaxElem<char>(); ++c) {
			if ('0' <= c && c <= '9') {
				int i = c - '0';
				ist->add(createDomElem(i));
			} else {
				ist->add(createDomElem(string(1, c)));
			}
		}
		ist->add(d);
		return ist;
	}
}

InternalSortTable* AllChars::remove(const DomainElement* d) {
	if (not contains(d)) {
		return this;
	} else {
		auto ist = new EnumeratedInternalSortTable;
		for (char c = getMinElem<char>(); c <= getMaxElem<char>(); ++c) {
			if ('0' <= c && c <= '9') {
				int i = c - '0';
				ist->add(createDomElem(i));
			} else {
				ist->add(createDomElem(string(1, c)));
			}
		}
		ist->remove(d);
		return ist;
	}
}

InternalSortIterator* AllChars::sortBegin() const {
	return new CharInternalSortIterator();
}

InternalSortIterator* AllChars::sortIterator(const DomainElement* d) const {
	char c;
	if (d->type() == DET_INT) {
		c = '0' + d->value()._int;
	} else {
		c = d->value()._string->operator[](0);
	}
	return new CharInternalSortIterator(c);
}

const DomainElement* AllChars::first() const {
	return createDomElem(string(1, getMinElem<char>()));
}

const DomainElement* AllChars::last() const {
	return createDomElem(string(1, getMaxElem<char>()));
}

tablesize AllChars::size() const {
	return tablesize(TST_EXACT, getMaxElem<char>() - getMinElem<char>() + 1);
}

/*************************
 Internal functable
 *************************/

inline void InternalFuncTable::incrementRef() {
	++_nrRefs;
}

inline void InternalFuncTable::decrementRef() {
	--_nrRefs;
	if (_nrRefs == 0) {
		delete (this);
	}
}

bool InternalFuncTable::contains(const ElementTuple& tuple) const {
	ElementTuple input = tuple;
	auto output = tuple.back();
	input.pop_back();
	auto computedoutput = operator[](input);
	return output == computedoutput;
}

ProcInternalFuncTable::~ProcInternalFuncTable() {
}

tablesize ProcInternalPredTable::size(const Universe& univ) const {
	tablesize univsize = univ.size();
	TableSizeType tst = univsize._type;
	if (tst == TST_EXACT) {
		tst = TST_APPROXIMATED;
	}
	return tablesize(tst, univsize._size / 2);
}

tablesize ProcInternalFuncTable::size(const Universe& univ) const {
	auto temptables = univ.tables();
	temptables.pop_back();
	return toDouble(univ.tables().back()->size())>0?Universe(temptables).size():tablesize(TableSizeType::TST_EXACT,0);
}

bool ProcInternalFuncTable::finite(const Universe& univ) const {
	if (empty(univ)) {
		return true;
	}
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (not univ.tables()[n]->finite()) {
			return false;
		}
	}
	return true;
}

bool ProcInternalFuncTable::empty(const Universe& univ) const {
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (univ.tables()[n]->empty()) {
			return true;
		}
	}
	throw notyetimplemented("Exact emptyness test on procedural function tables");
	return false;
}

bool ProcInternalFuncTable::approxFinite(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	}
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (not univ.tables()[n]->approxFinite()) {
			return false;
		}
	}
	return true;
}

bool ProcInternalFuncTable::approxEmpty(const Universe& univ) const {
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (univ.tables()[n]->approxEmpty()) {
			return true;
		}
	}
	return false;
}

const DomainElement* ProcInternalFuncTable::operator[](const ElementTuple& tuple) const {
	return LuaConnection::funccall(_procedure, tuple);
}

InternalFuncTable* ProcInternalFuncTable::add(const ElementTuple&) {
	throw notyetimplemented("adding a tuple to a procedural function interpretation");
}

InternalFuncTable* ProcInternalFuncTable::remove(const ElementTuple&) {
	throw notyetimplemented("removing a tuple from a procedural function interpretation");
}

InternalTableIterator* ProcInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

bool UNAInternalFuncTable::finite(const Universe& univ) const {
	if (empty(univ)) {
		return true;
	}
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (not univ.tables()[n]->finite()) {
			return false;
		}
	}
	return true;
}

bool UNAInternalFuncTable::empty(const Universe& univ) const {
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (univ.tables()[n]->empty()) {
			return true;
		}
	}
	return false;
}

bool UNAInternalFuncTable::approxFinite(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	}
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (not univ.tables()[n]->approxFinite()) {
			return false;
		}
	}
	return true;
}

bool UNAInternalFuncTable::approxEmpty(const Universe& univ) const {
	for (size_t n = 0; n < univ.tables().size() - 1; ++n) {
		if (univ.tables()[n]->approxEmpty()) {
			return true;
		}
	}
	return false;
}

tablesize UNAInternalFuncTable::size(const Universe& univ) const {
	auto vst = univ.tables();
	vst.pop_back();
	return Universe(vst).size();
}

const DomainElement* UNAInternalFuncTable::operator[](const ElementTuple& tuple) const {
	return createDomElem(_function, tuple);
}

InternalFuncTable* UNAInternalFuncTable::add(const ElementTuple&) {
	throw notyetimplemented("adding a tuple to a generated function interpretation");
}

InternalFuncTable* UNAInternalFuncTable::remove(const ElementTuple&) {
	throw notyetimplemented("removing a tuple from a generated function interpretation");
}

InternalTableIterator* UNAInternalFuncTable::begin(const Universe& univ) const {
	vector<SortIterator> vsi;
	for (size_t n = 0; n < _function->arity(); ++n) {
		vsi.push_back(univ.tables()[n]->sortBegin());
	}
	return new UNAInternalIterator(vsi, _function);
}

const DomainElement* EnumeratedInternalFuncTable::operator[](const ElementTuple& tuple) const {
	auto it = _table.find(tuple);
	if (it == _table.cend()) {
		return NULL;
	}
	return it->second;
}

EnumeratedInternalFuncTable* EnumeratedInternalFuncTable::add(const ElementTuple& tuple) {
	ElementTuple key = tuple;
	const DomainElement* mappedvalue = key.back();
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	if (computedvalue == NULL) {
		if (_nrRefs > 1) {
			Tuple2Elem newtable = _table;
			newtable[key] = mappedvalue;
			return new EnumeratedInternalFuncTable(newtable); // TODO memory leak
		} else {
			_table[key] = mappedvalue;
			return this;
		}
	}

	if(computedvalue!=mappedvalue){
		throw IdpException("Attempting to add a new image to an already existing tuple in an enumerated function table.");
	}
	return this;
}

EnumeratedInternalFuncTable* EnumeratedInternalFuncTable::remove(const ElementTuple& tuple) {
	ElementTuple key = tuple;
	const DomainElement* value = key.back();
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	if (computedvalue == value) {
		if (_nrRefs > 1) {
			Tuple2Elem newtable = _table;
			newtable.erase(key);
			return new EnumeratedInternalFuncTable(newtable);
		} else {
			_table.erase(key);
			return this;
		}
	} else {
		return this;
	}
}

InternalTableIterator* EnumeratedInternalFuncTable::begin(const Universe&) const {
	return new EnumInternalFuncIterator(_table.cbegin(), _table.cend());
}

void EnumeratedInternalFuncTable::put(std::ostream& stream) const {
	stream << "EnumeratedInternalFuncTable containing: (";
	size_t i = 0;
	for (auto it = _table.cbegin(); i < 5 && it != _table.cend(); i++, it++) {
		if (i != 0) {
			stream << ", ";
		}
		stream << print(it->first) << "->" << (print(it->second));
	}
	if (_table.size() > i) {
		stream << ",...";
	}
	stream << ")";
}

const DomainElement* ModInternalFuncTable::operator[](const ElementTuple& tuple) const {
	int a1 = tuple[0]->value()._int;
	int a2 = tuple[1]->value()._int;
	if (a2 == 0) {
		return NULL;
	} else {
		int cppModulo = a1 % a2;
		if (cppModulo < 0) {
			return createDomElem(cppModulo + a2);
		}
		return createDomElem(cppModulo);
	}
}

InternalFuncTable* ModInternalFuncTable::add(const ElementTuple&) {
	throw IdpException("Cannot add elements to this table.");
}

InternalFuncTable* ModInternalFuncTable::remove(const ElementTuple&) {
	throw IdpException("Cannot remove elements from this table.");
}

InternalTableIterator* ModInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* ExpInternalFuncTable::operator[](const ElementTuple& tuple) const {
	double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
	double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
	return createDomElem(pow(a1, a2), NumType::POSSIBLYINT);
}

InternalFuncTable* ExpInternalFuncTable::add(const ElementTuple&) {
	throw IdpException("Cannot add elements to this table.");
}

InternalFuncTable* ExpInternalFuncTable::remove(const ElementTuple&) {
	throw IdpException("Cannot remove elements from this table.");
}

InternalTableIterator* ExpInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

InternalFuncTable* IntFloatInternalFuncTable::add(const ElementTuple&) {
	throw IdpException("Cannot add elements to this table.");
}

InternalFuncTable* IntFloatInternalFuncTable::remove(const ElementTuple&) {
	throw IdpException("Cannot remove elements from this table.");
}

const DomainElement* PlusInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if (getType() == NumType::CERTAINLYINT) {
		return sum<int>(tuple[0]->value()._int, tuple[1]->value()._int);
	} else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return createDomElem(a1 + a2, NumType::POSSIBLYINT);
	}
}

InternalTableIterator* PlusInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* MinusInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if (getType() == NumType::CERTAINLYINT) {
		return difference<int>(tuple[0]->value()._int, tuple[1]->value()._int);
	} else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return createDomElem(a1 - a2, NumType::POSSIBLYINT);
	}
}

InternalTableIterator* MinusInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* TimesInternalFuncTable::operator[](const ElementTuple& tuple) const {
	Assert(tuple.size()==2);
	if (getType() == NumType::CERTAINLYINT) {
		return product<int>(tuple[0]->value()._int, tuple[1]->value()._int);
	} else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return createDomElem(a1 * a2, NumType::POSSIBLYINT);
	}
}

InternalTableIterator* TimesInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* DivInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if (getType() == NumType::CERTAINLYINT) {
		if (tuple[1]->value()._int == 0) {
			return NULL;
		}
		return division<int>(tuple[0]->value()._int, tuple[1]->value()._int);
	} else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		if (a2 == 0) {
			return NULL;
		} else {
			return createDomElem(a1 / a2, NumType::POSSIBLYINT);
		}
	}
}

InternalTableIterator* DivInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* AbsInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if (tuple[0]->type() == DET_INT) {
		int val = tuple[0]->value()._int;
		return createDomElem(val < 0 ? -val : val);
	} else {
		return createDomElem(abs(tuple[0]->value()._double), NumType::POSSIBLYINT);
	}
}

InternalTableIterator* AbsInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

const DomainElement* UminInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if (tuple[0]->type() == DET_INT) {
		return createDomElem(-(tuple[0]->value()._int));
	} else {
		return createDomElem(-(tuple[0]->value()._double), NumType::POSSIBLYINT);
	}
}

InternalTableIterator* UminInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this, univ);
}

void PredTable::put(std::ostream& stream) const {
	if (not approxFinite()) {
		stream << "infinite interpretation";
		return;
	}
	stream << "{";
	bool start = true;
	for (auto it = begin(); not it.isAtEnd(); ++it) {
		if (not start) {
			stream << "; ";
		}
		start = false;
		stream << *it;
	}
	stream << "}";
}

void FuncTable::put(std::ostream& stream) const {
	stream << print(_table);
}

void SortTable::put(std::ostream& stream) const {
	if (empty()) {
		stream << print(_table) << " is empty";
	} else {
		stream << print(_table) << "[" << print(first()) << ", ";
		if(finite()){
			stream << print(last());
		}else{
			stream << "...";
		}
		stream << "]";
	}
}

SortTable* SortTable::clone() const{
	return new SortTable(_table);
}


/****************
 PredTable
 ****************/

int createdtables = 0, deletedtables = 0;
PredTable::PredTable(InternalPredTable* table, const Universe& univ)
		: _table(NULL), _universe(univ) {
	setTable(table);
	table->incrementRef();
}

PredTable::~PredTable() {
	_table->decrementRef();
}

void PredTable::setTable(InternalPredTable* table) {
	/**
	 * TODO optimize table for size here:
	 * 		non-inverted table => contains by search in table                    log(|table|)
	 * 						   => iterate by walking over table                  |table| for |table| results
	 * 		inverted table     => contains by search in table and in universe    log(|table|) + log(|univ|)
	 * 		                   => iterate by walk over univ and check with table |univ| * log(|table|) for |univ|-|table| results
	 * 		                   		==> if table is very large, e.g. another inverted table, then ...
	 */
	Assert(table!=NULL);
	_table = table;
}

void PredTable::add(const ElementTuple& tuple, bool) {
	Assert(arity() == tuple.size());
	if (_table->contains(tuple, _universe)) {
		return;
	}
	auto temp = _table;
	setTable(_table->add(tuple));
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void PredTable::remove(const ElementTuple& tuple) {
	Assert(arity() == tuple.size());
	if (not _table->contains(tuple, _universe)) {
		return;
	}
	auto temp = _table;
	setTable(_table->remove(tuple));
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

TableIterator PredTable::begin() const {
	TableIterator ti(_table->begin(_universe));
	return ti;
}

void InternalPredTable::decrementRef() {
	--_nrRefs;
	if (_nrRefs == 0) {
		delete (this);
	}
}

void InternalPredTable::incrementRef() {
	++_nrRefs;
}

void InternalPredTable::put(std::ostream& stream) const {
	stream << typeid(*this).name();
}

bool InternalPredTable::approxEqual(const InternalPredTable* ipt, const Universe& u) const {
	if (ipt == this) {
		return true;
	}
	auto iipt = dynamic_cast<const InverseInternalPredTable*>(ipt);
	if (iipt != NULL) {
		return approxInverse(iipt->table(), u);
	}
	return false;
}

bool InternalPredTable::approxInverse(const InternalPredTable* ipt, const Universe& u) const {
	auto iipt = dynamic_cast<const InverseInternalPredTable*>(ipt);
	if (iipt == NULL) {
		return false;
	}
	return approxEqual(iipt->table(), u);
}


void InternalFuncTable::put(std::ostream& stream) const {
	stream << typeid(*this).name();
}

ProcInternalPredTable::~ProcInternalPredTable() {
}

bool ProcInternalPredTable::finite(const Universe& univ) const {
	return univ.finite();
}

bool ProcInternalPredTable::empty(const Universe& univ) const {
	return univ.empty();
}

bool ProcInternalPredTable::approxFinite(const Universe& univ) const {
	return univ.approxFinite();
}

bool ProcInternalPredTable::approxEmpty(const Universe& univ) const {
	return univ.approxEmpty();
}

bool ProcInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if (univ.contains(tuple)) {
		return LuaConnection::predcall(_procedure, tuple);
	} else {
		return false;
	}
}

InternalPredTable* ProcInternalPredTable::add(const ElementTuple&) {
	throw notyetimplemented("Adding a tuple to a procedural predicate table");
	return this;
}

InternalPredTable* ProcInternalPredTable::remove(const ElementTuple&) {
	throw notyetimplemented("Removing a tuple from a procedural predicate table");
	return this;
}

InternalTableIterator* ProcInternalPredTable::begin(const Universe& univ) const {
	return new ProcInternalTableIterator(this, univ);
}

InverseInternalPredTable::InverseInternalPredTable(InternalPredTable* inv)
		: InternalPredTable(), _invtable(inv) {
	inv->incrementRef();
}

InternalPredTable* InverseInternalPredTable::getInverseTable(InternalPredTable* inv){
	auto iipt = dynamic_cast<InverseInternalPredTable*>(inv);
	if(iipt != NULL){
		return iipt->table();
	}
	auto bddpt = dynamic_cast<BDDInternalPredTable*>(inv);
	if (bddpt != NULL) {
		auto mgr = bddpt->bdd()->manager();
		return new BDDInternalPredTable(mgr->negation(bddpt->bdd()), mgr, bddpt->vars(), bddpt->structure());
	}
	return new InverseInternalPredTable(inv);
}


InverseInternalPredTable::~InverseInternalPredTable() {
	_invtable->decrementRef();
}

void InverseInternalPredTable::internTable(InternalPredTable* ipt) {
	if(ipt!=NULL){
		ipt->incrementRef();
	}
	if(_invtable!=NULL) {
		_invtable->decrementRef();
	}
	_invtable = ipt;
}

/**
 *		Returns true iff the table is finite
 */
bool InverseInternalPredTable::finite(const Universe& univ) const {
	if (approxFinite(univ)) {
		return true;
	}
	if (univ.finite()) {
		return true;
	} else if (_invtable->finite(univ)) {
		return false;
	} else {
		throw notyetimplemented("Exact finiteness test on inverse predicate tables");
		return approxEmpty(univ);
	}
}

/**
 *		Returns true iff the table is empty
 */
bool InverseInternalPredTable::empty(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	}
	if (univ.empty()) {
		return true;
	}
	if (approxFinite(univ)) {
		auto ti = TableIterator(begin(univ));
		return ti.isAtEnd();
	} else {
		throw notyetimplemented("Exact emptyness test on inverse predicate tables");
		return approxEmpty(univ);
	}
}

/**
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
bool InverseInternalPredTable::approxFinite(const Universe& univ) const {
	return univ.approxFinite();
}

/**
 *		Returns false if the table is non-empty. May return true if the table is empty.
 */
bool InverseInternalPredTable::approxEmpty(const Universe& univ) const {
	return univ.approxEmpty();
}

bool InverseInternalPredTable::approxInverse(const InternalPredTable* ipt, const Universe& univ) const {
	return _invtable->approxEqual(ipt, univ);
}

tablesize InverseInternalPredTable::size(const Universe& univ) const {
	tablesize univsize = univ.size();
	tablesize invsize = _invtable->size(univ);
	if (univsize._type == TST_INFINITE) {
		if (invsize._type == TST_APPROXIMATED || invsize._type == TST_EXACT) {
			return tablesize(TST_INFINITE, 0);
		} else {
			return tablesize(TST_INFINITE, 0);
		}
	} else if (invsize._type == TST_APPROXIMATED || invsize._type == TST_EXACT) {
		unsigned int result = 0;
		if (univsize._size > invsize._size) {
			result = univsize._size - invsize._size;
		}
		if (invsize._type == TST_EXACT && univsize._type == TST_EXACT) {
			return tablesize(TST_EXACT, result);
		} else {
			return tablesize(TST_APPROXIMATED, result);
		}
	} else {
		return tablesize(TST_INFINITE, 0); // possibly infinite
	}
}

/**
 *	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool InverseInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if (_invtable->contains(tuple, univ)) {
		return false;
	} else if (univ.contains(tuple)) {
		return true;
	} else {
		return false;
	}
}

/**
 * Add a tuple to the table.
 *
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* InverseInternalPredTable::add(const ElementTuple& tuple) {
	if (_nrRefs > 1) {
		auto newtable = InverseInternalPredTable::getInverseTable(_invtable);
		InternalPredTable* newtableWithExtra = newtable->add(tuple);
		Assert(newtableWithExtra == newtable);
		return newtableWithExtra;
	} else {
		InternalPredTable* temp = _invtable->remove(tuple);
		if (temp != _invtable) {
			_invtable->decrementRef();
			temp->incrementRef();
			_invtable = temp;
		}
		return this;
	}
}

/**
 * Remove a tuple from the table.
 *
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* InverseInternalPredTable::remove(const ElementTuple& tuple) {
	if (_nrRefs > 1) {
		auto newtable = InverseInternalPredTable::getInverseTable(_invtable);
		InternalPredTable* newtableWithoutExtra = newtable->remove(tuple);
		Assert(newtableWithoutExtra == newtable);
		return newtableWithoutExtra;
	} else {
		InternalPredTable* temp = _invtable->add(tuple);
		if (temp != _invtable) {
			_invtable->decrementRef();
			temp->incrementRef();
			_invtable = temp;
		}
		return this;
	}
}

InternalTableIterator* InverseInternalPredTable::begin(const Universe& univ) const {
	vector<SortIterator> vsi;
	for (auto it = univ.tables().cbegin(); it != univ.tables().cend(); ++it) {
		vsi.push_back((*it)->sortBegin());
	}
	return new InverseInternalIterator(vsi, _invtable, univ);
}

/****************
 SortTable
 ****************/

SortTable::SortTable(InternalSortTable* table)
		: _table(table) {
	_table->incrementRef();
}

SortTable::~SortTable() {
	_table->decrementRef();
}

TableIterator SortTable::begin() const {
	return TableIterator(_table->begin());
}

SortIterator SortTable::sortBegin() const {
	return SortIterator(_table->sortBegin());
}

SortIterator SortTable::sortIterator(const DomainElement* d) const {
	return SortIterator(_table->sortIterator(d));
}

void SortTable::internTable(InternalSortTable* table) {
	_table->decrementRef();
	_table = table;
	_table->incrementRef();
}

void SortTable::add(const ElementTuple& tuple, bool) {
	if (_table->contains(tuple)) {
		return;
	}
	auto temp = _table;
	_table = _table->add(tuple);
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::add(const DomainElement* el) {
	add(ElementTuple { el });
}

void SortTable::add(int i1, int i2) {
	auto temp = _table;
	_table = _table->add(i1, i2);
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::remove(const ElementTuple& tuple) {
	if (not _table->contains(tuple)) {
		return;
	}
	auto temp = _table;
	_table = _table->remove(tuple);
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::remove(const DomainElement* el) {
	remove(ElementTuple { el });
}

/****************
 FuncTable
 ****************/

FuncTable::FuncTable(InternalFuncTable* table, const Universe& univ)
		: _table(table), _universe(univ) {
	Assert(univ.tables().size()>0);
	_table->incrementRef();
}

FuncTable::~FuncTable() {
	_table->decrementRef();
}

void FuncTable::add(const ElementTuple& tuple, bool ignoresortsortchecks) {
	if (_table->contains(tuple) || (not ignoresortsortchecks && not _universe.contains(tuple))) {
		return;
	}
	InternalFuncTable* temp = _table;
	_table = _table->add(tuple);
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void FuncTable::remove(const ElementTuple& tuple) {
	if (not _table->contains(tuple) || not _universe.contains(tuple)) {
		return;
	}
	InternalFuncTable* temp = _table;
	_table = _table->remove(tuple);
	if (temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

/**
 * Given a tuple (a_1,...,a_{n-1},a_n), this procedure returns true iff the value of
 * (a_1,...,a_{n-1}) according to the function is equal to a_n
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool FuncTable::contains(const ElementTuple& tuple) const {
	Assert(tuple.size() == arity()+1);
	ElementTuple key = tuple;
	const DomainElement* value = key.back();
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	return value == computedvalue;
}

TableIterator FuncTable::begin() const {
	TableIterator ti(_table->begin(_universe));
	return ti;
}

/********************************
 Predicate interpretations
 ********************************/


PredInter::PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf): _ct(NULL), _cf(NULL), _pt(NULL), _pf(NULL) {
	setTables(ctpf,cfpt,ct,cf);
}

void PredInter::setTables(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf){
	auto inverseCtpf = new PredTable(InverseInternalPredTable::getInverseTable(ctpf->internTable()), ctpf->universe());
	auto inverseCfpt = new PredTable(InverseInternalPredTable::getInverseTable(cfpt->internTable()), ctpf->universe());
	_inconsistentElements = {};
	if (ct) {
		if(ctpf!=_ct){
			delete(_ct);
		}
		delete(_pf);
		_ct = ctpf;
		_pf = inverseCtpf;
	} else {
		if(ctpf!=_pf){
			delete(_pf);
		}
		delete(_ct);
		_pf = ctpf;
		_ct = inverseCtpf;
	}
	if (cf) {
		if(cfpt!=_cf){
			delete(_cf);
		}
		delete(_pt);
		_cf = cfpt;
		_pt = inverseCfpt;
	} else {
		if(cfpt!=_pt){
			delete(_pt);
		}
		delete(_cf);
		_pt = cfpt;
		_cf = inverseCfpt;
	}
	checkConsistency();
}

PredInter::PredInter(PredTable* table, bool ct) {
	auto copiedtable = new PredTable(table->internTable(), table->universe());
	auto inverseCtpf = new PredTable(InverseInternalPredTable::getInverseTable(table->internTable()), table->universe());
	auto inverseCfpt = new PredTable(InverseInternalPredTable::getInverseTable(copiedtable->internTable()), copiedtable->universe());
	_inconsistentElements = {};
	if (ct) {
		_ct = table;
		_pt = copiedtable;
		_cf = inverseCtpf;
		_pf = inverseCfpt;
	} else {
		_pf = table;
		_cf = copiedtable;
		_ct = inverseCtpf;
		_pt = inverseCfpt;
	}
}

/**
 * \brief Destructor for predicate interpretations
 */
PredInter::~PredInter() {
	delete (_ct);
	delete (_cf);
	delete (_pt);
	delete (_pf);
}

PredInter* createSmallestPredInter(PredTable* ct, PredTable* cf, bool known_twovalued){
	if (known_twovalued || ct->approxInverse(cf)) {
		//In case the interpretation is twovalued, we only need to keep one table.
		//We decide to keep the smallest of ct and cf
		if (ct->size() > cf->size()) {
			delete ct;
			return new PredInter(cf, false);
		}
		delete cf;
		return new PredInter(ct, true);
	}

	//In the other case, where the inter will not be twovalued.
	//We need to keep two tables: the ct or the pf and the cf or the pt
	//In both cases, we want to keep the smallest of the two.
	//Thus: first we check which of the two will be the smallest.
	auto pf = new PredTable(InverseInternalPredTable::getInverseTable(ct->internTable()), ct->universe());
	auto pt = new PredTable(InverseInternalPredTable::getInverseTable(cf->internTable()), cf->universe());
	auto ctbigger = ct->size() > pf->size();
	auto cfbigger = cf->size() > pt->size();
	//Now we make to predtable representing the table we keep: ctpf is either ct or pf and analogously for the cfpt
	PredTable *ctpf, *cfpt;
	if (ctbigger) {
		delete (ct);
		ctpf = pf;
	} else{
		delete (pf);
		ctpf = ct;
	}
	if (cfbigger) {
		delete (cf);
		cfpt = pt;
	} else{
		delete (pt);
		cfpt = cf;
	}
	return new PredInter(ctpf, cfpt, not ctbigger, not cfbigger);
}

/**
 * \brief Returns true iff the tuple is true or inconsistent according to the predicate interpretation
 */
bool PredInter::isTrue(const ElementTuple& tuple, bool) const {
	return _ct->contains(tuple);
}

/**
 * \brief Returns true iff the tuple is false or inconsistent according to the predicate interpretation
 */
bool PredInter::isFalse(const ElementTuple& tuple, bool ignoresortchecks) const {
	if (_cf->contains(tuple)) {
		return true;
	}
	if (not ignoresortchecks && not universe().contains(tuple)) {
		return true;
	}
	return false;
}

/**
 * \brief Returns true iff the tuple is unknown according to the predicate interpretation
 */
bool PredInter::isUnknown(const ElementTuple& tuple, bool ignoresortchecks) const {
	if (approxTwoValued()) {
		return false;
	} else {
		return not (isFalse(tuple, ignoresortchecks) || isTrue(tuple, ignoresortchecks));
	}
}

/**
 * \brief Returns true iff the tuple is inconsistent according to the predicate interpretation
 */
bool PredInter::isInconsistent(const ElementTuple& tuple) const {
	return _inconsistentElements.find(tuple) != _inconsistentElements.cend();
}

bool PredInter::isConsistent() const {
	return _inconsistentElements.size() == 0;
}

const std::set<ElementTuple>& PredInter::getInconsistentAtoms() const{
	return _inconsistentElements;
}

void PredInter::checkConsistency() {
	_inconsistentElements.clear();
	if(_ct->approxInverse(_cf)){
		return;
	}
	if (not _ct->approxFinite() && not _cf->approxFinite()) {
		throw notyetimplemented("Check consistency between an infinite certainly-true and an infinite certainly-false table");
	}

	auto smallest = _ct->size() < _cf->size() ? _ct : _cf; // Walk over the smallest table first => also optimal behavior in case one is emtpy
	auto largest = (smallest == _ct) ? _cf : _ct;

	auto sPossTable = smallest == _ct ? _pt : _pf;
	auto lPossTable = smallest == _ct ? _pf : _pt;

	FirstNElementsEqual eq(smallest->arity());
	StrictWeakNTupleOrdering so(smallest->arity());

	//At this point, what we should do is: either
	//  * check whether ct \cap cf = \emptyset
	//  * check whether ct (cf) \subseteq pt (pf)
	// depending on the table that is explicitely represented. (one is an inversetable of the other, we search the explicitely represented one)

	if (not isa<InverseInternalPredTable>(*largest->internTable())) {
		auto largeIt = largest->begin();
		for (auto smallIt = smallest->begin(); not smallIt.isAtEnd(); ++smallIt) {
			CHECKTERMINATION;
			// find this element in large table
			while (not largeIt.isAtEnd() && so(*largeIt, *smallIt)) {
				CHECKTERMINATION;
				Assert(sPossTable->size()._size > 5 || not sPossTable->contains(*largeIt));
				// NOTE: checking pt and pf can be very expensive in large domains, so the debugging check is only done for small domains
				//Should always be true...
				++largeIt;
			}
			if (not largeIt.isAtEnd() && eq(*largeIt, *smallIt)) {
				_inconsistentElements.insert(*largeIt);
			}
			Assert(lPossTable->size()._size > 5 || not lPossTable->contains(*smallIt));
			// NOTE: checking pt and pf can be very expensive in large domains, so the debugging check is only done for small domains
			//Should always be true...
		}
	} else {
		auto smallposIt = sPossTable->begin();
		for (auto smallIt = smallest->begin() ; not smallIt.isAtEnd(); ++smallIt) {
			CHECKTERMINATION;
			// find this element in possibly small table
			while (not smallposIt.isAtEnd() && so(*smallposIt, *smallIt)) {
				CHECKTERMINATION;
				++smallposIt;
			}
			if (smallposIt.isAtEnd() || not eq(*smallposIt, *smallIt)) {
				_inconsistentElements.insert(*smallIt);
			}
		}
	}
}

/**
 * \brief Returns false if the interpretation is not two-valued. May return true if it is two-valued.
 *
 * NOTE: Simple check if _ct == _pt
 */
bool PredInter::approxTwoValued() const {
// TODO turn it into something that is smarter, without comparing the tables!
// => return isConsistent() && isFinite(universe().size()._type) && _ct->size()+_cf->size()==universe().size()._size;
	return isConsistent() && _ct->approxEqual(_pt);
}

void PredInter::makeTrueExactly(const ElementTuple& tuple, bool ignoresortchecks) {
	if (_inconsistentElements.find(tuple) != _inconsistentElements.cend()) {
		_inconsistentElements.erase(tuple);
	}
	moveTupleFromTo(tuple, _cf, _pt, ignoresortchecks);
	moveTupleFromTo(tuple, _pf, _ct, ignoresortchecks);
}

void PredInter::makeFalseExactly(const ElementTuple& tuple, bool ignoresortchecks) {
	if (_inconsistentElements.find(tuple) != _inconsistentElements.cend()) {
		_inconsistentElements.erase(tuple);
	}
	moveTupleFromTo(tuple, _pt, _cf, ignoresortchecks);
	moveTupleFromTo(tuple, _ct, _pf, ignoresortchecks);
}

void PredInter::makeUnknownExactly(const ElementTuple& tuple, bool ignoresortchecks) {
	if (_inconsistentElements.find(tuple) != _inconsistentElements.cend()) {
		_inconsistentElements.erase(tuple);
	}
	moveTupleFromTo(tuple, _cf, _pt, ignoresortchecks);
	moveTupleFromTo(tuple, _ct, _pf, ignoresortchecks);
}

void PredInter::makeTrueAtLeast(const ElementTuple& tuple, bool ignoresortchecks) {
	if (isFalse(tuple, ignoresortchecks)) {
		_inconsistentElements.insert(tuple);
	}
	moveTupleFromTo(tuple, _pf, _ct, ignoresortchecks);
}

void PredInter::makeFalseAtLeast(const ElementTuple& tuple, bool ignoresortchecks) {
	if (isTrue(tuple, ignoresortchecks)) {
		_inconsistentElements.insert(tuple);
	}
	if(not universe().contains(tuple)){
		return; // already false
	}
	moveTupleFromTo(tuple, _pt, _cf, ignoresortchecks);
}

void PredInter::moveTupleFromTo(const ElementTuple& tuple, PredTable* from, PredTable* to, bool ignoresortchecks) {
	Assert(from->approxInverse(to));
	if (tuple.size() != universe().arity()) {
		stringstream ss;
		ss << "Adding a tuple of size " << tuple.size() << " to a predicate with arity " << universe().arity();
		throw IdpException(ss.str());
	}
	if (not ignoresortchecks) {
		auto table = universe().tables().cbegin();
		auto elem = tuple.cbegin();
		for (; table < universe().tables().cend(); ++table, ++elem) {
			if (not (*table)->contains(*elem)) {
				stringstream ss;
				ss << "Element " << print(*elem) << " is not part of table " << print(*table) << ", but you are trying to assign it to such a table";
				throw IdpException(ss.str());
			}
		}
	}
	if (isa<InverseInternalPredTable>(*(from->internTable()))) {
		auto internfrom = dynamic_cast<InverseInternalPredTable*>(from->internTable());
		Assert(internfrom->table() == to->internTable());
		internfrom->internTable(NULL);
		to->add(tuple);
		internfrom->internTable(to->internTable());
	} else {
		auto internto = dynamic_cast<InverseInternalPredTable*>(to->internTable());
		Assert(internto->table() == from->internTable());
		internto->internTable(NULL);
		from->remove(tuple);
		internto->internTable(from->internTable());
	}
}

void PredInter::ct(PredTable* t) {
	delete (_ct);
	delete (_pf);
	_ct = t;
	_pf = new PredTable(InverseInternalPredTable::getInverseTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::cf(PredTable* t) {
	delete (_cf);
	delete (_pt);
	_cf = t;
	_pt = new PredTable(InverseInternalPredTable::getInverseTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::pt(PredTable* t) {
	delete (_pt);
	delete (_cf);
	_pt = t;
	_cf = new PredTable(InverseInternalPredTable::getInverseTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::pf(PredTable* t) {
	delete (_pf);
	delete (_ct);
	_pf = t;
	_ct = new PredTable(InverseInternalPredTable::getInverseTable(t->internTable()), t->universe());
	checkConsistency();
}

// Direct implementation to prevent checking consistency unnecessarily
void PredInter::ctpt(PredTable* newct) { // FIXME also change in other tables: it is possible that an already assigned table is assigned otherwise, so it gets
// deleted in the process!!!
	auto clone = new PredTable(newct->internTable(), newct->universe());
	delete (_ct);
	delete (_pf);
	delete (_pt);
	delete (_cf);
	_ct = clone;
	_pt = new PredTable(_ct->internTable(), _ct->universe());
	_pf = new PredTable(InverseInternalPredTable::getInverseTable(_ct->internTable()), _ct->universe());
	_cf = new PredTable(InverseInternalPredTable::getInverseTable(_ct->internTable()), _ct->universe());
}

// Direct implementation to prevent checking consistency unnecessarily
void PredInter::cfpf(PredTable* newcf) { // FIXME also change in other tables: it is possible that an already assigned table is assigned otherwise, so it gets
// deleted in the process!!!
	auto clone = new PredTable(newcf->internTable(), newcf->universe());
	delete (_ct);
	delete (_pf);
	delete (_pt);
	delete (_cf);
	_cf = clone;
	_pf = new PredTable(_cf->internTable(), _cf->universe());
	_pt = new PredTable(InverseInternalPredTable::getInverseTable(_cf->internTable()), _cf->universe());
	_ct = new PredTable(InverseInternalPredTable::getInverseTable(_cf->internTable()), _cf->universe());
}

void PredInter::materialize() {
	auto getCT = (_ct->size() <= _pf->size());
	auto getCF = (_cf->size() <= _pt->size());

	if (approxTwoValued()) {
		auto newt = getCT ? _ct->materialize() : _pf->materialize();
		if (newt != NULL) {
			getCT ? ctpt(newt) : cfpf(newt);
		}
	} else {
		auto newt = getCT ? _ct->materialize() : _pf->materialize();
		auto newf = getCF ? _cf->materialize() : _pt->materialize();
		if (newt != NULL && newf != NULL) {
			setTables(newt, newf, getCT, getCF);
		} else if (newt != NULL) {
			getCT ? ct(newt) : pf(newt);
		} else if (newf != NULL) {
			getCF ? cf(newf) : pt(newf);
		}
	}
}
PredInter* PredInter::clone() const {
	return clone(universe());
}

PredInter* PredInter::clone(const Universe& univ) const {
	PredTable* nctpf;
	bool ct;
	if (typeid(*(_pf->internTable())) == typeid(InverseInternalPredTable)) {
		nctpf = new PredTable(_ct->internTable(), univ);
		ct = true;
	} else {
		nctpf = new PredTable(_pf->internTable(), univ);
		ct = false;
	}
	auto result = new PredInter(nctpf, ct);
	if (approxTwoValued()) {
		return result;
	}

	PredTable* ncfpt;
	bool cf;
	if (typeid(*(_pt->internTable())) == typeid(InverseInternalPredTable)) {

		ncfpt = new PredTable(_cf->internTable(), univ);
		cf = true;
	} else {
		ncfpt = new PredTable(_pt->internTable(), univ);
		cf = false;
	}
	auto inverseCfpt = new PredTable(InverseInternalPredTable::getInverseTable(ncfpt->internTable()), univ);
	delete (result->_cf);
	delete (result->_pt);
	if (cf) {
		result->_cf = ncfpt;
		result->_pt = inverseCfpt;
	} else {
		result->_pt = ncfpt;
		result->_cf = inverseCfpt;
	}
	result->_inconsistentElements = _inconsistentElements; //OPTIMIZATION!
	return result;

}
void PredInter::put(std::ostream& stream) const {
	if (approxTwoValued()) {
		_ct->put(stream);
	} else {
		stream << "<ct>: ";
		_ct->put(stream);
		stream << nt() << "<cf>: ";
		_cf->put(stream);
	}
}

std::ostream& operator<<(std::ostream& stream, const PredInter& interpretation) {
	stream << "Certainly true: " << print(interpretation.ct()) << "\n";
	stream << "Certainly false: " << print(interpretation.cf()) << "\n";
	stream << "Possibly true: " << print(interpretation.pt()) << "\n";
	stream << "Possibly false: " << print(interpretation.pf()) << "\n";
	return stream;
}

PredInter* InconsistentPredInterGenerator::get(const Structure* structure) {
	auto emptytable = new PredTable(new EnumeratedInternalPredTable(), structure->universe(_predicate));
	return new PredInter(emptytable, emptytable, false, false);
}

// FIXME better way of managing (the memory of) these interpretations?
EqualInterGenerator::~EqualInterGenerator() {
	//deleteList(generatedInters);
}
PredInter* EqualInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2, st));
	EqualInternalPredTable* eip = new EqualInternalPredTable();
	PredTable* ct = new PredTable(eip, univ);
	auto pi = new PredInter(ct, true);
	structure->addInterToDelete(pi);
	return pi;
}

StrLessThanInterGenerator::~StrLessThanInterGenerator() {
	deleteList(generatedInters);
}
PredInter* StrLessThanInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2, st));
	StrLessInternalPredTable* eip = new StrLessInternalPredTable();
	PredTable* ct = new PredTable(eip, univ);
	generatedInters.push_back(new PredInter(ct, true));
	return generatedInters.back();
}

StrGreaterThanInterGenerator::~StrGreaterThanInterGenerator() {
	deleteList(generatedInters);
}
PredInter* StrGreaterThanInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2, st));
	StrGreaterInternalPredTable* eip = new StrGreaterInternalPredTable();
	PredTable* ct = new PredTable(eip, univ);
	generatedInters.push_back(new PredInter(ct, true));
	return generatedInters.back();
}

EqualInterGenerator* EqualInterGeneratorGenerator::get(const std::vector<Sort*>& sorts) {
	return new EqualInterGenerator(sorts[0]);
}

StrGreaterThanInterGenerator* StrGreaterThanInterGeneratorGenerator::get(const std::vector<Sort*>& sorts) {
	return new StrGreaterThanInterGenerator(sorts[0]);
}

StrLessThanInterGenerator* StrLessThanInterGeneratorGenerator::get(const std::vector<Sort*>& sorts) {
	return new StrLessThanInterGenerator(sorts[0]);
}

/*******************************
 Function interpretations
 *******************************/

FuncInter::FuncInter(FuncTable* ft)
		: _functable(ft) {
	//We cannot pass ft to the predtable since on several occasions, _functable gets deleted
	PredTable* ct = new PredTable(new FuncInternalPredTable(new FuncTable(ft->internTable(), ft->universe()), true), ft->universe());
	_graphinter = new PredInter(ct, true);
}

FuncInter::~FuncInter() {
	if (_functable) {
		delete (_functable);
	}
	delete (_graphinter);
}

void FuncInter::graphInter(PredInter* pt) {
	delete (_graphinter);
	_graphinter = pt;
	if (_functable) {
		delete (_functable);
	}
	_functable = 0;
}

void FuncInter::funcTable(FuncTable* ft) {
	delete (_functable);
	delete (_graphinter);
	_functable = ft;
	//We cannot pass ft to the predtable since on several occasions, _functable gets deleted
	PredTable* ct = new PredTable(new FuncInternalPredTable(new FuncTable(ft->internTable(), ft->universe()), true), ft->universe());
	_graphinter = new PredInter(ct, true);
}

const DomainElement* FuncInter::value(const ElementTuple& tuple) const{
	if(tuple.size() != this->universe().tables().size()-1){
		throw new IdpException("Wrong number of arguments given while evaluating function");
	}
	if(approxTwoValued()){
		return funcTable()->operator [](tuple);
	}else{
		for(auto i = graphInter()->ct()->begin() ; not i.isAtEnd() ; ++i ){
			bool equal = true;
			for(auto j = 0 ; j < tuple.size() ; j++){
				if( (*i)[j] != tuple[j] ){
					equal = false;
					break;
				}
			}
			if(equal){
				return (*i)[tuple.size()];
			}
		}
		return 0;
	}
}

void FuncInter::materialize() {
	if (approxTwoValued()) {
		auto ft = _functable->materialize();
		if (ft) {
			funcTable(ft);
		}
	} else {
		_graphinter->materialize();
	}
}

void FuncInter::add(const ElementTuple& tuple, bool ignoresortchecks){
	if(_functable!=NULL){
		_functable->add(tuple, ignoresortchecks);
	}else{
		_graphinter->makeTrueAtLeast(tuple, ignoresortchecks);
	}
}

bool FuncInter::isConsistent() const {
	if (_functable != NULL) {
		return true;
	} else {
		Assert(_graphinter != NULL);
		return _graphinter->isConsistent();
	}
}

std::set<ElementTuple> emptyset;
const std::set<ElementTuple>& FuncInter::getInconsistentAtoms() const{
	if(isConsistent()){
		return emptyset;
	}
	return _graphinter->getInconsistentAtoms();
}

FuncInter* FuncInter::clone() const {
	return clone(universe());
}

FuncInter* FuncInter::clone(const Universe& univ) const {
	if (_functable) {
		FuncTable* nft = new FuncTable(_functable->internTable(), univ);
		return new FuncInter(nft);
	} else {
		PredInter* npi = _graphinter->clone(univ);
		return new FuncInter(npi);
	}
}

void FuncInter::put(std::ostream& stream) const {
	if (approxTwoValued()) {
		_functable->put(stream);
	} else {
		_graphinter->put(stream);
	}
}

FuncInter* ConstructorFuncInterGenerator::get(const Structure* structure){
	return new FuncInter(new FuncTable(new UNAInternalFuncTable(_function),structure->universe(_function)));
}

FuncInter* MinInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1, st));
	ElementTuple t;
	Tuple2Elem ef;
	ef[t] = st->first();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift, univ);
	return new FuncInter(ft);
}

FuncInter* MaxInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1, st));
	ElementTuple t;
	Tuple2Elem ef;
	ef[t] = st->last();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift, univ);
	return new FuncInter(ft);
}

FuncInter* SuccInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2, st);
	throw notyetimplemented("successor function");
}

FuncInter* InvSuccInterGenerator::get(const Structure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2, st);
	throw notyetimplemented("successor function");
}

MinInterGenerator* MinInterGeneratorGenerator::get(const vector<Sort*>& sorts) {
	return new MinInterGenerator(sorts[0]);
}

MaxInterGenerator* MaxInterGeneratorGenerator::get(const vector<Sort*>& sorts) {
	return new MaxInterGenerator(sorts[0]);
}

SuccInterGenerator* SuccInterGeneratorGenerator::get(const vector<Sort*>& sorts) {
	return new SuccInterGenerator(sorts[0]);
}

InvSuccInterGenerator* InvSuccInterGeneratorGenerator::get(const vector<Sort*>& sorts) {
	return new InvSuccInterGenerator(sorts[0]);
}

/*****************
 TableUtils
 *****************/

namespace TableUtils {

PredInter* leastPredInter(const Universe& univ) {
	PredTable* t1 = new PredTable(new EnumeratedInternalPredTable(), univ);
	PredTable* t2 = new PredTable(new EnumeratedInternalPredTable(), univ);
	return new PredInter(t1, t2, true, true);
}

FuncInter* leastFuncInter(const Universe& univ) {
	PredInter* pt = leastPredInter(univ);
	return new FuncInter(pt);
}

Universe fullUniverse(unsigned int arity) {
	SortTable* tmp = new SortTable(new FullInternalSortTable());
	vector<SortTable*> vst(arity, tmp);
	return Universe(vst);
}

// Only if no shared tuples!
bool isInverse(const PredTable* pt1, const PredTable* pt2) {
	auto univsize = pt1->universe().size();
	auto pt1size = pt1->size();
	auto pt2size = pt2->size();
	if (univsize._type == TST_EXACT && pt1size._type == TST_EXACT && pt2size._type == TST_EXACT) {
		return pt1size._size + pt2size._size == univsize._size;
	} else {
		return false;
	}
}

bool approxTotalityCheck(const FuncInter* funcinter) {
	vector<SortTable*> vst = funcinter->universe().tables();
	vst.pop_back();
	tablesize nroftuples = Universe(vst).size();
	tablesize nrofvalues = funcinter->graphInter()->ct()->size();
	if (nroftuples._type == TST_EXACT && nrofvalues._type == TST_EXACT) {
		return nroftuples._size == nrofvalues._size;
	} else {
		return false;
	}
}

} /* namespace TableUtils */

/*****************
 Structures
 *****************/

/** Destructor **/

bool needFixedNumberOfModels() {
	auto expected = getOption(IntType::NBMODELS);
	return expected != 0 && expected < getMaxElem<int>();
}

bool needMoreModels(unsigned int found) {
	auto expected = getOption(IntType::NBMODELS);
	return expected == 0 || (needFixedNumberOfModels() && found < (unsigned int) expected);
}

// @PRE: consistent structure
// @POST: consistent, more precise, structure
void generateMorePreciseStructures(const PredTable* cf, const ElementTuple& domainElementWithoutValue, const SortTable* imageSort, Function* function,
		vector<Structure*>& extensions, int prevcount) {
// go over all saved structures and generate a new structure for each possible value for it
	auto imageIterator = imageSort->sortBegin();
	vector<Structure*> partialfalsestructs;
	if (function->partial()) {
		for (auto j = extensions.begin(); j < extensions.end(); ++j) {
			CHECKTERMINATION;
			partialfalsestructs.push_back((*j)->clone());
		}
	}

	vector<Structure*> newstructs;
	for (; not imageIterator.isAtEnd(); ++imageIterator) {
		CHECKTERMINATION;
		ElementTuple tuple(domainElementWithoutValue);
		tuple.push_back(*imageIterator);
		if (cf->contains(tuple)) {
			continue;
		}

		for (auto j = extensions.begin(); j < extensions.end() && needMoreModels(partialfalsestructs.size()+newstructs.size()+prevcount); ++j) {
			CHECKTERMINATION;
			auto news = (*j)->clone();
			news->inter(function)->graphInter()->makeTrueExactly(tuple);
			news->clean();
			newstructs.push_back(news);
		}
		for (auto j = partialfalsestructs.begin(); j < partialfalsestructs.end(); ++j) {
			CHECKTERMINATION;
			(*j)->inter(function)->graphInter()->makeFalseExactly(tuple);
		}
	}
	deleteList(extensions);
	extensions = newstructs;
	insertAtEnd(extensions, partialfalsestructs);
	Assert(extensions.size()>0);
}

std::vector<Structure*> generateEnoughTwoValuedExtensions(Structure* original, int prevcount = 0);

// Contents ownership to receiver
std::vector<Structure*> generateEnoughTwoValuedExtensions(const std::vector<Structure*>& partialstructures) {
	auto result = std::vector<Structure*>();
	for (auto structure : partialstructures) {
		if(not needMoreModels(result.size())){
			break;
		}
		auto extensions = generateEnoughTwoValuedExtensions(structure,result.size());
		insertAtEnd(result, extensions);
	}

	if (getOption(IntType::VERBOSE_SOLVING) > 1 && getOption(IntType::NBMODELS) != 0 && needMoreModels(result.size())) {
		stringstream ss;
		ss << "Only " << result.size() << " models exist, although " << getOption(IntType::NBMODELS) << " were requested.\n";
		Warning::warning(ss.str());
	}

	return result;
}

/*
 * Can only be called if this structure is cleaned; calculates all more precise two-valued structures
 * TODO refactor into clean methods!
 */
std::vector<Structure*> generateEnoughTwoValuedExtensions(Structure* original, int prevcount) {
	if(original->approxTwoValued()){
		return {original};
	}

	if (not original->isConsistent()) {
		throw IdpException("Cannot generate two-valued extensions of a four-valued (inconsistent) structure.");
	}

	vector<Structure*> extensions = {original->clone()};

	for (auto f2inter : original->getFuncInters()) {
		CHECKTERMINATION;
		if(not needMoreModels(extensions.size()+prevcount)){
			break;
		}
		auto function = f2inter.first;
		auto inter = f2inter.second;
		if (inter->approxTwoValued()) {
			continue;
		}
		// create a generator for the interpretation
		auto universe = inter->graphInter()->universe();
		const auto& sorts = universe.tables();

		vector<SortIterator> domainIterators;
		bool allempty = true;
		for (auto sort : sorts) {
			CHECKTERMINATION;
			const auto& temp = SortIterator(sort->internTable()->sortBegin());
			domainIterators.push_back(temp);
			if (not temp.isAtEnd()) {
				allempty = false;
			}
		}
		domainIterators.pop_back();

		auto ct = inter->graphInter()->ct();
		auto cf = inter->graphInter()->cf();

		//Now, choose an image for this domainelement
		ElementTuple domainElementWithoutValue;
		if (sorts.size() > 0) {
			auto internaliterator = new CartesianInternalTableIterator(domainIterators, domainIterators, not allempty);
			TableIterator domainIterator(internaliterator);

			auto ctIterator = ct->begin();
			FirstNElementsEqual eq(function->arity());
			StrictWeakNTupleOrdering so(function->arity());

			for (; not allempty && not domainIterator.isAtEnd() && needMoreModels(extensions.size()+prevcount); ++domainIterator) {
				CHECKTERMINATION
				// get unassigned domain element
				domainElementWithoutValue = *domainIterator;
				while (not ctIterator.isAtEnd() && so(*ctIterator, domainElementWithoutValue)) {
					++ctIterator;
				}
				if (not ctIterator.isAtEnd() && eq(domainElementWithoutValue, *ctIterator)) {
					continue;
				}
				generateMorePreciseStructures(cf, domainElementWithoutValue, sorts.back(), function, extensions, prevcount);
			}
		} else {
			generateMorePreciseStructures(cf, domainElementWithoutValue, sorts.back(), function, extensions, prevcount);
		}
	}

//If some predicate is not two-valued, calculate all structures that are more precise in which this function is two-valued
	for (auto i = original->getPredInters().cbegin(); i != original->getPredInters().end() && needMoreModels(extensions.size()+prevcount); i++) {
		CHECKTERMINATION;
		auto pred = (*i).first;
		auto inter = (*i).second;
		Assert(inter!=NULL);
		if (inter->approxTwoValued()) {
			continue;
		}

		auto pf = inter->pf();
		for (auto ptIterator = inter->pt()->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION;
			if(not needMoreModels(extensions.size()+prevcount)){
				break;
			}
			if(not pf->contains(*ptIterator)) {
				continue;
			}

			vector<Structure*> newstructs;
			for (auto ext : extensions) {
				CHECKTERMINATION;
				if(not needMoreModels(newstructs.size()+prevcount)){
					break;
				}
				auto news = ext->clone();
				news->inter(pred)->makeTrueExactly(*ptIterator);
				newstructs.push_back(news);
				if (not needMoreModels(newstructs.size()+prevcount)) {
					break;
				}
				news = ext->clone();
				news->inter(pred)->makeFalseExactly(*ptIterator);
				newstructs.push_back(news);
			}
			deleteList(extensions);
			extensions = newstructs;
		}
	}

	if (needFixedNumberOfModels()) {
		// In this case, not all structures might be two-valued, but are certainly extendable, so just choose a value for each of their elements
		for (auto ext : extensions) {
			ext->makeTwoValued();
		}
	}

	for (auto ext : extensions) {
		ext->clean();
	}

// TODO delete all structures which were cloned and discarded

	return extensions;
}

/**************
 Visitor
 **************/

void ProcInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void BDDInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void FullInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void FuncInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void UnionInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void EnumeratedInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void EqualInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void StrLessInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void StrGreaterInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void FullInternalSortTable::accept(StructureVisitor*) const {
	throw notyetimplemented("Visiting the universal table");
}
void InverseInternalPredTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void UnionInternalSortTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AllNaturalNumbers::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AllIntegers::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AllFloats::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AllChars::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AllStrings::accept(StructureVisitor* v) const {
	v->visit(this);
}
void EnumeratedInternalSortTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void IntRangeInternalSortTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void ConstructedInternalSortTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void ProcInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void UNAInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void EnumeratedInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void PlusInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void MinusInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void TimesInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void DivInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void AbsInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void UminInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void ExpInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}
void ModInternalFuncTable::accept(StructureVisitor* v) const {
	v->visit(this);
}

/******************
 Materialize
 ******************/

SortTable* SortTable::materialize() const {
	EnumerateSymbolicTable m;
	return m.run(this);
}
PredTable* PredTable::materialize() const {
	EnumerateSymbolicTable m;
	return m.run(this);
}
FuncTable* FuncTable::materialize() const {
	EnumerateSymbolicTable m;
	return m.run(this);
}

bool isConsistentWith(PredTable* table, PredInter* inter){
	auto cf = inter->cf();
	if(not intersectionEmpty(table, cf)){
		return false;
	}

	auto ct = inter->ct();
	for(auto i = ct->begin(); not i.isAtEnd(); ++i){
		if(not table->contains(*i)){
			return false;
		}
	}

	return true;
}

bool isConsistentWith(PredInter* inter, PredInter* inter2){
	auto ct = inter->ct();
	auto cf = inter->cf();
	auto ct2 = inter2->ct();
	auto cf2 = inter2->cf();

	if(not (intersectionEmpty(ct, cf2) and
			intersectionEmpty(ct2, cf))){
		return false;
	}

	return true;
}

bool intersectionEmpty(PredTable* left, PredTable* right){
	if(right->size()<left->size()){
		auto temp = left;
		left = right;
		right = temp;
	}

	for(auto i = left->begin(); not i.isAtEnd(); ++i){
		if(right->contains(*i)){
			return false;
		}
	}
	return true;
}
