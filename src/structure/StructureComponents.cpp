/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

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

ostream& operator<<(ostream& out, const DomainElementType& domeltype) {
	string DomElTypeStrings[4] = { "int", "double", "string", "compound" };
	return out << DomElTypeStrings[domeltype];
}

DomainElement::DomainElement()
		: 	_type(DET_INT),
			_value(1) {
}

/**
 *	Constructor for domain elements that are integers
 */
DomainElement::DomainElement(int value)
		: 	_type(DET_INT),
			_value(value) {
}

/**
 *	Constructor for domain elements that are floating point numbers but not integers
 */
DomainElement::DomainElement(double value)
		: 	_type(DET_DOUBLE),
			_value(value) {
	Assert(not isInt(value));
	// TODO check rest of code for such errors
}

/**
 *	Constructor for domain elements that are strings but not floating point numbers
 */
DomainElement::DomainElement(const string* value)
		: 	_type(DET_STRING),
			_value(value) {
	Assert(not isDouble(*value));
}

/**
 *	Constructor for domain elements that are compounds
 */
DomainElement::DomainElement(const Compound* value)
		: 	_type(DET_COMPOUND),
			_value(value) {
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
		output << *(_value._string);
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

ostream& operator<<(ostream& output, const DomainElement& d) {
	return d.put(output);
}

ostream& operator<<(ostream& output, const ElementTuple& tuple) {
	output << '(';
	for (auto it = tuple.cbegin(); it != tuple.cend(); ++it) {
		output << **it;
		if (it != tuple.cend() - 1) {
			output << ',';
		}
	}
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
		: 	_function(function),
			_arguments(arguments) {
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

string Compound::toString() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}

ostream& operator<<(ostream& output, const Compound& c) {
	return c.put(output);
}

/**
 * \brief Comparison of two compound domain element values
 */
bool operator<(const Compound& c1, const Compound& c2) {
	if (c1.function() < c2.function())
		return true;
	else if (c1.function() > c2.function())
		return false;
	else {
		for (unsigned int n = 0; n < c1.function()->arity(); ++n) {
			if (c1.arg(n) < c2.arg(n))
				return true;
			else if (c1.arg(n) > c2.arg(n))
				return false;
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

ostream& DomainAtom::put(ostream& output) const {
	output << *_symbol;
	if (typeid(*_symbol) == typeid(Predicate)) {
		if (!_symbol->sorts().empty()) {
			output << '(' << *_args[0];
			for (size_t n = 1; n < _args.size(); ++n) {
				output << ',' << *_args[n];
			}
			output << ')';
		}
	} else {
		Function* f = dynamic_cast<Function*>(_symbol);
		if (f->arity() > 0) {
			output << '(' << *_args[0];
			for (size_t n = 1; n < f->arity(); ++n) {
				output << ',' << *_args[n];
			}
			output << ") = " << *(_args.back());
		}
	}
	return output;
}

string DomainAtom::toString() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}

bool isFinite(const tablesize& tsize) {
	return tsize._type == TST_EXACT || tsize._type == TST_APPROXIMATED;
}

DomainAtomFactory::~DomainAtomFactory() {
	for (auto it = _atoms.cbegin(); it != _atoms.cend(); ++it) {
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			delete (jt->second);
		}
	}
}

DomainAtomFactory* DomainAtomFactory::_instance = NULL;

DomainAtomFactory* DomainAtomFactory::instance() {
	if (not _instance) {
		_instance = new DomainAtomFactory();
	}
	return _instance;
}

const DomainAtom* DomainAtomFactory::create(PFSymbol* s, const ElementTuple& tuple) {
	map<PFSymbol*, map<ElementTuple, DomainAtom*> >::const_iterator it = _atoms.find(s);
	if (it != _atoms.cend()) {
		map<ElementTuple, DomainAtom*>::const_iterator jt = it->second.find(tuple);
		if (jt != it->second.cend()) {
			return jt->second;
		}
	}
	DomainAtom* newatom = new DomainAtom(s, tuple);
	_atoms[s][tuple] = newatom;
	return newatom;
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
		: 	_iterators(vsi),
			_lowest(low),
			_hasNext(h) {
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
	return new CartesianInternalTableIterator(_iterators, _lowest, _hasNext);
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
		: 	_generator(generator),
			_vars(vars) {
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
		: 	_univ(univ),
			_predicate(p) {
	vector<SortIterator> vsi1;
	vector<SortIterator> vsi2;
	for (unsigned int n = 0; n < univ.arity(); ++n) {
		vsi1.push_back(univ.tables()[n]->sortBegin());
		vsi2.push_back(univ.tables()[n]->sortBegin());
	}
	_curr = TableIterator(new CartesianInternalTableIterator(vsi1, vsi2, true));
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
		: 	_iterators(its),
			_universe(univ),
			_outtables(outs) {
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
		: 	_curr(its),
			_lowest(its),
			_universe(univ),
			_outtable(out),
			_end(false),
			_currtuple(its.size()) {
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
		: 	_curr(curr),
			_lowest(low),
			_universe(univ),
			_outtable(out),
			_end(end),
			_currtuple(curr.size()) {
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
		: 	_curr(its),
			_lowest(its),
			_function(f),
			_end(false),
			_currtuple(its.size()) {
	for (unsigned int n = 0; n < _curr.size(); ++n) {
		if (_curr[n].isAtEnd()) {
			_end = true;
			break;
		}
		_currtuple[n] = *(_curr[n]);
	}
}

UNAInternalIterator::UNAInternalIterator(const vector<SortIterator>& curr, const vector<SortIterator>& low, Function* f, bool end)
		: 	_curr(curr),
			_lowest(low),
			_function(f),
			_end(end),
			_currtuple(curr.size()) {
	for (size_t n = 0; n < _curr.size(); ++n) {
		if (not _curr[n].isAtEnd()) {
			_currtuple[n] = *(_curr[n]);
		}
	}
}

UNAInternalIterator* UNAInternalIterator::clone() const {
	return new UNAInternalIterator(_curr, _lowest, _function, _end);
}

bool UNAInternalIterator::hasNext() const {
	return not _end;
}

const ElementTuple& UNAInternalIterator::operator*() const {
	_deref.push_back(_currtuple);
	_deref.back().push_back(createDomElem(_function, _deref.back()));
	return _deref.back();
}

void UNAInternalIterator::operator++() {
	int pos = _curr.size() - 1;
	for (auto it = _curr.rbegin(); it != _curr.rend(); ++it, --pos) {
		Assert(not it->isAtEnd());
		++(*it);
		if (not it->isAtEnd()) {
			_currtuple[pos] = *(*it);
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
		: 	_iterators(vsi),
			_outtables(tabs) {
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
	return createDomElem(StringPointer(_iter));
}

const DomainElement* CharInternalSortIterator::operator*() const {
	if ('0' <= _iter && _iter <= '9') {
		int i = _iter - '0';
		return createDomElem(i);
	} else {
		string* s = StringPointer(string(1, _iter));
		return createDomElem(s);
	}
}

void CharInternalSortIterator::operator++() {
	if (_iter == getMaxElem<char>()) {
		_end = true;
	} else {
		++_iter;
	}
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
		: 	InternalPredTable(),
			_table(table),
			_linked(linked) {
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
		: 	InternalPredTable(),
			_intables(intabs),
			_outtables(outtabs) {
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
		case TST_UNKNOWN:
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
		case TST_UNKNOWN:
		case TST_INFINITE:
			type = TST_UNKNOWN;
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
		notyetimplemented("Exact finiteness test on union predicate tables");
		return approxFinite(univ);
	}
}

bool UnionInternalPredTable::empty(const Universe& univ) const {
	if (approxEmpty(univ)) {
		return true;
	} else {
		notyetimplemented("Exact emptyness test on union predicate tables");
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

BDDInternalPredTable::BDDInternalPredTable(const FOBDD* bdd, FOBDDManager* manager, const vector<Variable*>& vars, const AbstractStructure* str)
		: 	_manager(manager),
			_bdd(bdd),
			_vars(vars),
			_structure(str) {
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
		set<const FOBDDDeBruijnIndex*> indices;
		set<Variable*> fovars;
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
	auto otherbdd = _manager->getBDD(bipt->bdd(),bipt->manager());
	if(_bdd == otherbdd){
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
	map<const FOBDDVariable*, const FOBDDVariable*> otherToThisVars;
	auto othervars = bipt->vars();
	if(othervars.size() != vars().size()){
		return false;
	}

	//Set the bdds to the same variables
	auto othervar = othervars.cbegin();
	for(auto var: vars()){
		auto varone = _manager->getVariable(*othervar);
		auto vartwo = _manager->getVariable(var);
		otherToThisVars[varone] = vartwo;
		othervar++;
	}
	auto otherbdd = _manager->getBDD(bipt->bdd(), bipt->manager());
	otherbdd = _manager->substitute(otherbdd,otherToThisVars);
	if (_bdd == _manager->negation(otherbdd)) {
		return true;
	}

	//Two tables are inverse if their intersection is empty (conjunction is false)
	//And their unioin is everything (disjunction is true)
	auto conj = _manager->conjunction(_bdd, otherbdd);
	auto disj = _manager->disjunction(_bdd, otherbdd);
	if(_manager->isFalsebdd(conj) && _manager->isTruebdd(disj)){
		return true;
	}
	return false;
}



tablesize BDDInternalPredTable::size(const Universe&) const {
	set<const FOBDDDeBruijnIndex*> indices;
	set<Variable*> fovars;
	fovars.insert(_vars.cbegin(), _vars.cend());
	auto bddvars = _manager->getVariables(fovars);
	double estimate = BddStatistics::estimateNrAnswers(_bdd, bddvars, indices, _structure, _manager);
	if (estimate < getMaxElem<double>()) {
		unsigned int es = estimate;
		return tablesize(TST_APPROXIMATED, es);
	} else
		return tablesize(TST_UNKNOWN, 0);
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
	BDDInternalPredTable* temporary = new BDDInternalPredTable(_bdd, _manager, _vars, _structure);
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
	if (not contains(d)) {
		return this;
	}
	if (d->type() == DET_INT) {
		if (d->value()._int == _first) {
			if (_nrRefs < 2) {
				_first = _first + 1;
				return this;
			}
		} else if (d->value()._int == _last) {
			if (_nrRefs < 2) {
				_last = _last - 1;
				return this;
			}
		}
	}
	auto eist = new EnumeratedInternalSortTable();
	InternalSortTable* ist = eist;
	for (int n = _first; n <= _last; ++n) {
		ist = ist->add(createDomElem(n));
	}
	ist = ist->remove(d);
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
	size_t result = 0;
	TableSizeType type = TST_APPROXIMATED;
	for (auto it = _intables.cbegin(); it != _intables.cend(); ++it) {
		tablesize tp = (*it)->size();
		switch (tp._type) {
		case TST_APPROXIMATED:
		case TST_EXACT:
			result += tp._size;
			break;
		case TST_UNKNOWN:
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
		case TST_UNKNOWN:
		case TST_INFINITE:
			type = TST_UNKNOWN;
			break;
		}
	}
	return tablesize(type, result);
}

bool UnionInternalSortTable::finite() const {
	if (approxFinite()) {
		return true;
	} else {
		notyetimplemented("Exact finiteness test on union sort tables");
		return approxFinite();
	}
}

bool UnionInternalSortTable::empty() const {
	if (approxEmpty()) {
		return true;
	} else {
		notyetimplemented("Exact emptyness test on union sort tables");
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
			InternalSortTable* temp = newtable->add(d);
			Assert(temp == newtable);
			return newtable;
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
		InternalSortTable* temp = newtable->remove(d);
		Assert(temp == newtable);
		return newtable;
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
	notyetimplemented("intermediate sortiterator for UnionInternalSortTable");
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
		notyetimplemented("Computation of last element of a UnionInternalSortTable");
	}
	return result;
}

bool UnionInternalSortTable::isRange() const {
	notyetimplemented("Exact range test of a UnionInternalSortTable");
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
	return createDomElem(StringPointer(""));
}

const DomainElement* AllStrings::last() const {
	notyetimplemented("impossible to get the largest string");
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
				string* s = StringPointer(string(1, c));
				ist->add(createDomElem(s));
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
				string* s = StringPointer(string(1, c));
				ist->add(createDomElem(s));
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
	return createDomElem(StringPointer(string(1, getMinElem<char>())));
}

const DomainElement* AllChars::last() const {
	return createDomElem(StringPointer(string(1, getMaxElem<char>())));
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

bool InternalFuncTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	ElementTuple input = tuple;
	const DomainElement* output = tuple.back();
	input.pop_back();
	const DomainElement* computedoutput = operator[](input);
	if (output == computedoutput) {
		return univ.contains(tuple);
	} else {
		return false;
	}
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
	tablesize univsize = univ.size();
	TableSizeType tst = univsize._type;
	tablesize outsize = univ.tables().back()->size();
	if (outsize._size != 0) {
		if (tst == TST_EXACT) {
			tst = TST_APPROXIMATED;
		}
		return tablesize(tst, univsize._size / outsize._size);
	} else {
		if (outsize._type == TST_EXACT) {
			return tablesize(TST_EXACT, 0);
		} else {
			return tablesize(TST_UNKNOWN, 0);
		}
	}
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
	notyetimplemented("Exact emptyness test on procedural function tables");
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
	notyetimplemented("adding a tuple to a procedural function interpretation");
	return this;
}

InternalFuncTable* ProcInternalFuncTable::remove(const ElementTuple&) {
	notyetimplemented("removing a tuple from a procedural function interpretation");
	return this;
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
	notyetimplemented("Exact emptyness test on constructor function tables");
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
	vector<SortTable*> vst = univ.tables();
	vst.pop_back();
	Universe newuniv(vst);
	return newuniv.size();
}

const DomainElement* UNAInternalFuncTable::operator[](const ElementTuple& tuple) const {
	return createDomElem(_function, tuple);
}

InternalFuncTable* UNAInternalFuncTable::add(const ElementTuple&) {
	notyetimplemented("adding a tuple to a generated function interpretation");
	return this;
}

InternalFuncTable* UNAInternalFuncTable::remove(const ElementTuple&) {
	notyetimplemented("removing a tuple from a generated function interpretation");
	return this;
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

	Assert(computedvalue == mappedvalue);
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
		stream << toString(it->first) << "->" << (toString(it->second));
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
	stream << toString(_table);
}

void SortTable::put(std::ostream& stream) const {
	if (empty()) {
		stream << toString(_table) << " is empty";
	} else {
		stream << toString(_table) << "[" << toString(first()) << ", " << toString(last()) << "]";
	}
}

/****************
 PredTable
 ****************/

PredTable::PredTable(InternalPredTable* table, const Universe& univ)
		: 	_table(NULL),
			_universe(univ) {
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

void PredTable::add(const ElementTuple& tuple) {
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
	notyetimplemented("Adding a tuple to a procedural predicate table");
	return this;
}

InternalPredTable* ProcInternalPredTable::remove(const ElementTuple&) {
	notyetimplemented("Removing a tuple from a procedural predicate table");
	return this;
}

InternalTableIterator* ProcInternalPredTable::begin(const Universe& univ) const {
	return new ProcInternalTableIterator(this, univ);
}

InverseInternalPredTable::InverseInternalPredTable(InternalPredTable* inv)
		: 	InternalPredTable(),
			_invtable(inv) {
	inv->incrementRef();
}

InverseInternalPredTable::~InverseInternalPredTable() {
	_invtable->decrementRef();
}

void InverseInternalPredTable::internTable(InternalPredTable* ipt) {
	ipt->incrementRef();
	_invtable->decrementRef();
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
		notyetimplemented("Exact finiteness test on inverse predicate tables");
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
		TableIterator ti = TableIterator(begin(univ));
		return ti.isAtEnd();
	} else {
		notyetimplemented("Exact emptyness test on inverse predicate tables");
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
	if (univsize._type == TST_UNKNOWN) {
		return univsize;
	} else if (univsize._type == TST_INFINITE) {
		if (invsize._type == TST_APPROXIMATED || invsize._type == TST_EXACT) {
			return tablesize(TST_INFINITE, 0);
		} else {
			return tablesize(TST_UNKNOWN, 0);
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
		return tablesize(TST_UNKNOWN, 0);
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
		InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable);
		InternalPredTable* temp = newtable->add(tuple);
		Assert(temp == newtable);
		return newtable;
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
		InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable);
		InternalPredTable* temp = newtable->remove(tuple);
		Assert(temp == newtable);
		return newtable;
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

void SortTable::add(const ElementTuple& tuple) {
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
		: 	_table(table),
			_universe(univ) {
	_table->incrementRef();
}

FuncTable::~FuncTable() {
	_table->decrementRef();
}

void FuncTable::add(const ElementTuple& tuple) {
	if (_table->contains(tuple, _universe)) {
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
	if (not _table->contains(tuple, _universe)) {
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

/**
 * \brief Create a three- or four-valued interpretation
 *
 * PARAMETERS
 *	- ctpf	: the certainly true or possibly false tuples
 *	- cfpt	: the certainly false or possibly true tuples
 *	- ct	: if true (false), ctpf stores the certainly true (possibly false) tuples
 *	- cf	: if true (false), cfpt stores the certainly false (possibly true) tuples
 *	- univ	: all possible domain elements of the sorts of the columns of the table
 */
PredInter::PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf) {
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->internTable()), ctpf->universe());
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->internTable()), ctpf->universe());
	_inconsistentElements = {};
	if (ct) {
		_ct = ctpf;
		_pf = inverseCtpf;
	} else {
		_pf = ctpf;
		_ct = inverseCtpf;
	}
	if (cf) {
		_cf = cfpt;
		_pt = inverseCfpt;
	} else {
		_pt = cfpt;
		_cf = inverseCfpt;
	}
	checkConsistency();
}

/**
 * \brief Create a two-valued interpretation
 *
 * PARAMETERS
 *	- ctpf	: the certainly true or possibly false tuples
 *	- ct	: if true (false), ctpf stores the certainly true (possibly false) tuples
 *	- univ	: all possible domain elements of the sorts of the columns of the table
 */
PredInter::PredInter(PredTable* ctpf, bool ct) {
	PredTable* cfpt = new PredTable(ctpf->internTable(), ctpf->universe());
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->internTable()), ctpf->universe());
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->internTable()), cfpt->universe());
	_inconsistentElements = {};
	if (ct) {
		_ct = ctpf;
		_pt = cfpt;
		_cf = inverseCtpf;
		_pf = inverseCfpt;
	} else {
		_pf = ctpf;
		_cf = cfpt;
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

/**
 * \brief Returns true iff the tuple is true or inconsistent according to the predicate interpretation
 */
bool PredInter::isTrue(const ElementTuple& tuple) const {
	return _ct->contains(tuple);
}

/**
 * \brief Returns true iff the tuple is false or inconsistent according to the predicate interpretation
 */
bool PredInter::isFalse(const ElementTuple& tuple) const {
	if (not _cf->contains(tuple)) {
		return (not _cf->universe().contains(tuple));
	}
	return true;
}

/**
 * \brief Returns true iff the tuple is unknown according to the predicate interpretation
 */
bool PredInter::isUnknown(const ElementTuple& tuple) const {
	if (approxTwoValued()) {
		return false;
	} else {
		return not (isFalse(tuple) || isTrue(tuple));
	}
}

/**
 * \brief Returns true iff the tuple is inconsistent according to the predicate interpretation
 */
bool PredInter::isInconsistent(const ElementTuple& tuple) const {
	return _inconsistentElements.find(&tuple) != _inconsistentElements.cend();
}

bool PredInter::isConsistent() const {
	return _inconsistentElements.size() == 0;
}

void PredInter::checkConsistency() {
	_inconsistentElements.clear();
	if(_ct->approxInverse(_cf)){
		return;
	}
	if (not _ct->approxFinite() || not _cf->approxFinite()) {
		throw notyetimplemented("Check consistency of infinite tables");
	}
	auto smallest = _ct->size()._size < _cf->size()._size ? _ct : _cf; // Walk over the smallest table first => also optimal behavior in case one is emtpy
	auto largest = (smallest == _ct) ? _cf : _ct;

	auto smallIt = smallest->begin();
	auto largeIt = largest->begin();

	auto sPossTable = smallest == _ct ? _pt : _pf;
	auto lPossTable = smallest == _ct ? _pf : _pt;

	FirstNElementsEqual eq(smallest->arity());
	StrictWeakNTupleOrdering so(smallest->arity());
	for (; not smallIt.isAtEnd(); ++smallIt) {
		CHECKTERMINATION
		// get unassigned domain element
while		(not largeIt.isAtEnd() && so(*largeIt, *smallIt)) {
			CHECKTERMINATION;
			Assert(sPossTable->size()._size > 1000 || not sPossTable->contains(*largeIt));
			// NOTE: checking pt and pf can be very expensive in large domains, so the debugging check is only done for small domains
			//Should always be true...
			++largeIt;
		}
		if (not largeIt.isAtEnd() && eq(*largeIt, *smallIt)) {
			_inconsistentElements.insert(&(*largeIt));
		}
		Assert(lPossTable->size()._size>1000 || not lPossTable->contains(*smallIt));
		// NOTE: checking pt and pf can be very expensive in large domains, so the debugging check is only done for small domains
		//Should always be true...
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

void PredInter::makeUnknown(const ElementTuple& tuple) {
	if (_inconsistentElements.find(&tuple) != _inconsistentElements.cend()) {
		_inconsistentElements.erase(&tuple);
	}
	moveTupleFromTo(tuple, _cf, _pt);
	moveTupleFromTo(tuple, _ct, _pf);
}

void PredInter::makeTrue(const ElementTuple& tuple) {
	if (_cf->contains(tuple)) {
		_inconsistentElements.insert(&tuple);
	}
	moveTupleFromTo(tuple, _pf, _ct);
}

void PredInter::makeFalse(const ElementTuple& tuple) {
	if (_ct->contains(tuple)) {
		_inconsistentElements.insert(&tuple);
	}
	moveTupleFromTo(tuple, _pt, _cf);
}

void PredInter::moveTupleFromTo(const ElementTuple& tuple, PredTable* from, PredTable* to) {
	if (tuple.size() != universe().arity()) {
		stringstream ss;
		ss << "Adding a tuple of size " << tuple.size() << " to a predicate with arity " << universe().arity();
		throw IdpException(ss.str());
	}
	auto table = universe().tables().cbegin();
	auto elem = tuple.cbegin();
	for (; table < universe().tables().cend(); ++table, ++elem) {
		if (not (*table)->contains(*elem)) {
			stringstream ss;
			ss << "Element " << toString(*elem) << " is not part of table " << toString(*table) << ", but you are trying to assign it to such a table";
			throw IdpException(ss.str());
		}
	}
	if (isa<InverseInternalPredTable>(*(from->internTable()))) {
		to->internTable()->decrementRef();
		auto old = to->internTable();
		to->add(tuple);
		old->incrementRef();
		if (to->internTable() != old) {
			auto internpf = dynamic_cast<InverseInternalPredTable*>(from->internTable());
			internpf->internTable(to->internTable());
		}
	} else {
		from->internTable()->decrementRef();
		auto old = from->internTable();
		from->remove(tuple);
		old->incrementRef();
		if (from->internTable() != old) {
			auto internct = dynamic_cast<InverseInternalPredTable*>(to->internTable());
			internct->internTable(from->internTable());
		}
	}
}

void PredInter::ct(PredTable* t) {
	delete (_ct);
	delete (_pf);
	_ct = t;
	_pf = new PredTable(new InverseInternalPredTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::cf(PredTable* t) {
	delete (_cf);
	delete (_pt);
	_cf = t;
	_pt = new PredTable(new InverseInternalPredTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::pt(PredTable* t) {
	delete (_pt);
	delete (_cf);
	_pt = t;
	_cf = new PredTable(new InverseInternalPredTable(t->internTable()), t->universe());
	checkConsistency();
}

void PredInter::pf(PredTable* t) {
	delete (_pf);
	delete (_ct);
	_pf = t;
	_ct = new PredTable(new InverseInternalPredTable(t->internTable()), t->universe());
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
	_pf = new PredTable(new InverseInternalPredTable(_ct->internTable()), _ct->universe());
	_cf = new PredTable(new InverseInternalPredTable(_ct->internTable()), _ct->universe());
}

void PredInter::materialize() {
	if (approxTwoValued()) {
		auto prt = _ct->materialize();
		if (prt) {
			ctpt(prt);
		}
	} else {
		auto prt = _ct->materialize();
		if (prt) {
			ct(prt);
		}
		auto prf = _cf->materialize();
		if (prf) {
			cf(prf);
		}
	}
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
	auto inverseCfpt = new PredTable(new InverseInternalPredTable(ncfpt->internTable()), univ);
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
	stream << "Certainly true: " << toString(interpretation.ct()) << "\n";
	stream << "Certainly false: " << toString(interpretation.cf()) << "\n";
	stream << "Possibly true: " << toString(interpretation.pt()) << "\n";
	stream << "Possibly false: " << toString(interpretation.pf()) << "\n";
	return stream;
}

PredInter* InconsistentPredInterGenerator::get(const AbstractStructure* structure) {
	auto emptytable = new PredTable(new EnumeratedInternalPredTable(), structure->universe(_predicate));
	return new PredInter(emptytable, emptytable, false, false);
}

// FIXME better way of managing (the memory of) these interpretations?
EqualInterGenerator::~EqualInterGenerator() {
	deleteList(generatedInters);
}
PredInter* EqualInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2, st));
	EqualInternalPredTable* eip = new EqualInternalPredTable();
	PredTable* ct = new PredTable(eip, univ);
	generatedInters.push_back(new PredInter(ct, true));
	return generatedInters.back();
}

StrLessThanInterGenerator::~StrLessThanInterGenerator() {
	deleteList(generatedInters);
}
PredInter* StrLessThanInterGenerator::get(const AbstractStructure* structure) {
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
PredInter* StrGreaterThanInterGenerator::get(const AbstractStructure* structure) {
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
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft, true), ft->universe());
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
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft, true), ft->universe());
	_graphinter = new PredInter(ct, true);
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

bool FuncInter::isConsistent() const {
	if (_functable != NULL) {
		return true;
	} else {
		Assert(_graphinter != NULL);
		return _graphinter->isConsistent();
	}
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

FuncInter* InconsistentFuncInterGenerator::get(const AbstractStructure* structure) {
	auto emptytable = new PredTable(new EnumeratedInternalPredTable(), structure->universe(_function));
	return new FuncInter(new PredInter(emptytable, emptytable, false, false));
}

FuncInter* MinInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1, st));
	ElementTuple t;
	Tuple2Elem ef;
	ef[t] = st->first();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift, univ);
	return new FuncInter(ft);
}

FuncInter* MaxInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1, st));
	ElementTuple t;
	Tuple2Elem ef;
	ef[t] = st->last();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift, univ);
	return new FuncInter(ft);
}

FuncInter* SuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2, st);
	FuncTable* ft = 0; // TODO
	notyetimplemented("successor function");
	return new FuncInter(ft);
}

FuncInter* InvSuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2, st);
	FuncTable* ft = 0; // TODO
	notyetimplemented("successor function");
	return new FuncInter(ft);
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
	vector<SortTable*> vst(arity, get(STDSORT::STRINGSORT)->interpretation());
	return Universe(vst);
}

bool approxIsInverse(const PredTable* pt1, const PredTable* pt2) {
	tablesize univsize = pt1->universe().size();
	tablesize pt1size = pt1->size();
	tablesize pt2size = pt2->size();
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
//clog << "Checking totality of " << *function << " -- nroftuples=" << nroftuples.second << " and nrofvalues=" << nrofvalues.second;
//clog << " (trust=" << (nroftuples.first && nrofvalues.first) << ")" << "\n";
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

void generateMorePreciseStructures(const PredTable* cf, const ElementTuple& domainElementWithoutValue, const SortTable* imageSort, Function* function,
		vector<AbstractStructure*>& extensions) {
	int currentnb = extensions.size();

// go over all saved structures and generate a new structure for each possible value for it
	auto imageIterator = SortIterator(imageSort->internTable()->sortBegin());
	vector<AbstractStructure*> partialfalsestructs;
	if (function->partial()) {
		for (auto j = extensions.begin(); j < extensions.end(); ++j) {
			CHECKTERMINATION;
			partialfalsestructs.push_back((*j)->clone());
		}
	}

	vector<AbstractStructure*> newstructs;
	for (; not imageIterator.isAtEnd(); ++imageIterator) {
		CHECKTERMINATION;
		ElementTuple tuple(domainElementWithoutValue);
		tuple.push_back(*imageIterator);
		if (cf->contains(tuple)) {
			continue;
		}

		for (auto j = extensions.begin(); j < extensions.end() && needMoreModels(currentnb); ++j) {
			CHECKTERMINATION;
			auto news = (*j)->clone();
			news->inter(function)->graphInter()->makeTrue(tuple);
			news->clean();
			newstructs.push_back(news);
			currentnb++;
		}
		for (auto j = partialfalsestructs.begin(); j < partialfalsestructs.end(); ++j) {
			CHECKTERMINATION(*j)->inter(function)->graphInter()->makeFalse(tuple);
		}
	}
	extensions = newstructs;
	extensions.insert(extensions.end(), partialfalsestructs.cbegin(), partialfalsestructs.cend());
	Assert(extensions.size()>0);
}

std::vector<AbstractStructure*> generateEnoughTwoValuedExtensions(AbstractStructure* original);

// Contents ownership to receiver
std::vector<AbstractStructure*> generateEnoughTwoValuedExtensions(const std::vector<AbstractStructure*>& partialstructures) {
	auto result = std::vector<AbstractStructure*>();
	for (auto i = partialstructures.cbegin(); i != partialstructures.cend(); ++i) {
		if (not (*i)->approxTwoValued()) {
			auto extensions = generateEnoughTwoValuedExtensions(*i);
			result.insert(result.end(), extensions.begin(), extensions.end());
		} else {
			result.push_back(*i);
		}
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
std::vector<AbstractStructure*> generateEnoughTwoValuedExtensions(AbstractStructure* original) {
	vector<AbstractStructure*> extensions;

	extensions.push_back(original->clone());

	if (original->approxTwoValued()) {
		return extensions;
	}

	if (not original->isConsistent()) {
		throw IdpException("Cannot generate two-valued extensions of a four-valued (inconsistent) structure.");
	}

// TODO if going through the vocabulary, it is not guaranteed that the struct has an interpretation for it (otherwise, could make this a global method). But is this logical, or should a monitor be added such that a struct is extended if its vocabulary changes?
	for (auto i = original->getFuncInters().cbegin(); i != original->getFuncInters().cend() && needMoreModels(extensions.size()); ++i) {
		CHECKTERMINATION;
		auto function = (*i).first;
		auto inter = (*i).second;
		if (inter->approxTwoValued()) {
			continue;
		}
		// create a generator for the interpretation
		auto universe = inter->graphInter()->universe();
		const auto& sorts = universe.tables();

		vector<SortIterator> domainIterators;
		bool allempty = true;
		for (auto sort = sorts.cbegin(); sort != sorts.cend(); ++sort) {
			CHECKTERMINATION;
			const auto& temp = SortIterator((*sort)->internTable()->sortBegin());
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
			FirstNElementsEqual eq((*i).first->arity());
			StrictWeakNTupleOrdering so((*i).first->arity());

			for (; not allempty && not domainIterator.isAtEnd() && needMoreModels(extensions.size()); ++domainIterator) {
				CHECKTERMINATION
				// get unassigned domain element
domainElementWithoutValue				= *domainIterator;
				while (not ctIterator.isAtEnd() && so(*ctIterator, domainElementWithoutValue)) {
					++ctIterator;
				}
				if (not ctIterator.isAtEnd() && eq(domainElementWithoutValue, *ctIterator)) {
					continue;
				}
				generateMorePreciseStructures(cf, domainElementWithoutValue, sorts.back(), function, extensions);
			}
		} else {
			generateMorePreciseStructures(cf, domainElementWithoutValue, sorts.back(), function, extensions);
		}
	}

//If some predicate is not two-valued, calculate all structures that are more precise in which this function is two-valued
	for (auto i = original->getPredInters().cbegin(); i != original->getPredInters().end() && needMoreModels(extensions.size()); i++) {
		CHECKTERMINATION;
		auto pred = (*i).first;
		auto inter = (*i).second;
		Assert(inter!=NULL);
		if (inter->approxTwoValued()) {
			continue;
		}

		const PredTable* pf = inter->pf();
		for (TableIterator ptIterator = inter->pt()->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION;
			if(not pf->contains(*ptIterator))
			{
				continue;
			}

			vector<AbstractStructure*> newstructs;
			int count = 0;
			for (auto j = extensions.begin(); j < extensions.end() && needMoreModels(count); ++j) {
				CHECKTERMINATION;
				auto news = (*j)->clone();
				news->inter(pred)->makeTrue(*ptIterator);
				newstructs.push_back(news);
				count++;
				if (not needMoreModels(count)) {
					break;
				}
				news = (*j)->clone();
				news->inter(pred)->makeFalse(*ptIterator);
				newstructs.push_back(news);
				count++;
			}
			extensions = newstructs;
		}
	}

	if (needFixedNumberOfModels()) {
		// In this case, not all structures might be two-valued, but are certainly extendable, so just choose a value for each of their elements
		for (auto j = extensions.begin(); j < extensions.end(); ++j) {
			(*j)->makeTwoValued();
		}
	}

	for (auto j = extensions.begin(); j < extensions.end(); ++j) {
		(*j)->clean();
		//Assert((*j)->approxTwoValued()); TODO: place this back and make it NOT approx
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

