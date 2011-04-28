/************************************
	structure.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cmath>
#include <sstream>
#include <iostream>
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
class InternalArgument;
#include "execute.hpp"
#include "error.hpp"
using namespace std;

/**********************
	Domain elements
**********************/

/**
 *	Constructor for domain elements that are integers
 */
DomainElement::DomainElement(int value) : _type(DET_INT) {
	_value._int = value;
}

/**
 *	Constructor for domain elements that are floating point numbers but not integers
 */
DomainElement::DomainElement(double value) : _type(DET_DOUBLE) {
	assert(!isInt(value));
	_value._double = value;
}

/**
 *	Constructor for domain elements that are strings but not floating point numbers
 */
DomainElement::DomainElement(const string* value) : _type(DET_STRING) {
	assert(!isDouble(*value));
	_value._string = value;
}

/**
 *	Constructor for domain elements that are compounds
 */
DomainElement::DomainElement(const Compound* value) : _type(DET_COMPOUND) {
	_value._compound = value;
}


DomainElement::~DomainElement() { 
}

inline DomainElementType DomainElement::type() const {
	return _type;
}

inline DomainElementValue DomainElement::value() const {
	return _value;
}

ostream& DomainElement::put(ostream& output) const {
	switch(_type) {
		case DET_INT:
			output << itos(_value._int);
			break;
		case DET_DOUBLE:
			output << dtos(_value._double);
			break;
		case DET_STRING:
			output << *(_value._string);
			break;
		case DET_COMPOUND:
			_value._compound->put(output);
			break;
		default:
			assert(false); 
	}
	return output;
}

string DomainElement::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}

ostream& operator<<(ostream& output, const DomainElement& d) {
	return d.put(output);
}

bool operator<(const DomainElement& d1, const DomainElement& d2) {
	switch(d1.type()) {
		case DET_INT:
			switch(d2.type()) {
				case DET_INT:
					return d1.value()._int < d2.value()._int;
				case DET_DOUBLE:
					return double(d1.value()._int) < d2.value()._double;
				case DET_STRING:
				case DET_COMPOUND:
					return true;
				default:
					assert(false);
			}
		case DET_DOUBLE:
			switch(d2.type()) {
				case DET_INT:
					return d1.value()._double < double(d2.value()._int);
				case DET_DOUBLE:
					return d1.value()._double < d2.value()._double;
				case DET_STRING:
				case DET_COMPOUND:
					return true;
				default:
					assert(false);
			}
		case DET_STRING:
			switch(d2.type()) {
				case DET_INT:
				case DET_DOUBLE:
					return false;
				case DET_STRING:
					return *(d1.value()._string) < *(d2.value()._string);
				case DET_COMPOUND:
					return true;
				default:
					assert(false);
			}
			break;
		case DET_COMPOUND:
			switch(d2.type()) {
				case DET_INT:
				case DET_DOUBLE:
				case DET_STRING:
					return false;
				case DET_COMPOUND:
					return *(d1.value()._compound) < *(d2.value()._compound);
				default:
					assert(false);
			}
			break;
		default:
			assert(false);
	}
	return false;
}

bool operator>(const DomainElement& d1, const DomainElement& d2) {
	return d2 < d1;
}

bool operator==(const DomainElement& d1, const DomainElement& d2) {
	return &d1 == &d2;
}

bool operator!=(const DomainElement& d1, const DomainElement& d2) {
	return &d1 != &d2;
}

bool operator<=(const DomainElement& d1, const DomainElement& d2) {
	if(d1 == d2) return true;
	else return d1 < d2;
}

bool operator>=(const DomainElement& d1, const DomainElement& d2) {
	if(d1 == d2) return true;
	else return d1 > d2;
}

Compound::Compound(Function* function, const std::vector<const DomainElement*> arguments) :
	_function(function), _arguments(arguments) { 
	assert(function != 0); 
}

/**
 *	\brief Destructor for compound domain element values. Does not delete its arguments.
 */
Compound::~Compound() { 
}

inline Function* Compound::function() const {
	return _function;
}

inline const DomainElement* Compound::arg(unsigned int n) const {
	return _arguments[n];
}

ostream& Compound::put(ostream& output) const {
	output << *_function;
	if(_function->arity() > 0) {
		output << '(' << *_arguments[0];
		for(unsigned int n = 1; n < _function->arity(); ++n) {
			output << ',' << *_arguments[n];
		}
		output << ')';
	}
	return output;
}

string Compound::to_string() const {
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
	if(c1.function() < c2.function()) return true;
	else if(c1.function() > c2.function()) return false;
	else {
		for(unsigned int n = 0; n < c1.function()->arity(); ++n) {
			if(c1.arg(n) < c2.arg(n)) return true;
			else if(c1.arg(n) > c2.arg(n)) return false;
		}
	}
	return false;
}

bool operator>(const Compound& c1, const Compound& c2) {
	return c2 < c1;
}

bool operator==(const Compound& c1,const Compound& c2) {
	return &c1 == &c2;
}

bool operator!=(const Compound& c1,const Compound& c2) {
	return &c1 != &c2;
}

bool operator<=(const Compound& c1,const Compound& c2) {
	return (c1 == c2 || c1 < c2);
}

bool operator>=(const Compound& c1,const Compound& c2) {
	return (c1 == c2 || c2 < c1);
}

/**
 *	Constructor for a domain element factory. The constructor gets two arguments, 
 *	specifying the range of integer for which creation of domain elements is optimized.
 *
 * PARAMETERS
 *		- firstfastint:	the lowest 'efficient' integer
 *		- lastfastint:	one past the highest 'efficient' integer
 */
DomainElementFactory::DomainElementFactory(int firstfastint, int lastfastint) : 
	_firstfastint(firstfastint), _lastfastint(lastfastint) {
	assert(firstfastint < lastfastint);
	_fastintelements = vector<DomainElement*>(lastfastint - firstfastint,(DomainElement*)0);
}

DomainElementFactory* DomainElementFactory::_instance = 0;

/**
 *	\brief Returns the unique instance of DomainElementFactory
 */
DomainElementFactory* DomainElementFactory::instance() {
	if(!_instance) _instance = new DomainElementFactory();
	return _instance;
}

/**
 *	\brief Destructor for DomainElementFactory. Deletes all domain elements and compounds it created.
 */
DomainElementFactory::~DomainElementFactory() {
	for(vector<DomainElement*>::iterator it = _fastintelements.begin(); it != _fastintelements.end(); ++it) 
		delete(*it);
	for(map<int,DomainElement*>::iterator it = _intelements.begin(); it != _intelements.end(); ++it) 
		delete(it->second);
	for(map<double,DomainElement*>::iterator it = _doubleelements.begin(); it != _doubleelements.end(); ++it) 
		delete(it->second);
	for(map<const string*,DomainElement*>::iterator it = _stringelements.begin(); it != _stringelements.end(); ++it) 
		delete(it->second);
	for(map<const Compound*,DomainElement*>::iterator it = _compoundelements.begin(); it != _compoundelements.end(); ++it) 
		delete(it->second);
	for(map<Function*,map<vector<const DomainElement*>,Compound*> >::iterator it = _compounds.begin(); it != _compounds.end(); ++it) {
		for(map<vector<const DomainElement*>,Compound*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			delete(jt->second);
		}
	}
}

/**
 * \brief Returns the unique compound that consists of the given function and arguments.
 * 
 * PARAMETERS
 *		- function:	the given function
 *		- args:		the given arguments
 */
const Compound* DomainElementFactory::compound(Function* function, const ElementTuple& args) {
	map<Function*,map<ElementTuple,Compound*> >::const_iterator it = _compounds.find(function);
	if(it != _compounds.end()) {
		map<ElementTuple,Compound*>::const_iterator jt = it->second.find(args); 
		if(jt != it->second.end()) return jt->second;
	}
	Compound* newcompound = new Compound(function,args);
	_compounds[function][args] = newcompound;
	return newcompound;
}

/**
 * \brief Returns the unique domain element of type int that has a given value
 *
 * PARAMETERS
 *		- value: the given value
 */
const DomainElement* DomainElementFactory::create(int value) {
	DomainElement* element = 0;
	// Check if the value is within the efficient range
	if(value >= _firstfastint && value < _lastfastint) {
		int lookupvalue = value + _firstfastint;
		element = _fastintelements[lookupvalue];
		if(element) return element;
		else {
			element = new DomainElement(value);
			_fastintelements[lookupvalue] = element;
		}
	}
	else { // The value is not within the efficient range
		map<int,DomainElement*>::const_iterator it = _intelements.find(value);
		if(it == _intelements.end()) {
			element = new DomainElement(value);
			_intelements[value] = element;
		}
		else {
			element = it->second;
		}
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given floating point value
 *
 * PARAMETERS
 *		- value:		the given value
 *		- certnotint:	true iff the caller of this method asserts that the value is not an integer
 */
const DomainElement* DomainElementFactory::create(double value, bool certnotint) {
	if(!certnotint && isInt(value)) return create(int(value));

	DomainElement* element;
	map<double,DomainElement*>::const_iterator it = _doubleelements.find(value);
	if(it == _doubleelements.end()) {
		element = new DomainElement(value);
		_doubleelements[value] = element;
	}
	else {
		element = it->second;
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given string value
 *
 * PARAMETERS
 *		- value:			the given value
 *		- certnotdouble:	true iff the caller of this method asserts that the value is not a floating point number
 */
const DomainElement* DomainElementFactory::create(const string* value, bool certnotdouble) {
	if(!certnotdouble && isDouble(*value)) return create(stod(*value),false);

	DomainElement* element;
	map<const string*,DomainElement*>::const_iterator it = _stringelements.find(value);
	if(it == _stringelements.end()) {
		element = new DomainElement(value);
		_stringelements[value] = element;
	}
	else {
		element = it->second;
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- value:	the given value
 */
const DomainElement* DomainElementFactory::create(const Compound* value) {
	DomainElement* element;
	map<const Compound*,DomainElement*>::const_iterator it = _compoundelements.find(value);
	if(it == _compoundelements.end()) {
		element = new DomainElement(value);
		_compoundelements[value] = element;
	}
	else {
		element = it->second;
	}
	return element;
}

/**
 * \brief Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- function:	the function of the given compound value
 *		- args:		the arguments of the given compound value
 */
const DomainElement* DomainElementFactory::create(Function* function, const ElementTuple& args) {
	assert(function != 0);
	const Compound* value = compound(function,args);
	return create(value);
}

/****************
	Iterators
****************/

TableIterator::TableIterator(const TableIterator& tab) {
	_iterator = tab.iterator()->clone();
}

TableIterator& TableIterator::operator=(const TableIterator& tab) {
	if(this != &tab) {
		delete(_iterator);
		_iterator = tab.iterator()->clone();

	}
	return *this;
}

inline bool TableIterator::hasNext() const {
	return _iterator->hasNext();
}

inline const ElementTuple& TableIterator::operator*() const {
	return _iterator->operator*();
}

TableIterator::~TableIterator() {
	delete(_iterator);
}

void TableIterator::operator++() {
	_iterator->operator++();
}

SortIterator::SortIterator(const SortIterator& si) {
	_iterator = si.iterator()->clone();
}

SortIterator& SortIterator::operator=(const SortIterator& si) {
	if(this != &si) {
		delete(_iterator);
		_iterator = si.iterator()->clone();
	}
	return *this;
}

inline bool SortIterator::hasNext() const {
	return _iterator->hasNext();
}

inline const DomainElement* SortIterator::operator*() const {
	return _iterator->operator*();
}

SortIterator::~SortIterator() {
	delete(_iterator);
}

SortIterator& SortIterator::operator++() {
	_iterator->operator++();
	return *this;
}

CartesianInternalTableIterator* CartesianInternalTableIterator::clone() const {
	return new CartesianInternalTableIterator(_iterators,_lowest,_hasNext);
}

bool CartesianInternalTableIterator::hasNext() const {
	return _hasNext;
}

const ElementTuple& CartesianInternalTableIterator::operator*() const {
	ElementTuple tup;
	for(vector<SortIterator>::const_iterator it = _iterators.begin(); it != _iterators.end(); ++it) 
		tup.push_back(*(*it));
	_deref.push_back(tup);
	return _deref.back();
}

void CartesianInternalTableIterator::operator++() {
	int n = _iterators.size() - 1;
	for(; n >= 0; --n) {
		++(_iterators[n]);
		if(_iterators[n].hasNext()) break;
		else _iterators[n] = _lowest[n];
	}
	if(n < 0) _hasNext = false;
}

SortInternalTableIterator::~SortInternalTableIterator() {
	delete(_iter);
}

inline bool SortInternalTableIterator::hasNext() const {
	return _iter->hasNext();
}

inline const ElementTuple& SortInternalTableIterator::operator*() const {
	ElementTuple tuple(1,*(*_iter));
	_deref.push_back(tuple);
	return _deref.back();
}

inline void	SortInternalTableIterator::operator++() {
	_iter->operator++();
}

SortInternalTableIterator* SortInternalTableIterator::clone() const {
	return new SortInternalTableIterator(_iter->clone());
}

EnumInternalIterator* EnumInternalIterator::clone() const {
	return new EnumInternalIterator(_iter,_end);
}

const ElementTuple& EnumInternalFuncIterator::operator*() const {
	ElementTuple tuple = _iter->first;
	tuple.push_back(_iter->second);
	_deref.push_back(tuple);
	return _deref.back();
}

EnumInternalFuncIterator* EnumInternalFuncIterator::clone() const {
	return new EnumInternalFuncIterator(_iter,_end);
}

bool UnionInternalIterator::contains(const ElementTuple& tuple) const {
	for(vector<InternalPredTable*>::const_iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(tuple,_universe)) return false;
	}
	return true;
}

void UnionInternalIterator::setcurriterator() {
	_curriterator = _iterators.begin();
	for(; _curriterator != _iterators.end(); ) {
		if(_curriterator->hasNext()) {
			if(contains(*(*_curriterator))) break;
			else  ++(*_curriterator); 
		}
		else ++_curriterator;
	}
	if(_curriterator->hasNext()) {
		vector<TableIterator>::iterator jt = _curriterator; ++jt;
		for(; jt != _iterators.end(); ) {
			if(jt->hasNext()) {
				if(contains(*(*jt))) {
					StrictWeakTupleOrdering swto;
					if(swto(*(*jt),*(*_curriterator))) _curriterator = jt;
					else if(!swto(*(*_curriterator),*(*jt))) {
						++(*jt);
						++jt;
					}
				}
				else ++(*jt); 
			}
			else ++jt;
		}
	}
}

UnionInternalIterator::UnionInternalIterator(const vector<TableIterator>& its, const vector<PredTable*>& outs) :
	_iterators(its), _outtables(outs) {
	setcurriterator();
}

UnionInternalIterator* UnionInternalIterator::clone() const {
	return new UnionInternalIterator(_iterators,_outtables);
}

bool UnionInternalIterator::hasNext() const {
	return _curriterator != _iterators.end();
}

const ElementTuple& UnionInternalIterator::operator*() const {
	assert(_curriterator->hasNext());
	return *(*_curriterator);
}

void UnionInternalIterator::operator++() {
	++(*_curriterator);
	setcurriterator();
}

InverseInternalIterator::InverseInternalIterator(const vector<SortIterator>& its, InternalPredTable* out, const Universe& univ) :
	_curr(its), _lowest(its), _universe(univ), _outtable(out), _end(false), _currtuple(its.size())  {
	for(unsigned int n = 0; n < _curr.size(); ++n) {
		if(_curr[n].hasNext()) _currtuple[n] = *(_curr[n]);
		else _end = true; break;
	}
	if((!_end) && _outtable->contains(_currtuple,_universe)) operator++();
}

InverseInternalIterator::InverseInternalIterator(const vector<SortIterator>& curr, const vector<SortIterator>& low, InternalPredTable* out, const Universe& univ, bool end) :
	_curr(curr), _lowest(low), _universe(univ), _outtable(out), _end(end), _currtuple(curr.size()) {
	for(unsigned int n = 0; n < _curr.size(); ++n) {
		if(_curr[n].hasNext()) _currtuple[n] = *(_curr[n]);
	}
}

InverseInternalIterator* InverseInternalIterator::clone() const {
	return new InverseInternalIterator(_curr,_lowest,_outtable,_universe,_end);
}

bool InverseInternalIterator::hasNext() const {
	return !_end;
}

const ElementTuple& InverseInternalIterator::operator*() const {
	_deref.push_back(_currtuple);
	return _deref.back();
}

void InverseInternalIterator::operator++() {
	int pos = _curr.size() - 1;
	do {
		for(vector<SortIterator>::reverse_iterator it = _curr.rbegin(); it != _curr.rend(); ++it, --pos) {
			assert(it->hasNext());
			++(*it);
			if(it->hasNext()) {
				_currtuple[pos] = *(*it);
				break;
			}
			else {
				_curr[pos] = _lowest[pos];
				assert(_curr[pos].hasNext());
				_currtuple[pos] = *(_curr[pos]);
			}
		}
		if(pos < 0) _end = true;
	} while((!_end) && _outtable->contains(_currtuple,_universe));
}

EqualInternalIterator::EqualInternalIterator(const SortIterator& iter) 
	: _iterator(iter) {
}

bool EqualInternalIterator::hasNext() const {
	return _iterator.hasNext();
}

const ElementTuple& EqualInternalIterator::operator*() const {
	_deref.push_back(ElementTuple(2,*_iterator));
	return _deref.back();
}

void EqualInternalIterator::operator++() {
	++_iterator;
}

EqualInternalIterator* EqualInternalIterator::clone() const {
	return new EqualInternalIterator(_iterator);
}

StrLessThanInternalIterator::StrLessThanInternalIterator(const SortIterator& si) :
	_leftiterator(si), _rightiterator(si) {
	if(_rightiterator.hasNext()) ++_rightiterator;
}	

bool StrLessThanInternalIterator::hasNext() const {
	return (_leftiterator.hasNext() && _rightiterator.hasNext());
}

const ElementTuple& StrLessThanInternalIterator::operator*() const {
	ElementTuple tuple(2); tuple[0] = *_leftiterator; tuple[1] = *_rightiterator;
	_deref.push_back(tuple);
	return _deref.back();
}

void StrLessThanInternalIterator::operator++() {
	if(_rightiterator.hasNext()) ++_rightiterator;
	if(_rightiterator.hasNext()) return;
	else if(_leftiterator.hasNext()) {
		++_leftiterator;
		_rightiterator = _leftiterator;
		if(_rightiterator.hasNext()) ++_rightiterator;
	}
}

StrLessThanInternalIterator* StrLessThanInternalIterator::clone() const {
	return new StrLessThanInternalIterator(_leftiterator,_rightiterator);
}

StrGreaterThanInternalIterator::StrGreaterThanInternalIterator(const SortIterator& si) :
	_leftiterator(si), _rightiterator(si), _lowest(si) {
	if(_leftiterator.hasNext()) ++_leftiterator;
}	

bool StrGreaterThanInternalIterator::hasNext() const {
	return (_leftiterator.hasNext() && _rightiterator.hasNext());
}

const ElementTuple& StrGreaterThanInternalIterator::operator*() const {
	ElementTuple tuple(2); tuple[0] = *_leftiterator; tuple[1] = *_rightiterator;
	_deref.push_back(tuple);
	return _deref.back();
}

void StrGreaterThanInternalIterator::operator++() {
	if(_rightiterator.hasNext()) ++_rightiterator;
	if(_leftiterator.hasNext() && _rightiterator.hasNext()) {
		if(*(*_leftiterator) > *(*_rightiterator)) return;
		else {
			++_leftiterator;
			_rightiterator = _lowest;
		}
	}
}

StrGreaterThanInternalIterator* StrGreaterThanInternalIterator::clone() const {
	return new StrGreaterThanInternalIterator(_leftiterator,_rightiterator,_lowest);
}

UnionInternalSortIterator::UnionInternalSortIterator(const vector<SortIterator>& vsi, const vector<SortTable*>& tabs) :
	_iterators(vsi), _outtables(tabs) {
	setcurriterator();
}

UnionInternalSortIterator* UnionInternalSortIterator::clone() const {
	return new UnionInternalSortIterator(_iterators,_outtables);
}

bool UnionInternalSortIterator::contains(const DomainElement* d) const {
	for(vector<SortTable*>::const_iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(d)) return false;
	}
	return true;
}

void UnionInternalSortIterator::setcurriterator() {
	_curriterator = _iterators.begin();
	for(; _curriterator != _iterators.end(); ) {
		if(_curriterator->hasNext()) {
			if(contains(*(*_curriterator))) break;
			else  ++(*_curriterator); 
		}
		else ++_curriterator;
	}
	if(_curriterator->hasNext()) {
		vector<SortIterator>::iterator jt = _curriterator; ++jt;
		for(; jt != _iterators.end(); ) {
			if(jt->hasNext()) {
				if(contains(*(*jt))) {
					if(*(*(*jt))< *(*(*_curriterator))) _curriterator = jt;
					else if(*(*_curriterator) == *(*jt)) {
						++(*jt);
						++jt;
					}
				}
				else ++(*jt); 
			}
			else ++jt;
		}
	}
}

bool UnionInternalSortIterator::hasNext() const {
	return _curriterator != _iterators.end();
}

const DomainElement* UnionInternalSortIterator::operator*() const {
	assert(_curriterator->hasNext());
	return *(*_curriterator);
}

void UnionInternalSortIterator::operator++() {
	++(*_curriterator);
	setcurriterator();
}

void StringInternalSortIterator::operator++() {
	int n = _iter.size() - 1;
	for(; n >= 0; --n) {
		++_iter[n];
		if(_iter[n] != numeric_limits<char>::min()) break;
	}
	if(n < 0) {
		_iter = string(_iter.size()+1,numeric_limits<char>::min());
	}
}

const DomainElement* StringInternalSortIterator::operator*() const {
	return DomainElementFactory::instance()->create(StringPointer(_iter));
}

const DomainElement* CharInternalSortIterator::operator*() const {
	if('0' <= _iter && _iter <= '9') {
		int i = _iter - '0';
		return DomainElementFactory::instance()->create(i);
	}
	else {
		string* s = StringPointer(string(1,_iter));
		return DomainElementFactory::instance()->create(s);
	}
}

void CharInternalSortIterator::operator++() {
	if(_iter == numeric_limits<char>::max()) _end = true;
	else ++_iter;
}


/*************************
	Internal predtable
*************************/

bool Universe::empty() const {
	for(vector<SortTable*>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
		if((*it)->empty()) return true;
	}
	return false;
}

bool Universe::finite() const {
	if(empty()) return true;
	for(vector<SortTable*>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
		if(!(*it)->finite()) return false;
	}
	return true;
}

bool Universe::approxempty() const {
	for(vector<SortTable*>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
		if((*it)->approxempty()) return true;
	}
	return false;
}

bool Universe::approxfinite() const {
	if(approxempty()) return true;
	for(vector<SortTable*>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
		if(!(*it)->approxfinite()) return false;
	}
	return true;
}

bool Universe::contains(const ElementTuple& tuple) const {
	for(unsigned int n = 0; n < tuple.size(); ++n) {
		if(!_tables[n]->contains(tuple[n])) return false;
	}
	return true;
}

bool StrictWeakTupleOrdering::operator()(const ElementTuple& t1, const ElementTuple& t2) const {
	assert(t1.size() == t2.size());
	for(unsigned int n = 0; n < t1.size(); ++n) {
		if(*(t1[n]) < *(t2[n])) return true;
		else if(*(t1[n]) > *(t2[n])) return false;
	}
	return false;
}

bool StrictWeakNTupleEquality::operator()(const ElementTuple& t1, const ElementTuple& t2) const {
	for(unsigned int n = 0; n < _arity; ++n) {
		if(t1[n] != t2[n]) return false;
	}
	return true;
}


FuncInternalPredTable::FuncInternalPredTable(FuncTable* table, bool linked) : 
	InternalPredTable(), _table(table), _linked(linked) { }

FuncInternalPredTable::~FuncInternalPredTable() {
	if(!_linked) delete(_table);
}

inline bool FuncInternalPredTable::finite(const Universe&) const { 
	return _table->finite();		
}

inline bool FuncInternalPredTable::empty(const Universe&) const {
	return _table->empty();			
}

inline bool FuncInternalPredTable::approxfinite(const Universe&) const {
	return _table->approxfinite();	
}

inline bool FuncInternalPredTable::approxempty(const Universe&) const {
	return _table->approxempty();	
}

bool FuncInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if(_table->contains(tuple)) {
		return univ.contains(tuple);
	}
	else return false;
}

InternalPredTable* FuncInternalPredTable::add(const ElementTuple& tuple) {
	ElementTuple in = tuple;
	in.pop_back();
	const DomainElement* out = _table->operator[](in);
	if(out == 0) {
		if(_nrRefs > 1 && !_linked) {
			FuncTable* nft = new FuncTable(_table->interntable(),_table->universe());
			nft->add(tuple);
			return new FuncInternalPredTable(nft,false);
		}
		else {
			_table->add(tuple);
			return this;
		}
	}
	else if(_table->approxfinite()) {
		EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
		for(TableIterator it = _table->begin(); it.hasNext(); ++it)
			eipt->add(*it);
		eipt->add(tuple);
		return eipt;
	}
	else {
		UnionInternalPredTable* uipt = new UnionInternalPredTable();
		FuncTable* nft = new FuncTable(_table->interntable(),_table->universe());
		uipt->addInTable(new FuncInternalPredTable(nft,false));
		uipt->add(tuple);
		return uipt;
	}
}

InternalPredTable* FuncInternalPredTable::remove(const ElementTuple& tuple) {
	if(_nrRefs > 1 && !_linked) {
		FuncTable* nft = new FuncTable(_table->interntable(),_table->universe());
		nft->remove(tuple);
		return new FuncInternalPredTable(nft,false);
	}
	else {
		_table->remove(tuple);
		return this;
	}
}

InternalTableIterator* FuncInternalPredTable::begin(const Universe&) const {
	return _table->interntable()->begin(_table->universe());
}

FullInternalPredTable::~FullInternalPredTable() { }

bool FullInternalPredTable::finite(const Universe& univ) const {
	return univ.finite();
}

bool FullInternalPredTable::empty(const Universe& univ) const {
	return univ.empty();
}

bool FullInternalPredTable::approxfinite(const Universe& univ) const {
	return univ.approxfinite();
}

bool FullInternalPredTable::approxempty(const Universe& univ) const {
	return univ.approxempty();
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
	for(vector<SortTable*>::const_iterator it = univ.tables().begin(); it != univ.tables().end(); ++it) {
		vsi.push_back((*it)->sortbegin());
	}
	return new CartesianInternalTableIterator(vsi,vsi);
}

UnionInternalPredTable::UnionInternalPredTable() : InternalPredTable() { 
	_intables.push_back(new EnumeratedInternalPredTable());
	_outtables.push_back(new EnumeratedInternalPredTable());
}

/**
 *	Destructor for union predicate tables
 */
UnionInternalPredTable::~UnionInternalPredTable() {
	for(vector<InternalPredTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it)
		(*it)->decrementRef();
	for(vector<InternalPredTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it)
		(*it)->decrementRef();
}

bool UnionInternalPredTable::finite(const Universe& univ) const {
	if(approxfinite(univ)) return true;
	else {
		notyetimplemented("Exact finiteness test on union predicate tables");	
		return approxfinite(univ);
	}
}

bool UnionInternalPredTable::empty(const Universe& univ) const {
	if(approxempty(univ)) return true;
	else {
		notyetimplemented("Exact emptyness test on union predicate tables");	
		return approxempty(univ);
	}
}

bool UnionInternalPredTable::approxfinite(const Universe& univ) const {
	for(vector<InternalPredTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if(!(*it)->approxfinite(univ)) return false;
	}
	return true;
}

bool UnionInternalPredTable::approxempty(const Universe& univ) const {
	for(vector<InternalPredTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if(!(*it)->approxempty(univ)) return false;
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
	for(vector<InternalPredTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if((*it)->contains(tuple,univ)) { in = true; break;	}
	}
	if(!in) return false;

	bool out = false;
	for(vector<InternalPredTable*>::const_iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(tuple,univ)) { out = true; break;	}
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
	if(_nrRefs > 1) {
		UnionInternalPredTable* newtable = new UnionInternalPredTable(_intables,_outtables);
		InternalPredTable* temp = newtable->add(tuple); assert(temp == newtable);
		return newtable;
	}
	else {
		InternalPredTable* temp = _intables[0]->add(tuple);
		if(temp != _intables[0]) {
			_intables[0]->decrementRef();
			temp->incrementRef();
			_intables[0] = temp;
		}
		for(unsigned int n = 0; n < _outtables.size(); ++n) {
			InternalPredTable* temp2 = _outtables[n]->remove(tuple);
			if(temp2 != _outtables[n]) {
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
	assert(!_outtables.empty());
	if(_nrRefs > 1) {
		UnionInternalPredTable* newtable = new UnionInternalPredTable(_intables,_outtables);
		InternalPredTable* temp = newtable->remove(tuple); assert(temp == newtable);
		return newtable;
	}
	else {
		InternalPredTable* temp = _outtables[0]->add(tuple);
		if(temp != _outtables[0]) {
			_outtables[0]->decrementRef();
			temp->incrementRef();
			_outtables[0] = temp;
		}
		return this;
	}
}

InternalTableIterator* UnionInternalPredTable::begin(const Universe& univ) const {
	vector<TableIterator> vti;
	for(vector<InternalPredTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		vti.push_back(TableIterator((*it)->begin(univ)));
	}
	return new UnionInternalIterator(vti,_outtables,univ);
}


/**
 * \brief	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EnumeratedInternalPredTable::contains(const ElementTuple& tuple) const {
	if(_table.empty()) return false;
	else {
		assert(tuple.size() == arity());
		return _table.find(tuple) != _table.end();
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
	if(contains(tuple)) return this;
	else {
		if(_nrRefs == 1) {
			_table.insert(tuple);
			return this;
		}
		else {
			SortedElementTable newtable = _table;
			newtable.insert(tuple);
			return new EnumeratedInternalPredTable(newtable,_universe);
		}
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
	if(it != _table.end()) {
		if(_nrRefs == 1) {
			_table.erase(it);
			return this;
		}
		else {
			SortedElementTable newtable = _table;
			SortedElementTable::iterator nt = newtable.find(tuple);
			newtable.erase(nt);
			return new EnumeratedInternalPredTable(newtable,_universe);
		}
	}
	else return this;
}

/**
 * \brief Returns an iterator on the first tuple of the table
 */
InternalTableIterator* EnumeratedInternalPredTable::begin() const {
	return new EnumInternalIterator(_table.begin(),_table.end());
}

ComparisonInternalPredTable::ComparisonInternalPredTable(const Universe& univ) :
	InternalPredTable(univ) {
}

ComparisonInternalPredTable::~ComparisonInternalPredTable() {
}

InternalPredTable* ComparisonInternalPredTable::add(const ElementTuple& tuple) {
	if(!contains(tuple)) {
		UnionInternalPredTable* upt = new UnionInternalPredTable(_universe);
		upt->addInTable(new PredTable(this));
		InternalPredTable* temp = upt->add(tuple);
		if(temp != upt) delete(upt);
		return temp;
	}
	else return this;
}

InternalPredTable* ComparisonInternalPredTable::remove(const ElementTuple& tuple) {
	if(contains(tuple)) {
		UnionInternalPredTable* upt = new UnionInternalPredTable(_universe);
		upt->addInTable(new PredTable(this));
		InternalPredTable* temp = upt->remove(tuple);
		if(temp != upt) delete(upt);
		return temp;
	}
	else return this;
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EqualInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return (tuple[0] == tuple[1] && _universe.contains(tuple));
}

/**
 * \brief	Returns true iff the table is finite
 */
bool EqualInternalPredTable::finite(const Universe&) const {
	if(approxfinite()) return true;
	else if(_universe.finite()) return true;
	else return false;
}

/**
 * \brief Returns true iff the table is empty
 */
bool EqualInternalPredTable::empty() const {
	if(approxempty()) return true;
	else if(_universe.empty()) return true;
	else return false;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool EqualInternalPredTable::approxfinite(const Universe&) const {
	return _universe.approxfinite();
}

/**
 * \brief Returns false if the table is not empty. May return true if the table is empty.
 */
inline bool EqualInternalPredTable::approxempty() const {
	return _universe.approxempty();
}

InternalTableIterator* EqualInternalPredTable::begin() const {
	return new EqualInternalIterator(_universe.tables()[0]->sortbegin());
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrLessInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return (*(tuple[0]) < *(tuple[1]) && _universe.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrLessInternalPredTable::finite(const Universe&) const {
	if(approxfinite()) return true;
	else return _universe.finite();
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrLessInternalPredTable::empty() const {
	SortIterator isi = _universe.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxfinite(const Universe&) const {
	return (_universe.approxfinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxempty() const {
	SortIterator isi = _universe.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

InternalTableIterator* StrLessInternalPredTable::begin() const {
	return new StrLessThanInternalIterator(_universe.tables()[0]->sortbegin());
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrGreaterInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return (*(tuple[0]) > *(tuple[1]) && _universe.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrGreaterInternalPredTable::finite(const Universe&) const {
	if(approxfinite()) return true;
	else return _universe.finite(); 
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrGreaterInternalPredTable::empty() const {
	SortIterator isi = _universe.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxfinite(const Universe&) const {
	return (_universe.approxfinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxempty() const {
	SortIterator isi = _universe.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

InternalTableIterator* StrGreaterInternalPredTable::begin() const {
	return new StrGreaterThanInternalIterator(_universe.tables()[0]->sortbegin());
}

/*************************
	Internal sorttable
*************************/

InternalTableIterator* InternalSortTable::begin() const {
	return new SortInternalTableIterator(sortbegin());
}

bool EnumeratedInternalSortTable::contains(const DomainElement* d) const {
	return _table.find(d) != _table.end();
}

bool EnumeratedInternalSortTable::isRange() const {
	const DomainElement* f = first();
	const DomainElement* l = last();
	if(f->type() == DET_INT && l->type() == DET_INT) {
		return l->value()._int - f->value()._int == _table.size() - 1;
	}
	else return false;
}

InternalSortIterator* EnumeratedInternalSortTable::sortbegin() const {
	return new EnumInternalSortIterator(_table.begin(),_table.end());
}

InternalSortTable* EnumeratedInternalSortTable::add(int i1, int i2) {
	InternalSortTable* temp = this;
	for(int n = i1; n <= i2; ++n) {
		temp = temp->add(DomainElementFactory::instance()->create(n));
	}
	return temp;
}

InternalSortTable* EnumeratedInternalSortTable::add(const DomainElement* d) {
	if(contains(d)) return this;
	else {
		if(_nrRefs > 1) {
			EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable(_table);
			ist->add(d);
			return ist;
		}
		else {
			_table.insert(d);
			return this;
		}
	}
}

InternalSortTable* EnumeratedInternalSortTable::remove(const DomainElement* d) {
	if(!contains(d)) return this;
	else {
		if(_nrRefs > 1) {
			EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable(_table);
			ist->remove(d);
			return ist;
		}
		else {
			_table.erase(d);
			return this;
		}
	}
}

const DomainElement* EnumeratedInternalSortTable::first() const {
	return *(_table.begin());
}

const DomainElement* EnumeratedInternalSortTable::last() const {
	return *(_table.rbegin());
}


UnionInternalSortTable::UnionInternalSortTable() {
	_intables.push_back(new SortTable(new EnumeratedInternalSortTable()));
	_outtables.push_back(new SortTable(new EnumeratedInternalSortTable()));
}

UnionInternalSortTable::~UnionInternalSortTable() {
	for(vector<SortTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it)
		delete(*it);
	for(vector<SortTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it)
		delete(*it);
}

bool UnionInternalSortTable::finite(const Universe&) const {
	if(approxfinite()) return true;
	else {
		notyetimplemented("Exact finiteness test on union sort tables");	
		return approxfinite();
	}
}

bool UnionInternalSortTable::empty() const {
	if(approxempty()) return true;
	else {
		notyetimplemented("Exact emptyness test on union sort tables");	
		return approxempty();
	}
}

bool UnionInternalSortTable::approxfinite() const {
	for(vector<SortTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if(!(*it)->approxfinite()) return false;
	}
	return true;
}

bool UnionInternalSortTable::approxempty() const {
	for(vector<SortTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if(!(*it)->approxempty()) return false;
	}
	return true;
}

bool UnionInternalSortTable::contains(const DomainElement* d) const {
	bool in = false;
	for(vector<SortTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if((*it)->contains(d)) { in = true; break;	}
	}
	if(!in) return false;

	bool out = false;
	for(vector<SortTable*>::const_iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(d)) { out = true; break;	}
	}
	return !out;
}

InternalSortTable* UnionInternalSortTable::add(int i1, int i2) {
	InternalSortTable* temp = this;
	for(int n = i1; n <= i2; ++n) {
		temp = temp->add(DomainElementFactory::instance()->create(n));
	}
	return temp;
}

InternalSortTable* UnionInternalSortTable::add(const DomainElement* d) {
	if(!contains(d)) {
		if(_nrRefs > 1) {
			vector<SortTable*> newin;
			vector<SortTable*> newout;
			for(vector<SortTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it) {
				newin.push_back(new SortTable((*it)->interntable()));
			}
			for(vector<SortTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
				newout.push_back(new SortTable((*it)->interntable()));
			}
			UnionInternalSortTable* newtable = new UnionInternalSortTable(newin,newout);
			InternalSortTable* temp = newtable->add(d); assert(temp == newtable);
			return newtable;
		}
		else {
			bool in = false;
			for(unsigned int n = 0; n < _intables.size(); ++n) {
				if(_intables[n]->contains(d)) { in = true; break;	}
			}
			if(in) {
				for(unsigned int n = 0; n < _outtables.size(); ++n) 
					_outtables[n]->remove(d);
			}
			else {
				assert(!_intables.empty());
				_intables[0]->add(d);
			}
			return this;
		}
	}
	else return this;
}

InternalSortTable* UnionInternalSortTable::remove(const DomainElement* d) {
	assert(!_outtables.empty());
	if(_nrRefs > 1) {
		vector<SortTable*> newin;
		vector<SortTable*> newout;
		for(vector<SortTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it) {
			newin.push_back(new SortTable((*it)->interntable()));
		}
		for(vector<SortTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
			newout.push_back(new SortTable((*it)->interntable()));
		}
		UnionInternalSortTable* newtable = new UnionInternalSortTable(newin,newout);
		InternalSortTable* temp = newtable->remove(d); assert(temp == newtable);
		return newtable;
	}
	else {
		_outtables[0]->add(d);
		return this;
	}
}

InternalSortIterator* UnionInternalSortTable::sortbegin() const {
	vector<SortIterator> vsi;
	for(vector<SortTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		vsi.push_back((*it)->sortbegin());
	}
	return new UnionInternalSortIterator(vsi,_outtables);
}

const DomainElement* UnionInternalSortTable::first() const {
	// TODO
	return 0;
}

const DomainElement* UnionInternalSortTable::last() const {
	// TODO
	return 0;
}

bool UnionInternalSortTable::isRange() const {
	// TODO
	return false;
}

InternalSortTable* InfiniteInternalSortTable::add(const DomainElement* d) {
	if(!contains(d)) {
		UnionInternalSortTable* upt = new UnionInternalSortTable();
		upt->addInTable(new SortTable(this));
		InternalSortTable* temp = upt->add(d);
		if(temp != upt) delete(upt);
		return temp;
	}
	else return this;
}

InternalSortTable* InfiniteInternalSortTable::remove(const DomainElement* d) {
	if(contains(d)) {
		UnionInternalSortTable* upt = new UnionInternalSortTable();
		upt->addOutTable(new SortTable(this));
		InternalSortTable* temp = upt->remove(d);
		if(temp != upt) delete(upt);
		return temp;
	}
	else return this;
}

bool AllNaturalNumbers::contains(const DomainElement* d) const {
	if(d->type() == DET_INT) return d->value()._int >= 0;
	else return false;
}

InternalSortIterator* AllNaturalNumbers::sortbegin() const {
	return new NatInternalSortIterator();
}

const DomainElement* AllNaturalNumbers::first() const {
	return DomainElementFactory::instance()->create(0);
}

const DomainElement* AllNaturalNumbers::last() const {
	return DomainElementFactory::instance()->create(numeric_limits<int>::max());
}

InternalSortTable* AllNaturalNumbers::add(int i1,int i2) {
	if(i1 >= 0) return this;
	else {
		int stop = i2 > 0 ? 0 : i2;
		InternalSortTable* temp = this;
		for(int n = i1; n < stop; ++n) {
			temp = temp->add(DomainElementFactory::instance()->create(n));
		}
		return temp;
	}
}

bool AllIntegers::contains(const DomainElement* d) const {
	return (d->type() == DET_INT);
}

InternalSortIterator* AllIntegers::sortbegin() const {
	return new IntInternalSortIterator();
}

InternalSortTable* AllIntegers::add(int,int) {
	return this;
}

const DomainElement* AllIntegers::first() const {
	return DomainElementFactory::instance()->create(numeric_limits<int>::min());
}

const DomainElement* AllIntegers::last() const {
	return DomainElementFactory::instance()->create(numeric_limits<int>::max());
}

bool AllFloats::contains(const DomainElement* d) const {
	return (d->type() == DET_INT || d->type() == DET_DOUBLE);
}

InternalSortIterator* AllFloats::sortbegin() const {
	return new FloatInternalSortIterator();
}

InternalSortTable* AllFloats::add(int,int) {
	return this;
}

const DomainElement* AllFloats::first() const {
	return DomainElementFactory::instance()->create(numeric_limits<double>::min());
}

const DomainElement* AllFloats::last() const {
	return DomainElementFactory::instance()->create(numeric_limits<double>::max());
}

bool AllStrings::contains(const DomainElement* d) const {
	return d->type() != DET_COMPOUND;
}

InternalSortTable* AllStrings::add(int,int) {
	return this;
}

InternalSortIterator* AllStrings::sortbegin() const {
	return new StringInternalSortIterator();
}

const DomainElement* AllStrings::first() const {
	return DomainElementFactory::instance()->create(StringPointer(""));
}

const DomainElement* AllStrings::last() const {
	notyetimplemented("impossible to get the largest string");
	return 0;
}

bool AllChars::contains(const DomainElement* d) const {
	if(d->type() == DET_INT) return (d->value()._int >= 0 && d->value()._int < 10);
	else if(d->type() == DET_STRING) return (d->value()._string->size() == 1);
	else return false;
}

InternalSortTable* AllChars::add(int i1,int i2) {
	InternalSortTable* temp = this;
	for(int n = i1; n <= i2; ++n) {
		temp = temp->add(DomainElementFactory::instance()->create(n));
	}
	return temp;
}

InternalSortTable* AllChars::add(const DomainElement* d) {
	if(contains(d)) return this;
	else {
		EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable;
		for(char c = numeric_limits<char>::min(); c <= numeric_limits<char>::max(); ++c) {
			if('0' <= c && c <= '9') {
				int i = c - '0';
				ist->add(DomainElementFactory::instance()->create(i));
			}
			else {
				string* s = StringPointer(string(1,c));
				ist->add(DomainElementFactory::instance()->create(s));
			}
		}
		ist->add(d);
		return ist;
	}
}

InternalSortTable* AllChars::remove(const DomainElement* d) {
	if(!contains(d)) return this;
	else {
		EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable;
		for(char c = numeric_limits<char>::min(); c <= numeric_limits<char>::max(); ++c) {
			if('0' <= c && c <= '9') {
				int i = c - '0';
				ist->add(DomainElementFactory::instance()->create(i));
			}
			else {
				string* s = StringPointer(string(1,c));
				ist->add(DomainElementFactory::instance()->create(s));
			}
		}
		ist->remove(d);
		return ist;
	}
}

InternalSortIterator* AllChars::sortbegin() const {
	return new CharInternalSortIterator();
}

const DomainElement* AllChars::first() const {
	return DomainElementFactory::instance()->create(StringPointer(string(1,numeric_limits<char>::min())));
}

const DomainElement* AllChars::last() const {
	return DomainElementFactory::instance()->create(StringPointer(string(1,numeric_limits<char>::max())));
}

/*************************
	Internal functable
*************************/

inline void InternalFuncTable::incrementRef() {
	++_nrRefs;
}

inline void InternalFuncTable::decrementRef() {
	--_nrRefs;
	if(_nrRefs == 0) delete(this);
}

bool InternalFuncTable::contains(const ElementTuple& tuple) const {
	ElementTuple input = tuple;
	const DomainElement* output = tuple.back();
	input.pop_back();
	const DomainElement* computedoutput = operator[](input);
	return output == computedoutput;
}

ProcInternalFuncTable::~ProcInternalFuncTable() {
}

bool ProcInternalFuncTable::finite() const {
	if(empty()) return true;
	for(unsigned int n = 0; n < arity(); ++n) {
		if(!_universe.tables()[n]->finite()) return false;
	}
	return true;
}

bool ProcInternalFuncTable::empty() const {
	for(unsigned int n = 0; n < arity(); ++n) {
		if(_universe.tables()[n]->empty()) return true;
	}
	return false;
}

bool ProcInternalFuncTable::approxfinite() const {
	if(approxempty()) return true;
	for(unsigned int n = 0; n < arity(); ++n) {
		if(!_universe.tables()[n]->approxfinite()) return false;
	}
	return true;
}

bool ProcInternalFuncTable::approxempty() const {
	for(unsigned int n = 0; n < arity(); ++n) {
		if(_universe.tables()[n]->approxempty()) return true;
	}
	return false;
}

const DomainElement* ProcInternalFuncTable::operator[](const ElementTuple& tuple) const {
	for(unsigned int n = 0; n < arity(); ++n) {
		if(!_universe.tables()[n]->contains(tuple[n])) return 0;
	}
	return LuaConnection::funccall(_procedure,tuple);
}

InternalFuncTable* ProcInternalFuncTable::add(const ElementTuple& tuple) {
	if(!contains(tuple)) {
		// TODO
		return 0;
	}
	else return this;
}

InternalFuncTable* ProcInternalFuncTable::remove(const ElementTuple& tuple) {
	if(contains(tuple)) {
		// TODO
		return 0;
	}
	else return this;
}

InternalTableIterator* ProcInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* EnumeratedInternalFuncTable::operator[](const ElementTuple& tuple) const {
	ElementFunc::const_iterator it = _table.find(tuple);
	if(it != _table.end()) return it->second;
	else return 0;
}

InternalFuncTable* EnumeratedInternalFuncTable::add(const ElementTuple& tuple) {
	ElementTuple key = tuple;
	const DomainElement* value = key.back(); 
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	if(computedvalue != 0) {
		if(computedvalue == value) return this;
		else assert(false);
	}
	else {
		if(_nrRefs > 1) {
			ElementFunc newtable = _table;
			newtable[key] = value;
			return new EnumeratedInternalFuncTable(newtable,_universe);
		}
		else {
			_table[key] = value;
			return this;
		}
	}
}

InternalFuncTable* EnumeratedInternalFuncTable::remove(const ElementTuple& tuple) {
	ElementTuple key = tuple;
	const DomainElement* value = key.back(); 
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	if(computedvalue == value) {
		if(_nrRefs > 1) {
			ElementFunc newtable = _table;
			newtable.erase(key);
			return new EnumeratedInternalFuncTable(newtable,_universe);
		}
		else {
			_table.erase(key);
			return this;
		}
	}
	else return this;
}

InternalTableIterator* EnumeratedInternalFuncTable::begin() const {
	return new EnumInternalFuncIterator(_table.begin(),_table.end());
}

const DomainElement* ModInternalFuncTable::operator[](const vector<const DomainElement*>& tuple) const {
	int a1 = tuple[0]->value()._int;
	int a2 = tuple[1]->value()._int;
	return DomainElementFactory::instance()->create(a1 % a2);
}

InternalFuncTable* ModInternalFuncTable::add(const ElementTuple& tuple) {
	if(contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

InternalFuncTable* ModInternalFuncTable::remove(const ElementTuple& tuple) {
	if(!contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

InternalTableIterator* ModInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* ExpInternalFuncTable::operator[](const vector<const DomainElement*>& tuple) const {
	double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
	double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
	return DomainElementFactory::instance()->create(pow(a1,a2),false);
}

InternalFuncTable* ExpInternalFuncTable::add(const ElementTuple& tuple) {
	if(contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

InternalFuncTable* ExpInternalFuncTable::remove(const ElementTuple& tuple) {
	if(!contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

InternalTableIterator* ExpInternalFuncTable::begin() const {
	// TODO
	return 0;
}

InternalFuncTable* IntFloatInternalFuncTable::add(const ElementTuple& tuple) {
	if(contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

InternalFuncTable* IntFloatInternalFuncTable::remove(const ElementTuple& tuple) {
	if(!contains(tuple)) return this;
	else {
		// TODO
		return 0;
	}
}

const DomainElement* PlusInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(_int) {
		int a1 = tuple[0]->value()._int;
		int a2 = tuple[1]->value()._int;
		return DomainElementFactory::instance()->create(a1 + a2);
	}
	else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return DomainElementFactory::instance()->create(a1 + a2,false);
	}
}

InternalTableIterator* PlusInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* MinusInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(_int) {
		int a1 = tuple[0]->value()._int;
		int a2 = tuple[1]->value()._int;
		return DomainElementFactory::instance()->create(a1 - a2);
	}
	else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return DomainElementFactory::instance()->create(a1 - a2,false);
	}
}

InternalTableIterator* MinusInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* TimesInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(_int) {
		int a1 = tuple[0]->value()._int;
		int a2 = tuple[1]->value()._int;
		return DomainElementFactory::instance()->create(a1 * a2);
	}
	else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return DomainElementFactory::instance()->create(a1 * a2,false);
	}
}

InternalTableIterator* TimesInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* DivInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(_int) {
		int a1 = tuple[0]->value()._int;
		int a2 = tuple[1]->value()._int;
		return DomainElementFactory::instance()->create(a1 / a2);
	}
	else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		return DomainElementFactory::instance()->create(a1 / a2,false);
	}
}

InternalTableIterator* DivInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* AbsInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(tuple[0]->type() == DET_INT) {
		return DomainElementFactory::instance()->create(abs(tuple[0]->value()._int));
	}
	else {
		return DomainElementFactory::instance()->create(abs(tuple[0]->value()._double),true);
	}
}

InternalTableIterator* AbsInternalFuncTable::begin() const {
	// TODO
	return 0;
}

const DomainElement* UminInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(tuple[0]->type() == DET_INT) {
		return DomainElementFactory::instance()->create(-(tuple[0]->value()._int));
	}
	else {
		return DomainElementFactory::instance()->create(-(tuple[0]->value()._double),true);
	}
}

InternalTableIterator* UminInternalFuncTable::begin() const {
	// TODO
	return 0;
}

/****************
	PredTable
****************/

PredTable::PredTable(InternalPredTable* table) : _table(table) {
	table->incrementRef();
}

PredTable::~PredTable() {
	_table->decrementRef();
}

void PredTable::add(const ElementTuple& tuple) {
	if(_table->contains(tuple,_universe)) return;
	InternalPredTable* temp = _table;
	_table = _table->add(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void PredTable::remove(const ElementTuple& tuple) {
	if(!_table->contains(tuple,_universe)) return;
	InternalPredTable* temp = _table;
	_table = _table->remove(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

TableIterator PredTable::begin() const {
	TableIterator ti(_table->begin());
	return ti;
}

void InternalPredTable::decrementRef() {
	--_nrRefs;
	if(_nrRefs == 0) delete(this);
}

void InternalPredTable::incrementRef() {
	++_nrRefs;
}

ProcInternalPredTable::~ProcInternalPredTable() {
}

bool ProcInternalPredTable::finite() const {
	return _universe.finite();
}

bool ProcInternalPredTable::empty() const {
	return _universe.empty();
}

bool ProcInternalPredTable::approxfinite() const {
	return _universe.approxfinite();
}

bool ProcInternalPredTable::approxempty() const {
	return _universe.approxempty();
}

bool ProcInternalPredTable::contains(const ElementTuple& tuple) const {
	if(_universe.contains(tuple)) {
		return LuaConnection::predcall(_procedure,tuple);
	}
	else return false;
}

InternalPredTable* ProcInternalPredTable::add(const ElementTuple& ) {
	// TODO
	return 0;
}

InternalPredTable* ProcInternalPredTable::remove(const ElementTuple& ) {
	// TODO
	return 0;
}

InternalTableIterator* ProcInternalPredTable::begin() const {
	// TODO
	return 0;
}

InverseInternalPredTable::~InverseInternalPredTable() {
	_invtable->decrementRef();
}

/**
 *		Returns true iff the table is finite
 */
bool InverseInternalPredTable::finite() const {
	if(approxfinite()) return true;
	if(_universe.finite()) return true;
	else if(_invtable->finite()) return false;
	else {
		notyetimplemented("Exact finiteness test on inverse predicate tables");	
		return approxempty();
	}
}

/**
 *		Returns true iff the table is empty
 */
bool InverseInternalPredTable::empty() const {
	if(approxempty()) return true;
	if(_universe.empty()) return true;
	if(finite()) {
		TableIterator ti = TableIterator(begin());
		return !(ti.hasNext());
	}
	else {
		notyetimplemented("Exact emptyness test on inverse predicate tables");	
		return approxempty();
	}
}

/**
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
bool InverseInternalPredTable::approxfinite() const {
	return _universe.approxfinite();
}

/**
 *		Returns false if the table is non-empty. May return true if the table is empty.
 */
bool InverseInternalPredTable::approxempty() const {
	return _universe.approxempty();
}

/**
 *	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool InverseInternalPredTable::contains(const ElementTuple& tuple) const {
	if(_invtable->contains(tuple)) return false;
	else if(_universe.contains(tuple)) return true;
	else return false; 
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
	if(!contains(tuple)) {
		if(_nrRefs > 1) {
			InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable,_universe);
			InternalPredTable* temp = newtable->add(tuple); assert(temp == newtable);
			return newtable;
		}
		else {
			InternalPredTable* temp = _invtable->remove(tuple);
			if(temp != _invtable) {
				_invtable->decrementRef();
				temp->incrementRef();
				_invtable = temp;
			}
			return this;
		}
	}
	else return this;
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
	if(contains(tuple)) {
		if(_nrRefs > 1) {
			InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable,_universe);
			InternalPredTable* temp = newtable->remove(tuple); assert(temp == newtable);
			return newtable;
		}
		else {
			InternalPredTable* temp = _invtable->add(tuple);
			if(temp != _invtable) {
				_invtable->decrementRef();
				temp->incrementRef();
				_invtable = temp;
			}
			return this;
		}
	}
	else return this;
}

InternalTableIterator* InverseInternalPredTable::begin() const {
	vector<SortIterator> vsi;
	for(vector<SortTable*>::const_iterator it = _universe.tables().begin(); it != _universe.tables().end(); ++it) {
		vsi.push_back((*it)->sortbegin());
	}
	return new InverseInternalIterator(vsi,_invtable);
}

/****************
	SortTable
****************/

SortTable::SortTable(InternalSortTable* table) : _table(table) {
	_table->incrementRef();
}

SortTable::~SortTable() {
	_table->decrementRef();
}

TableIterator SortTable::begin() const {
	return TableIterator(_table->begin());
}

SortIterator SortTable::sortbegin() const {
	return SortIterator(_table->sortbegin());
}

void SortTable::interntable(InternalSortTable* table) {
	_table->decrementRef();
	_table = table;
	_table->incrementRef();
}

/****************
	FuncTable
****************/

FuncTable::FuncTable(InternalFuncTable* table) : _table(table) {
	_table->incrementRef();
}

FuncTable::~FuncTable() {
	_table->decrementRef();
}

void FuncTable::add(const ElementTuple& tuple) {
	if(_table->contains(tuple)) return;
	InternalFuncTable* temp = _table;
	_table = _table->add(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void FuncTable::remove(const ElementTuple& tuple) {
	if(!_table->contains(tuple)) return;
	InternalFuncTable* temp = _table;
	_table = _table->remove(tuple);
	if(temp != _table) {
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
	assert(tuple.size() == arity()+1);
	ElementTuple key = tuple;
	const DomainElement* value = key.back();
	key.pop_back();
	const DomainElement* computedvalue = operator[](key);
	return value == computedvalue;
}

TableIterator FuncTable::begin() const {
	TableIterator ti(_table->begin());
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
 *	- cf	: if true (false), ctpf stores the certainly false (possibly true) tuples
 *	- univ	: all possible domain elements of the sorts of the columns of the table
 */
PredInter::PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf) {
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->interntable(),ctpf->universe()));
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->interntable(),ctpf->universe()));
	if(ct)	{ _ct = ctpf; _pf = inverseCtpf; } else	{ _pf = ctpf; _ct = inverseCtpf; }
	if(cf)	{ _cf = cfpt; _pt = inverseCfpt; } else	{ _pt = cfpt; _cf = inverseCfpt; }
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
	PredTable* cfpt = new PredTable(ctpf->interntable());
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->interntable(),ctpf->universe()));
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->interntable(),cfpt->universe()));
	if(ct) {
		_ct = ctpf; 
		_pt = cfpt;
		_cf = inverseCtpf; 
		_pf = inverseCfpt;
	}
	else {
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
	delete(_ct); 
	delete(_cf);
	delete(_pt);
	delete(_pf);
}

/**
 * \brief Returns true iff the tuple is true or inconsistent according to the predicate interpretation
 */
inline bool PredInter::istrue(const ElementTuple& tuple) const {
	return _ct->contains(tuple);
}

/**
 * \brief Returns true iff the tuple is false or inconsistent according to the predicate interpretation
 */
inline bool PredInter::isfalse(const ElementTuple& tuple) const {
	if(!_cf->contains(tuple)) {
		return !(_cf->universe().contains(tuple));
	}
	return true;
}

/**
 * \brief Returns true iff the tuple is unknown according to the predicate interpretation
 */
inline bool PredInter::isunknown(const ElementTuple& tuple) const {
	if(approxtwovalued()) return false;
	else {
		return !(isfalse(tuple) || istrue(tuple));
	}
}

/**
 * \brief Returns true iff the tuple is inconsistent according to the predicate interpretation
 */
inline bool PredInter::isinconsistent(const ElementTuple& tuple) const {
	if(approxtwovalued()) return false;
	else return (isfalse(tuple) && istrue(tuple));
}

/**
 * \brief Returns false is the interpretation is not two-valued. May return true if it is two-valued.
 *
 * NOTE: Simple check if _ct == _pt
 */
bool PredInter::approxtwovalued() const {
	return _ct->interntable() == _pt->interntable();
}

void PredInter::ct(PredTable* t) {
	delete(_ct);
	delete(_pf);
	_ct = t;
	_pf = new PredTable(new InverseInternalPredTable(t->interntable(),t->universe()));
}

void PredInter::cf(PredTable* t) {
	delete(_cf);
	delete(_pt);
	_cf = t;
	_pt = new PredTable(new InverseInternalPredTable(t->interntable(),t->universe()));
}

void PredInter::pt(PredTable* t) {
	delete(_pt);
	delete(_cf);
	_pt = t;
	_cf = new PredTable(new InverseInternalPredTable(t->interntable(),t->universe()));
}

void PredInter::pf(PredTable* t) {
	delete(_pf);
	delete(_ct);
	_pf = t;
	_ct = new PredTable(new InverseInternalPredTable(t->interntable(),t->universe()));
}

void PredInter::ctpt(PredTable* t) {
	ct(t);
	PredTable* npt = new PredTable(t->interntable());
	pt(npt);
}

PredInter* PredInter::clone(const Universe& univ) const {
	PredTable* nctpf;
	bool ct;
	if(typeid(*(_pf->interntable())) == typeid(InverseInternalPredTable)) {
		nctpf = new PredTable(_ct->interntable());
		ct = true;
	}
	else {
		nctpf = new PredTable(_pf->interntable());
		ct = false;
	}
	if(approxtwovalued) return new PredInter
}

PredInter* EqualInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	EqualInternalPredTable* eip = new EqualInternalPredTable(univ);
	PredTable* ct = new PredTable(eip);
	return new PredInter(ct,true);
}

PredInter* StrLessThanInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	StrLessInternalPredTable* eip = new StrLessInternalPredTable(univ);
	PredTable* ct = new PredTable(eip);
	return new PredInter(ct,true);
}

PredInter* StrGreaterThanInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	StrGreaterInternalPredTable* eip = new StrGreaterInternalPredTable(univ);
	PredTable* ct = new PredTable(eip);
	return new PredInter(ct,true);
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

FuncInter::FuncInter(FuncTable* ft) : _functable(ft) {
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft,true));
	_graphinter = new PredInter(ct,true);
}

FuncInter::~FuncInter() {
	if(_functable) delete(_functable);
	delete(_graphinter);
}

void FuncInter::graphinter(PredInter* pt) {
	delete(_graphinter);
	_graphinter = pt;
	if(_functable) delete(_functable);
	_functable = 0;
}

void FuncInter::functable(FuncTable* ft) {
	delete(_functable);
	delete(_graphinter);
	_functable = ft;
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft,true));
	_graphinter = new PredInter(ct,true);
}

FuncInter* MinInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	ElementTuple t;
	ElementFunc ef; ef[t] = st->first();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef,univ);
	FuncTable* ft = new FuncTable(ift);
	return new FuncInter(ft);
}

FuncInter* MaxInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	ElementTuple t;
	ElementFunc ef; ef[t] = st->last();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef,univ);
	FuncTable* ft = new FuncTable(ift);
	return new FuncInter(ft);
}

FuncInter* SuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2,st);
	FuncTable* ft; // TODO
	return new FuncInter(ft);
}

FuncInter* InvSuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2,st);
	FuncTable* ft; // TODO
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
		PredTable* t1 = new PredTable(new EnumeratedInternalPredTable(univ));
		PredTable* t2 = new PredTable(new EnumeratedInternalPredTable(univ));
		return new PredInter(t1,t2,true,true);
	}

	FuncInter* leastFuncInter(const Universe& univ) {
		PredInter* pt = leastPredInter(univ);
		return new FuncInter(pt);
	}

}

/*****************
	Structures
*****************/

/** Destructor **/

Structure::~Structure() {
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ++it) 
		delete(it->second);
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it)
		delete(it->second);
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it)
		delete(it->second);
}

Structure* Structure::clone() const {
	Structure* s = new Structure("",ParseInfo());
	s->vocabulary(_vocabulary);
	for(map<Sort*,SortTable*>::const_iterator it = _sortinter.begin(); it != _sortinter.end(); ++it) {
		s->inter(it->first)->interntable(it->second->interntable());
	}
	for(map<Predicate*,PredInter*>::const_iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		s->inter(it->first,it->second->clone(s->inter(it->first)->universe()));
	}
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		s->inter(it->first,it->second->clone(s->inter(it->first)->universe()));
	}
	return s;
}

/**
 * This method changes the vocabulary of the structure
 * All tables of symbols that do not occur in the new vocabulary are deleted.
 * Empty tables are created for symbols that occur in the new vocabulary, but did not occur in the old one.
 */
void Structure::vocabulary(Vocabulary* v) {
	_vocabulary = v;
	// Delete tables for symbols that do not occur anymore
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ) {
		map<Sort*,SortTable*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) {
			delete(jt->second);
			_sortinter.erase(jt);
		}
	}
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ) {
		map<Predicate*,PredInter*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) {
			delete(jt->second);
			_predinter.erase(jt);
		}
	}
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ) {
		map<Function*,FuncInter*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) {
			delete(jt->second);
			_funcinter.erase(jt);
		}
	}
	// Create empty tables for new symbols
	for(map<string,set<Sort*> >::const_iterator it = _vocabulary->firstsort(); it != _vocabulary->lastsort(); ++it) {
		for(set<Sort*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			if(_sortinter.find(*jt) == _sortinter.end()) {
				SortTable* st = new SortTable(new EnumeratedInternalSortTable());
				_sortinter[*jt] = st;
				vector<SortTable*> univ(1,st);
				PredTable* pt = new PredTable(new SortInternalPredTable(st,true));
				_predinter[(*jt)->pred()] = new PredInter(pt,true,univ);
			}
		}
	}
	for(map<string,Predicate*>::const_iterator it = _vocabulary->firstpred(); it != _vocabulary->lastpred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for(set<Predicate*>::iterator jt = sp.begin(); jt != sp.end(); ++jt) {
			if(_predinter.find(*jt) == _predinter.end()) {
				vector<SortTable*> univ;
				for(vector<Sort*>::const_iterator kt = (*jt)->sorts().begin(); kt != (*jt)->sorts().end(); ++kt) {
					univ.push_back(_sortinter[*kt]);
				}
				_predinter[*jt] = TableUtils::leastPredInter(Universe(univ));
			}
		}
	}
	for(map<string,Function*>::const_iterator it = _vocabulary->firstfunc(); it != _vocabulary->lastfunc(); ++it) {
		set<Function*> sf = it->second->nonbuiltins();
		for(set<Function*>::iterator jt = sf.begin(); jt != sf.end(); ++jt) {
			if(_funcinter.find(*jt) == _funcinter.end()) {
				vector<SortTable*> univ;
				for(vector<Sort*>::const_iterator kt = (*jt)->sorts().begin(); kt != (*jt)->sorts().end(); ++kt) {
					univ.push_back(_sortinter[*kt]);
				}
				_funcinter[(*jt)] = TableUtils::leastFuncInter(Universe(univ));
			}
		}
	}
}

void Structure::inter(Sort* s, SortTable* d) {
	delete(_sortinter[s]);
	_sortinter[s] = d;
}

void Structure::inter(Predicate* p, PredInter* i) {
	delete(_predinter[p]);
	_predinter[p] = i;
}

void Structure::inter(Function* f, FuncInter* i) {
	delete(_funcinter[f]);
	_funcinter[f] = i;
}

/*
void computescore(Sort* s, map<Sort*,unsigned int>& scores) {
	if(scores.find(s) == scores.end()) {
		unsigned int sc = 0;
		for(set<Sort*>::const_iterator it = s->parents().begin(); it != s->parents().end(); ++it) {
			computescore(*it,scores);
			if(scores[*it] >= sc) sc = scores[*it] + 1;
		}
		scores[s] = sc;
	}
}

void Structure::autocomplete() {
	bool message = false;
	// Adding elements from predicate interpretations to sorts
	for(map<Predicate*,PredInter*>::const_iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		Predicate* p = it->first;
		vector<SortTable*> tables(p->arity());
		for(unsigned int m = 0; m < p->arity(); ++m) tables[m] = inter(p->sort(m));
		PredTable* pt = it->second->ctpf();
		for(unsigned int r = 0; r < pt->size(); ++r) {
			for(unsigned int c = 0; c < pt->arity(); ++c) {
				if(!(tables[c]->contains(pt->element(r,c),pt->type(c)))) {
					if(p->sort(c)->builtin()) {
						string el = ElementUtil::ElementToString(pt->element(r,c),pt->type(c));
						Error::predelnotinsort(el,p->name(),p->sort(c)->name(),_name);
					}
					else {
						if(!message) {
							cerr << "Completing structure " << _name << ".\n";
							message = true;
						}
						addElement(pt->element(r,c),pt->type(c),p->sort(c));
						tables[c] = inter(p->sort(c));
					}
				}
			}
		}
		if(it->second->ctpf() != it->second->cfpt()) {
			pt = it->second->cfpt();
			for(unsigned int r = 0; r < pt->size(); ++r) {
				for(unsigned int c = 0; c < pt->arity(); ++c) {
					if(!(tables[c]->contains(pt->element(r,c),pt->type(c)))) {
						if(p->sort(c)->builtin()) {
							string el = ElementUtil::ElementToString(pt->element(r,c),pt->type(c));
							Error::predelnotinsort(el,p->name(),p->sort(c)->name(),_name);
						}
						else {
							if(!message) {
								cerr << "Completing structure " << _name << ".\n";
								message = true;
							}
							addElement(pt->element(r,c),pt->type(c),p->sort(c));
							tables[c] = inter(p->sort(c));
						}
					}	
				}
			}
		}
	}
	// Adding elements from function interpretations to sorts
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		Function* f = it->first;
		vector<SortTable*> tables(f->arity()+1);
		for(unsigned int m = 0; m < f->arity()+1; ++m) tables[m] = inter(f->sort(m));
		PredTable* pt = it->second->predinter()->ctpf();
		for(unsigned int r = 0; r < pt->size(); ++r) {
			for(unsigned int c = 0; c < pt->arity(); ++c) {
				if(!(tables[c]->contains(pt->element(r,c),pt->type(c)))) {
					if(f->sort(c)->builtin()) {
						string el = ElementUtil::ElementToString(pt->element(r,c),pt->type(c));
						Error::funcelnotinsort(el,f->name(),f->sort(c)->name(),_name);
					}
					else {
						if(!message) {
							cerr << "Completing structure " << _name << ".\n";
							message = true;
						}
						addElement(pt->element(r,c),pt->type(c),f->sort(c));
						tables[c] = inter(f->sort(c));
					}
				}
			}
		}
		if(it->second->predinter()->ctpf() != it->second->predinter()->cfpt()) {
			pt = it->second->predinter()->cfpt();
			for(unsigned int r = 0; r < pt->size(); ++r) {
				for(unsigned int c = 0; c < pt->arity(); ++c) {
					if(!(tables[c]->contains(pt->element(r,c),pt->type(c)))) {
						if(f->sort(c)->builtin()) {
							string el = ElementUtil::ElementToString(pt->element(r,c),pt->type(c));
							Error::funcelnotinsort(el,f->name(),f->sort(c)->name(),_name);
						}
						else {
							if(!message) {
								cerr << "Completing structure " << _name << ".\n";
								message = true;
							}
							addElement(pt->element(r,c),pt->type(c),f->sort(c));
							tables[c] = inter(f->sort(c));
						}
					}	
				}
			}
		}
	}

	// Adding elements from subsorts to supersorts
	map<Sort*,unsigned int> scores;
	for(unsigned int n = 0; n < _vocabulary->nrNBSorts(); ++n) {
		computescore(_vocabulary->nbsort(n),scores);
	}
	map<unsigned int,vector<Sort*> > invscores;
	for(map<Sort*,unsigned int>::const_iterator it = scores.begin(); it != scores.end(); ++it) {
		if(_vocabulary->contains(it->first)) {
			invscores[it->second].push_back(it->first);
		}
	}
	for(map<unsigned int,vector<Sort*> >::const_reverse_iterator it = invscores.rbegin(); it != invscores.rend(); ++it) {
		for(unsigned int n = 0; n < (it->second).size(); ++n) {
			Sort* s = (it->second)[n];
			if(inter(s)->finite()) {
				set<Sort*> notextend;
				notextend.insert(s);
				vector<Sort*> toextend;
				vector<Sort*> tocheck;
				while(!(notextend.empty())) {
					Sort* e = *(notextend.begin());
					for(unsigned int p = 0; p < e->nrParents(); ++p) {
						Sort* sp = e->parent(p);
						if(_vocabulary->contains(sp)) {
							if(sp->builtin()) tocheck.push_back(sp);
							else toextend.push_back(sp); 
						}
						else {
							notextend.insert(sp);
						}
					}
					notextend.erase(e);
				}
				SortTable* st = inter(s);
				for(unsigned int m = 0; m < st->size(); ++m) {
					for(unsigned int k = 0; k < toextend.size(); ++k) {
						if(!(inter(toextend[k])->contains(st->element(m),st->type()))) {
							if(!message) {
								cerr << "Completing structure " << _name << ".\n";
								message = true;
							}
							addElement(st->element(m),st->type(),toextend[k]);
						}
					}
					for(unsigned int k = 0; k < tocheck.size(); ++k) {
						if(!inter(tocheck[k])->contains(st->element(m),st->type())) {
							string el = ElementUtil::ElementToString(st->element(m),st->type());
							Error::sortelnotinsort(el,s->name(),tocheck[k]->name(),_name);
						}
					}
				}
			}
		}
	}
}
*/

void Structure::autocomplete() {
	// do nothing?
}

void Structure::addStructure(AbstractStructure* ) {
	// TODO
}

void Structure::functioncheck() {
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		Function* f = it->first;
		FuncInter* ft = it->second;
		PredInter* pt = ft->graphinter();
		PredTable* ct = pt->ct();
		// Check if the interpretation is indeed a function
		bool isfunc = true;
		StrictWeakNTupleEquality eq(f->arity());
		TableIterator it = ct->begin();
		if(it.hasNext()) {
			TableIterator jt = ct->begin(); ++jt;
			for(; jt.hasNext(); ++it, ++jt) {
				if(eq(*it,*jt)) {
					const ElementTuple& tuple = *it;
					vector<string> vstr;
					for(unsigned int c = 0; c < f->arity(); ++c) 
						vstr.push_back(tuple[c]->to_string());
					Error::notfunction(f->name(),name(),vstr);
					do { ++it; ++jt; } while(jt.hasNext() && eq(*it,*jt));
					isfunc = false;
				}
			}
		}
		// Check if the interpretation is total
		if(isfunc && !(f->partial()) && ft->approxtwovalued() && ct->approxfinite()) {
			vector<SortTable*> vst;
			vector<bool> linked;
			for(unsigned int c = 0; c < f->arity(); ++c) {
				vst.push_back(inter(f->insort(c)));
				linked.push_back(true);
			}
			PredTable spt(new FullInternalPredTable(vst,linked));
			it = spt.begin();
			TableIterator jt = ct->begin();
			for(; it.hasNext() && jt.hasNext(); ++it, ++jt) {
				if(!eq(*it,*jt)) {
					break;
				}
			}
			if(it.hasNext() || jt.hasNext()) {
				Error::nottotal(f->name(),name());
			}
		}
	}
}

SortTable* Structure::inter(Sort* s) const {
	if(s->builtin()) return s->interpretation();
	else {
		map<Sort*,SortTable*>::const_iterator it = _sortinter.find(s);
		assert(it != _sortinter.end());
		return it->second;
	}
}

PredInter* Structure::inter(Predicate* p) const {
	if(p->builtin()) return p->interpretation(this);
	else {
		map<Predicate*,PredInter*>::const_iterator it = _predinter.find(p);
		assert(it != _predinter.end());
		return it->second;
	}
}

FuncInter* Structure::inter(Function* f) const {
	if(f->builtin()) return f->interpretation(this);
	else {
		map<Function*,FuncInter*>::const_iterator it = _funcinter.find(f);
		assert(it != _funcinter.end());
		return it->second;
	}
}

PredInter* Structure::inter(PFSymbol* s) const {
	if(typeid(*s) == typeid(Predicate)) return inter(dynamic_cast<Predicate*>(s));
	else return inter(dynamic_cast<Function*>(s))->graphinter();
}

void Structure::clean() {
	// TODO
}
