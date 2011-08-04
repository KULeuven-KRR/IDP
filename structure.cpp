/************************************
	structure.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cmath> // double std::abs(double) and double std::pow(double,double)
#include <cstdlib> // int std::abs(int)
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "error.hpp"
#include "fobdd.hpp"
#include "luaconnection.hpp" //FIXME break connection with lua!
using namespace std;

/**********************
	Domain elements
**********************/

ostream& operator<<(ostream& out, const DomainElementType& domeltype) {
	string DomElTypeStrings[4] = { "int", "double", "string", "compound" };
	return out << DomElTypeStrings[domeltype];
}

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

DomainElementType DomainElement::type() const {
	return _type;
}

DomainElementValue DomainElement::value() const {
	return _value;
}

ostream& DomainElement::put(ostream& output) const {
	switch(_type) {
		case DET_INT:
			output << toString(_value._int);
			break;
		case DET_DOUBLE:
			output << toString(_value._double);
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

ostream& operator<<(ostream& output, const ElementTuple& tuple) {
	output << '(';
	for(ElementTuple::const_iterator it = tuple.begin(); it != tuple.end(); ++it) {
		output << **it;
		if(it != tuple.end()-1) output << ',';
	}
	output << ')';
	return output;
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

Compound::Compound(Function* function, const ElementTuple& arguments) :
	_function(function), _arguments(arguments) { 
	assert(function != 0); 
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

ostream& DomainAtom::put(ostream& output) const {
	output << *_symbol;
	if(typeid(*_symbol) == typeid(Predicate)) {
		if(!_symbol->sorts().empty()) {
			output << '(' << *_args[0];
			for(unsigned int n = 1; n < _args.size(); ++n) {
				output << ',' << *_args[n];
			}
			output << ')';
		}
	}
	else {
		Function* f = dynamic_cast<Function*>(_symbol);
		if(f->arity() > 0) {
			output << '(' << *_args[0];
			for(unsigned int n = 1; n < f->arity(); ++n) {
				output << ',' << *_args[n];
			}
			output << ") = " << *(_args.back());
		}
	}
	return output;
}

string DomainAtom::to_string() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
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
	for(map<Function*,map<ElementTuple,Compound*> >::iterator it = _compounds.begin(); it != _compounds.end(); ++it) {
		for(map<ElementTuple,Compound*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
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
	if(!certnotdouble && isDouble(*value)) return create(toDouble(*value),false);

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

DomainAtomFactory::~DomainAtomFactory() {
	for(map<PFSymbol*,map<ElementTuple,DomainAtom*> >::iterator it = _atoms.begin(); it != _atoms.end(); ++it) {
		for(map<ElementTuple,DomainAtom*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			delete(jt->second);
		}
	}
}

DomainAtomFactory* DomainAtomFactory::_instance = 0;

DomainAtomFactory* DomainAtomFactory::instance() {
	if(!_instance) _instance = new DomainAtomFactory();
	return _instance;
}

const DomainAtom* DomainAtomFactory::create(PFSymbol* s, const ElementTuple& tuple) {
	map<PFSymbol*,map<ElementTuple,DomainAtom*> >::const_iterator it = _atoms.find(s);
	if(it != _atoms.end()) {
		map<ElementTuple,DomainAtom*>::const_iterator jt = it->second.find(tuple); 
		if(jt != it->second.end()) return jt->second;
	}
	DomainAtom* newatom = new DomainAtom(s,tuple);
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
	if(this != &tab) {
		delete(_iterator);
		_iterator = tab.iterator()->clone();

	}
	return *this;
}

bool TableIterator::hasNext() const {
	return _iterator->hasNext();
}

const ElementTuple& TableIterator::operator*() const {
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

bool SortIterator::hasNext() const {
	return _iterator->hasNext();
}

const DomainElement* SortIterator::operator*() const {
	return _iterator->operator*();
}

SortIterator::~SortIterator() {
	delete(_iterator);
}

SortIterator& SortIterator::operator++() {
	_iterator->operator++();
	return *this;
}

CartesianInternalTableIterator::CartesianInternalTableIterator(const vector<SortIterator>& vsi, const vector<SortIterator>& low, bool h) : _iterators(vsi), _lowest(low), _hasNext(h) {
	if(h) {
		for(vector<SortIterator>::iterator it = _iterators.begin(); it != _iterators.end(); ++it) {
			if(!it->hasNext()) {
				_hasNext = false;
				break;
			}
		}
	}
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

const ElementTuple&	InternalFuncIterator::operator*() const {
	ElementTuple e = *_curr;
	e.push_back(_function->operator[](e));
	_deref.push_back(e);
	return _deref.back();
}

void InternalFuncIterator::operator++() {
	++_curr;
	while(_curr.hasNext() && !(_function->operator[](*_curr))) ++_curr;
}

InternalFuncIterator::InternalFuncIterator(const InternalFuncTable* f, const Universe& univ) : _function(f) {
	vector<SortIterator> vsi1;
	vector<SortIterator> vsi2;
	for(unsigned int n = 0; n < univ.arity()-1; ++n) {
		vsi1.push_back(univ.tables()[n]->sortbegin());
		vsi2.push_back(univ.tables()[n]->sortbegin());
	}
	_curr = TableIterator(new CartesianInternalTableIterator(vsi1,vsi2,true));
	if(_curr.hasNext() && !_function->operator[](*_curr)) operator++();
}

const ElementTuple&	ProcInternalTableIterator::operator*() const {
	return *_curr;
}

void ProcInternalTableIterator::operator++() {
	++_curr;
	while(_curr.hasNext() && !(_predicate->contains(*_curr,_univ))) ++_curr;
}

ProcInternalTableIterator::ProcInternalTableIterator(const InternalPredTable* p, const Universe& univ) :  _univ(univ), _predicate(p) {
	vector<SortIterator> vsi1;
	vector<SortIterator> vsi2;
	for(unsigned int n = 0; n < univ.arity(); ++n) {
		vsi1.push_back(univ.tables()[n]->sortbegin());
		vsi2.push_back(univ.tables()[n]->sortbegin());
	}
	_curr = TableIterator(new CartesianInternalTableIterator(vsi1,vsi2,true));
	if(!_predicate->contains(*_curr,_univ)) operator++();
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

UnionInternalIterator::UnionInternalIterator(const vector<TableIterator>& its, const vector<InternalPredTable*>& outs, const Universe& univ) :
	_iterators(its), _universe(univ), _outtables(outs) {
	setcurriterator();
}

UnionInternalIterator* UnionInternalIterator::clone() const {
	return new UnionInternalIterator(_iterators,_outtables,_universe);
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
		if(_curr[n].hasNext()) {
			_currtuple[n] = *(_curr[n]);
		}
		else {
			_end = true; break;
		}
	}
	if(!_end) {
		if(_outtable->contains(_currtuple,_universe)) operator++();
	}
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
	do {
		int pos = _curr.size() - 1;
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

UNAInternalIterator::UNAInternalIterator(const vector<SortIterator>& its, Function* f) :
	_curr(its), _lowest(its), _function(f), _end(false), _currtuple(its.size())  {
	for(unsigned int n = 0; n < _curr.size(); ++n) {
		if(_curr[n].hasNext()) {
			_currtuple[n] = *(_curr[n]);
		}
		else {
			_end = true; break;
		}
	}
}

UNAInternalIterator::UNAInternalIterator(const vector<SortIterator>& curr, const vector<SortIterator>& low, Function* f, bool end) :
	_curr(curr), _lowest(low), _function(f), _end(end), _currtuple(curr.size()) {
	for(unsigned int n = 0; n < _curr.size(); ++n) {
		if(_curr[n].hasNext()) _currtuple[n] = *(_curr[n]);
	}
}

UNAInternalIterator* UNAInternalIterator::clone() const {
	return new UNAInternalIterator(_curr,_lowest,_function,_end);
}

bool UNAInternalIterator::hasNext() const {
	return !_end;
}

const ElementTuple& UNAInternalIterator::operator*() const {
	_deref.push_back(_currtuple);
	_deref.back().push_back(DomainElementFactory::instance()->create(_function,_deref.back()));
	return _deref.back();
}

void UNAInternalIterator::operator++() {
	int pos = _curr.size() - 1;
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

tablesize Universe::size() const {
	unsigned int currsize = 1;
	for(vector<SortTable*>::const_iterator it = _tables.begin(); it != _tables.end(); ++it) {
		tablesize ts = (*it)->size();
		if(ts.first) currsize = currsize * ts.second;
		else return tablesize(false,0);
	}
	return tablesize(true,currsize);
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

bool StrictWeakNTupleOrdering::operator()(const ElementTuple& t1, const ElementTuple& t2) const {
	for(unsigned int n = 0; n < _arity; ++n) {
		if(*(t1[n]) < *(t2[n])) return true;
		else if(*(t1[n]) > *(t2[n])) return false;
	}
	return false;
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

tablesize FuncInternalPredTable::size(const Universe& ) const {
	return _table->size();
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
	_intables[0]->incrementRef();
	_outtables.push_back(new EnumeratedInternalPredTable());
	_outtables[0]->incrementRef();
}

UnionInternalPredTable::UnionInternalPredTable(const vector<InternalPredTable*>& intabs, const vector<InternalPredTable*>& outtabs) : InternalPredTable(), _intables(intabs), _outtables(outtabs) { 
	for(vector<InternalPredTable*>::const_iterator it = intabs.begin(); it != intabs.end(); ++it) {
		(*it)->incrementRef();
	}
	for(vector<InternalPredTable*>::const_iterator it = outtabs.begin(); it != outtabs.end(); ++it) {
		(*it)->incrementRef();
	}
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
		return temp;
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
		return temp;
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

BDDInternalPredTable::BDDInternalPredTable(const FOBDD* bdd, FOBDDManager* manager, const vector<Variable*>& vars, AbstractStructure* str) :
	_manager(manager), _bdd(bdd), _vars(vars), _structure(str->clone()) { 
}

bool BDDInternalPredTable::finite(const Universe&) const {
	// TODO
	return false;
}

bool BDDInternalPredTable::empty(const Universe&) const {
	// TODO
	return false;
}

bool BDDInternalPredTable::approxfinite(const Universe&) const {
	// TODO
	return false;
}

bool BDDInternalPredTable::approxempty(const Universe&) const {
	// TODO
	return false;
}

tablesize BDDInternalPredTable::size(const Universe&) const {
	// TODO
	return tablesize(false,0);
}

bool BDDInternalPredTable::contains(const ElementTuple& , const Universe& ) const {
	// TODO
	return false;
}

InternalPredTable* BDDInternalPredTable::add(const ElementTuple&) {
	// TODO
	return 0;
}

InternalPredTable* BDDInternalPredTable::remove(const ElementTuple&) {
	// TODO
	return 0;
}

InternalTableIterator* BDDInternalPredTable::begin(const Universe&) const {
	// TODO
	return 0;
}


/**
 * \brief	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EnumeratedInternalPredTable::contains(const ElementTuple& tuple, const Universe&) const {
	if(_table.empty()) return false;
	else return _table.find(tuple) != _table.end();
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
	if(_nrRefs <= 1) {
		_table.insert(tuple);
		return this;
	}
	else {
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
	if(it != _table.end()) {
		if(_nrRefs == 1) {
			_table.erase(it);
			return this;
		}
		else {
			SortedElementTable newtable = _table;
			SortedElementTable::iterator nt = newtable.find(tuple);
			newtable.erase(nt);
			return new EnumeratedInternalPredTable(newtable);
		}
	}
	else return this;
}

/**
 * \brief Returns an iterator on the first tuple of the table
 */
InternalTableIterator* EnumeratedInternalPredTable::begin(const Universe&) const {
	return new EnumInternalIterator(_table.begin(),_table.end());
}

ComparisonInternalPredTable::ComparisonInternalPredTable() { }

ComparisonInternalPredTable::~ComparisonInternalPredTable() { }

InternalPredTable* ComparisonInternalPredTable::add(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->add(tuple);
	if(temp != upt) delete(upt);
	return temp;
}

InternalPredTable* ComparisonInternalPredTable::remove(const ElementTuple& tuple) {
	UnionInternalPredTable* upt = new UnionInternalPredTable();
	upt->addInTable(this);
	InternalPredTable* temp = upt->remove(tuple);
	if(temp != upt) delete(upt);
	return temp;
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EqualInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	assert(tuple.size() == 2);
	return (tuple[0] == tuple[1] && univ.contains(tuple));
}

/**
 * \brief	Returns true iff the table is finite
 */
bool EqualInternalPredTable::finite(const Universe& univ) const {
	if(approxfinite(univ)) return true;
	else if(univ.finite()) return true;
	else return false;
}

/**
 * \brief Returns true iff the table is empty
 */
bool EqualInternalPredTable::empty(const Universe& univ) const {
	if(approxempty(univ)) return true;
	else if(univ.empty()) return true;
	else return false;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool EqualInternalPredTable::approxfinite(const Universe& univ) const {
	return univ.approxfinite();
}

/**
 * \brief Returns false if the table is not empty. May return true if the table is empty.
 */
inline bool EqualInternalPredTable::approxempty(const Universe& univ) const {
	return univ.approxempty();
}

tablesize EqualInternalPredTable::size(const Universe& univ) const {
	return univ.tables()[0]->size();
}

InternalTableIterator* EqualInternalPredTable::begin(const Universe& univ) const {
	return new EqualInternalIterator(univ.tables()[0]->sortbegin());
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrLessInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	assert(tuple.size() == 2);
	return (*(tuple[0]) < *(tuple[1]) && univ.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrLessInternalPredTable::finite(const Universe& univ) const {
	if(approxfinite(univ)) return true;
	else return univ.finite();
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrLessInternalPredTable::empty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxfinite(const Universe& univ) const {
	return (univ.approxfinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxempty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

tablesize StrLessInternalPredTable::size(const Universe& univ) const {
	tablesize ts = univ.tables()[0]->size();
	if(ts.first) {
		if(ts.second == 1) return tablesize(true,0);
		else {
			unsigned int n = ts.second * (ts.second - 1) / 2;
			return tablesize(true,n);
		}
	}
	else return ts;
}

InternalTableIterator* StrLessInternalPredTable::begin(const Universe& univ) const {
	return new StrLessThanInternalIterator(univ.tables()[0]->sortbegin());
}

/**
 * \brief Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrGreaterInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	assert(tuple.size() == 2);
	return (*(tuple[0]) > *(tuple[1]) && univ.contains(tuple));
}

/**
 * \brief Returns true iff the table is finite
 */
bool StrGreaterInternalPredTable::finite(const Universe& univ) const {
	if(approxfinite(univ)) return true;
	else return univ.finite(); 
}

/**
 * \brief Returns true iff the table is empty
 */
bool StrGreaterInternalPredTable::empty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxfinite(const Universe& univ) const {
	return (univ.approxfinite());
}

/**
 * \brief Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxempty(const Universe& univ) const {
	SortIterator isi = univ.tables()[0]->sortbegin();
	if(isi.hasNext()) {
		++isi;
		if(isi.hasNext()) return false;
	}
	return true;
}

tablesize StrGreaterInternalPredTable::size(const Universe& univ) const {
	tablesize ts = univ.tables()[0]->size();
	if(ts.first) {
		if(ts.second == 1) return tablesize(true,0);
		else {
			unsigned int n = ts.second * (ts.second - 1) / 2;
			return tablesize(true,n);
		}
	}
	else return ts;
}

InternalTableIterator* StrGreaterInternalPredTable::begin(const Universe& univ) const {
	return new StrGreaterThanInternalIterator(univ.tables()[0]->sortbegin());
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
		return l->value()._int - f->value()._int == (int)_table.size() - 1;
	}
	else return false;
}

InternalSortIterator* EnumeratedInternalSortTable::sortbegin() const {
	return new EnumInternalSortIterator(_table.begin(),_table.end());
}

InternalSortIterator* EnumeratedInternalSortTable::sortiterator(const DomainElement* d) const {
	return new EnumInternalSortIterator(_table.find(d),_table.end());
}

InternalSortTable* EnumeratedInternalSortTable::add(int i1, int i2) {
	if(empty()) return new IntRangeInternalSortTable(i1,i2);
	else if(first()->type() == DET_INT && last()->type() == DET_INT) {
		if(i1 <= first()->value()._int && last()->value()._int <= i2) {
			return new IntRangeInternalSortTable(i1,i2);
		}
		else if(isRange()) {
			if((i1 <= last()->value()._int + 1) && (i2 >= first()->value()._int - 1)) {
				int f = i1 < first()->value()._int ? i1 : first()->value()._int;
				int l = i2 < last()->value()._int ? last()->value()._int : i2;
				return new IntRangeInternalSortTable(f,l);
			}
		}
	}
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
	if(_table.empty()) return 0;
	else return *(_table.begin());
}

const DomainElement* EnumeratedInternalSortTable::last() const {
	if(_table.empty()) return 0;
	else return *(_table.rbegin());
}

InternalSortTable* IntRangeInternalSortTable::add(const DomainElement* d) {
	if(!contains(d)) {
		if(d->type() == DET_INT) {
			if(d->value()._int == _first - 1) {
				if(_nrRefs < 2) { _first = d->value()._int; return this;	}
			}
			else if(d->value()._int == _last +1) {
				if(_nrRefs < 2) { _last = d->value()._int; return this;		}
			}
		}
		EnumeratedInternalSortTable* eist = new EnumeratedInternalSortTable();
		InternalSortTable* ist = eist->add(d);
		InternalSortTable* ist2 = ist->add(_first,_last);
		if(ist2 != eist) delete(eist);
		return ist2;
	}
	else return this;
}

InternalSortTable* IntRangeInternalSortTable::remove(const DomainElement* d) {
	if(contains(d)) {
		if(d->type() == DET_INT) {
			if(d->value()._int == _first) {
				if(_nrRefs < 2) { _first = _first + 1; return this;	}
			}
			else if(d->value()._int == _last) {
				if(_nrRefs < 2) { _last = _last - 1; return this;	}
			}
		}
		EnumeratedInternalSortTable* eist = new EnumeratedInternalSortTable();
		for(int n = _first; n < d->value()._int; ++n) {
			eist->add(DomainElementFactory::instance()->create(n));
		}
		for(int n = d->value()._int + 1; n <= _last; ++n) {
			eist->add(DomainElementFactory::instance()->create(n));
		}
		return eist;
	}
	else return this;
}

InternalSortTable* IntRangeInternalSortTable::add(int i1, int i2) {
	if(i1 <= _last + 1 && i2 >= _first -1) {
		if(_nrRefs > 1) {
			int f = i1 < _first ? i1 : _first;
			int l = i2 > _last ? i2 : _last;
			return new IntRangeInternalSortTable(f,l);
		}
		else {
			_first = i1 < _first ? i1 : _first;
			_last = i2 > _last ? i2 : _last;
			return this;
		}
	}
	else {
		EnumeratedInternalSortTable* eist = new EnumeratedInternalSortTable();
		for(int n = _first; n <= _last; ++n) eist->add(DomainElementFactory::instance()->create(n));
		InternalSortTable* ist = eist->add(i1,i2);
		if(ist != eist) delete(eist);
		return ist;
	}
}

const DomainElement* IntRangeInternalSortTable::first() const {
	return DomainElementFactory::instance()->create(_first);
}

const DomainElement* IntRangeInternalSortTable::last() const {
	return DomainElementFactory::instance()->create(_last);
}

bool IntRangeInternalSortTable::contains(const DomainElement* d) const {
	if(d->type() == DET_INT) {
		return (_first <= d->value()._int && d->value()._int <= _last);
	}
	else return false;
}

InternalSortIterator* IntRangeInternalSortTable::sortbegin() const {
	return new RangeInternalSortIterator(_first,_last);
}

InternalSortIterator* IntRangeInternalSortTable::sortiterator(const DomainElement* d) const {
	return new RangeInternalSortIterator(d->value()._int,_last);
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

bool UnionInternalSortTable::finite() const {
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

InternalSortIterator* UnionInternalSortTable::sortiterator(const DomainElement* ) const {
	notyetimplemented("intermediate sortiterator for UnionInternalSortTable");
	return 0;
}

const DomainElement* UnionInternalSortTable::first() const {
	InternalSortIterator* isi = sortbegin();
	if(isi->hasNext()) {
		const DomainElement* f = *(*isi);
		delete(isi);
		return f;
	}
	else return 0;
}

const DomainElement* UnionInternalSortTable::last() const {
	const DomainElement* result = 0;
	for(vector<SortTable*>::const_iterator it = _intables.begin(); it != _intables.end(); ++it) {
		const DomainElement* temp = (*it)->last();
		if(temp && contains(temp)) {
			if(result) {
				result = *result < *temp ? temp : result;
			}
			else result = temp;
		}
	}
	if(!result) {
		notyetimplemented("Computation of last element of a UnionInternalSortTable");
	}
	return result;
}

bool UnionInternalSortTable::isRange() const {
	notyetimplemented("Exact range test of a UnionInternalSortTable");
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

InternalSortIterator* AllNaturalNumbers::sortiterator(const DomainElement* d) const {
	return new NatInternalSortIterator(d->value()._int);
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

InternalSortIterator* AllIntegers::sortiterator(const DomainElement* d) const {
	return new IntInternalSortIterator(d->value()._int);
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

InternalSortIterator* AllFloats::sortiterator(const DomainElement* d) const {
	double dou = d->type() == DET_DOUBLE ? d->value()._double : double(d->value()._int);
	return new FloatInternalSortIterator(dou);
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

InternalSortIterator* AllStrings::sortiterator(const DomainElement* d) const {
	string str;
	if(d->type() == DET_INT) str = toString(d->value()._int);
	else if(d->type() == DET_DOUBLE) str = toString(d->value()._double);
	else str = *(d->value()._string);
	return new StringInternalSortIterator(str);
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

InternalSortIterator* AllChars::sortiterator(const DomainElement* d) const {
	char c;
	if(d->type() == DET_INT) c = '0' + d->value()._int;
	else c = d->value()._string->operator[](0);
	return new CharInternalSortIterator(c);
}

const DomainElement* AllChars::first() const {
	return DomainElementFactory::instance()->create(StringPointer(string(1,numeric_limits<char>::min())));
}

const DomainElement* AllChars::last() const {
	return DomainElementFactory::instance()->create(StringPointer(string(1,numeric_limits<char>::max())));
}

tablesize AllChars::size() const {
	return tablesize(true,numeric_limits<char>::max() - numeric_limits<char>::min() + 1);
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

bool InternalFuncTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	ElementTuple input = tuple;
	const DomainElement* output = tuple.back();
	input.pop_back();
	const DomainElement* computedoutput = operator[](input);
	if(output == computedoutput) return univ.contains(tuple);
	else return false;
}

ProcInternalFuncTable::~ProcInternalFuncTable() {
}

bool ProcInternalFuncTable::finite(const Universe& univ) const {
	if(empty(univ)) return true;
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(!univ.tables()[n]->finite()) return false;
	}
	return true;
}

bool ProcInternalFuncTable::empty(const Universe& univ) const {
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(univ.tables()[n]->empty()) return true;
	}
	notyetimplemented("Exact finiteness test on procedural function tables");	
	return false;
}

bool ProcInternalFuncTable::approxfinite(const Universe& univ) const {
	if(approxempty(univ)) return true;
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(!univ.tables()[n]->approxfinite()) return false;
	}
	return true;
}

bool ProcInternalFuncTable::approxempty(const Universe& univ) const {
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(univ.tables()[n]->approxempty()) return true;
	}
	return false;
}

const DomainElement* ProcInternalFuncTable::operator[](const ElementTuple& tuple) const {
	return LuaConnection::funccall(_procedure,tuple);
}

InternalFuncTable* ProcInternalFuncTable::add(const ElementTuple& ) {
	notyetimplemented("adding a tuple to a procedural function interpretation");
	return this;
}

InternalFuncTable* ProcInternalFuncTable::remove(const ElementTuple& ) {
	notyetimplemented("removing a tuple from a procedural function interpretation");
	return this;
}

InternalTableIterator* ProcInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

bool UNAInternalFuncTable::finite(const Universe& univ) const {
	if(empty(univ)) return true;
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(!univ.tables()[n]->finite()) return false;
	}
	return true;
}

bool UNAInternalFuncTable::empty(const Universe& univ) const {
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(univ.tables()[n]->empty()) return true;
	}
	notyetimplemented("Exact finiteness test on constructor function tables");	
	return false;
}

bool UNAInternalFuncTable::approxfinite(const Universe& univ) const {
	if(approxempty(univ)) return true;
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(!univ.tables()[n]->approxfinite()) return false;
	}
	return true;
}

bool UNAInternalFuncTable::approxempty(const Universe& univ) const {
	for(unsigned int n = 0; n < univ.tables().size()-1; ++n) {
		if(univ.tables()[n]->approxempty()) return true;
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
	return DomainElementFactory::instance()->create(_function,tuple);
}

InternalFuncTable* UNAInternalFuncTable::add(const ElementTuple& ) {
	notyetimplemented("adding a tuple to a generated function interpretation");
	return this;
}

InternalFuncTable* UNAInternalFuncTable::remove(const ElementTuple& ) {
	notyetimplemented("removing a tuple from a generated function interpretation");
	return this;
}

InternalTableIterator* UNAInternalFuncTable::begin(const Universe& univ) const {
	vector<SortIterator> vsi;
	for(unsigned int n = 0; n < _function->arity(); ++n) {
		vsi.push_back(univ.tables()[n]->sortbegin());
	}
	return new UNAInternalIterator(vsi,_function);
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
		else { 
			assert(false); return this;
		}
	}
	else {
		if(_nrRefs > 1) {
			ElementFunc newtable = _table;
			newtable[key] = value;
			return new EnumeratedInternalFuncTable(newtable);
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
			return new EnumeratedInternalFuncTable(newtable);
		}
		else {
			_table.erase(key);
			return this;
		}
	}
	else return this;
}

InternalTableIterator* EnumeratedInternalFuncTable::begin(const Universe&) const {
	return new EnumInternalFuncIterator(_table.begin(),_table.end());
}

const DomainElement* ModInternalFuncTable::operator[](const ElementTuple& tuple) const {
	int a1 = tuple[0]->value()._int;
	int a2 = tuple[1]->value()._int;
	if(a2 == 0) return 0;
	else return DomainElementFactory::instance()->create(a1 % a2);
}

InternalFuncTable* ModInternalFuncTable::add(const ElementTuple& ) {
	assert(false);
	return 0;
}

InternalFuncTable* ModInternalFuncTable::remove(const ElementTuple& ) {
	assert(false);
	return 0;
}

InternalTableIterator* ModInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

const DomainElement* ExpInternalFuncTable::operator[](const ElementTuple& tuple) const {
	double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
	double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
	return DomainElementFactory::instance()->create(pow(a1,a2),false);
}

InternalFuncTable* ExpInternalFuncTable::add(const ElementTuple& ) {
	assert(false);
	return 0;
}

InternalFuncTable* ExpInternalFuncTable::remove(const ElementTuple& ) {
	assert(false);
	return 0;
}

InternalTableIterator* ExpInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

InternalFuncTable* IntFloatInternalFuncTable::add(const ElementTuple& ) {
	assert(false);
	return 0;
}

InternalFuncTable* IntFloatInternalFuncTable::remove(const ElementTuple& ) {
	assert(false);
	return 0;
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

InternalTableIterator* PlusInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
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

InternalTableIterator* MinusInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
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

InternalTableIterator* TimesInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

const DomainElement* DivInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(_int) {
		int a1 = tuple[0]->value()._int;
		int a2 = tuple[1]->value()._int;
		if(a2 == 0) return 0;
		else return DomainElementFactory::instance()->create(a1 / a2);
	}
	else {
		double a1 = tuple[0]->type() == DET_DOUBLE ? tuple[0]->value()._double : double(tuple[0]->value()._int);
		double a2 = tuple[1]->type() == DET_DOUBLE ? tuple[1]->value()._double : double(tuple[1]->value()._int);
		if(a2 == 0) return 0;
		else return DomainElementFactory::instance()->create(a1 / a2,false);
	}
}

InternalTableIterator* DivInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

const DomainElement* AbsInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(tuple[0]->type() == DET_INT) {
		int val = tuple[0]->value()._int;
		return DomainElementFactory::instance()->create(val < 0 ? -val : val);
	}
	else {
		return DomainElementFactory::instance()->create(abs(tuple[0]->value()._double),true);
	}
}

InternalTableIterator* AbsInternalFuncTable::begin(const Universe& univ ) const {
	return new InternalFuncIterator(this,univ);
}

const DomainElement* UminInternalFuncTable::operator[](const ElementTuple& tuple) const {
	if(tuple[0]->type() == DET_INT) {
		return DomainElementFactory::instance()->create(-(tuple[0]->value()._int));
	}
	else {
		return DomainElementFactory::instance()->create(-(tuple[0]->value()._double),true);
	}
}

InternalTableIterator* UminInternalFuncTable::begin(const Universe& univ) const {
	return new InternalFuncIterator(this,univ);
}

/****************
	PredTable
****************/

PredTable::PredTable(InternalPredTable* table, const Universe& univ) : _table(table), _universe(univ) {
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
	TableIterator ti(_table->begin(_universe));
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

bool ProcInternalPredTable::finite(const Universe& univ) const {
	return univ.finite();
}

bool ProcInternalPredTable::empty(const Universe& univ) const {
	return univ.empty();
}

bool ProcInternalPredTable::approxfinite(const Universe& univ) const {
	return univ.approxfinite();
}

bool ProcInternalPredTable::approxempty(const Universe& univ) const {
	return univ.approxempty();
}

bool ProcInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if(univ.contains(tuple)) {
		return LuaConnection::predcall(_procedure,tuple);
	}
	else return false;
}

InternalPredTable* ProcInternalPredTable::add(const ElementTuple& ) {
	notyetimplemented("Adding a tuple to a procedural predicate table");
	return this;
}

InternalPredTable* ProcInternalPredTable::remove(const ElementTuple& ) {
	notyetimplemented("Removing a tuple from a procedural predicate table");
	return this;
}

InternalTableIterator* ProcInternalPredTable::begin(const Universe& univ) const {
	return new ProcInternalTableIterator(this,univ);
}

InverseInternalPredTable::~InverseInternalPredTable() {
	_invtable->decrementRef();
}

void InverseInternalPredTable::interntable(InternalPredTable* ipt) {
	ipt->incrementRef();
	_invtable->decrementRef();
	_invtable = ipt;
}

/**
 *		Returns true iff the table is finite
 */
bool InverseInternalPredTable::finite(const Universe& univ) const {
	if(approxfinite(univ)) return true;
	if(univ.finite()) return true;
	else if(_invtable->finite(univ)) return false;
	else {
		notyetimplemented("Exact finiteness test on inverse predicate tables");	
		return approxempty(univ);
	}
}

/**
 *		Returns true iff the table is empty
 */
bool InverseInternalPredTable::empty(const Universe& univ) const {
	if(approxempty(univ)) return true;
	if(univ.empty()) return true;
	if(finite(univ)) {
		TableIterator ti = TableIterator(begin(univ));
		return !(ti.hasNext());
	}
	else {
		notyetimplemented("Exact emptyness test on inverse predicate tables");	
		return approxempty(univ);
	}
}

/**
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
bool InverseInternalPredTable::approxfinite(const Universe& univ) const {
	return univ.approxfinite();
}

/**
 *		Returns false if the table is non-empty. May return true if the table is empty.
 */
bool InverseInternalPredTable::approxempty(const Universe& univ) const {
	return univ.approxempty();
}

tablesize InverseInternalPredTable::size(const Universe& univ) const {
	tablesize univsize = univ.size();
	tablesize invsize = _invtable->size(univ);
	if(univsize.first && invsize.first) {
		return tablesize(true,univsize.second - invsize.second);
	}
	else return univsize;
}

/**
 *	Returns true iff the table contains a given tuple
 *
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool InverseInternalPredTable::contains(const ElementTuple& tuple, const Universe& univ) const {
	if(_invtable->contains(tuple,univ)) return false;
	else if(univ.contains(tuple)) return true;
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
	if(_nrRefs > 1) {
		InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable);
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
	if(_nrRefs > 1) {
		InverseInternalPredTable* newtable = new InverseInternalPredTable(_invtable);
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

InternalTableIterator* InverseInternalPredTable::begin(const Universe& univ) const {
	vector<SortIterator> vsi;
	for(vector<SortTable*>::const_iterator it = univ.tables().begin(); it != univ.tables().end(); ++it) {
		vsi.push_back((*it)->sortbegin());
	}
	return new InverseInternalIterator(vsi,_invtable,univ);
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

SortIterator SortTable::sortiterator(const DomainElement* d) const {
	return SortIterator(_table->sortiterator(d));
}

void SortTable::interntable(InternalSortTable* table) {
	_table->decrementRef();
	_table = table;
	_table->incrementRef();
}

void SortTable::add(const ElementTuple& tuple) {
	if(_table->contains(tuple)) return;
	InternalSortTable* temp = _table;
	_table = _table->add(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::add(const DomainElement* el) {
	if(_table->contains(el)) return;
	InternalSortTable* temp = _table;
	_table = _table->add(el);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::add(int i1, int i2) {
	InternalSortTable* temp = _table;
	_table = _table->add(i1,i2);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::remove(const ElementTuple& tuple) {
	if(!_table->contains(tuple)) return;
	InternalSortTable* temp = _table;
	_table = _table->remove(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void SortTable::remove(const DomainElement* el) {
	if(!_table->contains(el)) return;
	InternalSortTable* temp = _table;
	_table = _table->remove(el);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}


/****************
	FuncTable
****************/

FuncTable::FuncTable(InternalFuncTable* table, const Universe& univ) : _table(table), _universe(univ) {
	_table->incrementRef();
}

FuncTable::~FuncTable() {
	_table->decrementRef();
}

void FuncTable::add(const ElementTuple& tuple) {
	if(_table->contains(tuple,_universe)) return;
	InternalFuncTable* temp = _table;
	_table = _table->add(tuple);
	if(temp != _table) {
		temp->decrementRef();
		_table->incrementRef();
	}
}

void FuncTable::remove(const ElementTuple& tuple) {
	if(!_table->contains(tuple,_universe)) return;
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
 *	- cf	: if true (false), ctpf stores the certainly false (possibly true) tuples
 *	- univ	: all possible domain elements of the sorts of the columns of the table
 */
PredInter::PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf) {
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->interntable()),ctpf->universe());
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->interntable()),ctpf->universe());
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
	PredTable* cfpt = new PredTable(ctpf->interntable(),ctpf->universe());
	PredTable* inverseCtpf = new PredTable(new InverseInternalPredTable(ctpf->interntable()),ctpf->universe());
	PredTable* inverseCfpt = new PredTable(new InverseInternalPredTable(cfpt->interntable()),cfpt->universe());
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

void PredInter::makeUnknown(const ElementTuple& tuple) {
	if(typeid(*(_pf->interntable())) == typeid(InverseInternalPredTable)) {
		_ct->interntable()->decrementRef();
		InternalPredTable* old = _ct->interntable();
		_ct->remove(tuple);
		old->incrementRef();
		if(_ct->interntable() != old) {
			InverseInternalPredTable* internpf = dynamic_cast<InverseInternalPredTable*>(_pf->interntable());
			internpf->interntable(_ct->interntable());
		}
	}
	else {
		_pf->interntable()->decrementRef();
		InternalPredTable* old = _pf->interntable();
		_pf->add(tuple);
		old->incrementRef();
		if(_pf->interntable() != old) {
			InverseInternalPredTable* internct = dynamic_cast<InverseInternalPredTable*>(_ct->interntable());
			internct->interntable(_pf->interntable());
		}
	}
	if(typeid(*(_pt->interntable())) == typeid(InverseInternalPredTable)) {
		_cf->interntable()->decrementRef();
		InternalPredTable* old = _cf->interntable();
		_cf->remove(tuple);
		old->incrementRef();
		if(_cf->interntable() != old) {
			InverseInternalPredTable* internpt = dynamic_cast<InverseInternalPredTable*>(_pt->interntable());
			internpt->interntable(_cf->interntable());
		}
	}
	else {
		_pt->interntable()->decrementRef();
		InternalPredTable* old = _pt->interntable();
		_pt->add(tuple);
		old->incrementRef();
		if(_pt->interntable() != old) {
			InverseInternalPredTable* interncf = dynamic_cast<InverseInternalPredTable*>(_cf->interntable());
			interncf->interntable(_pt->interntable());
		}
	}
}

void PredInter::makeTrue(const ElementTuple& tuple) {
	if(typeid(*(_pf->interntable())) == typeid(InverseInternalPredTable)) {
		_ct->interntable()->decrementRef();
		InternalPredTable* old = _ct->interntable();
		_ct->add(tuple);
		old->incrementRef();
		if(_ct->interntable() != old) {
			InverseInternalPredTable* internpf = dynamic_cast<InverseInternalPredTable*>(_pf->interntable());
			internpf->interntable(_ct->interntable());
		}
	}
	else {
		_pf->interntable()->decrementRef();
		InternalPredTable* old = _pf->interntable();
		_pf->remove(tuple);
		old->incrementRef();
		if(_pf->interntable() != old) {
			InverseInternalPredTable* internct = dynamic_cast<InverseInternalPredTable*>(_ct->interntable());
			internct->interntable(_pf->interntable());
		}
	}
}

void PredInter::makeFalse(const ElementTuple& tuple) {
	if(typeid(*(_pt->interntable())) == typeid(InverseInternalPredTable)) {
		_cf->interntable()->decrementRef();
		InternalPredTable* old = _cf->interntable();
		_cf->add(tuple);
		old->incrementRef();
		if(_cf->interntable() != old) {
			InverseInternalPredTable* internpt = dynamic_cast<InverseInternalPredTable*>(_pt->interntable());
			internpt->interntable(_cf->interntable());
		}
	}
	else {
		_pt->interntable()->decrementRef();
		InternalPredTable* old = _pt->interntable();
		_pt->remove(tuple);
		old->incrementRef();
		if(_pt->interntable() != old) {
			InverseInternalPredTable* interncf = dynamic_cast<InverseInternalPredTable*>(_cf->interntable());
			interncf->interntable(_pt->interntable());
		}
	}
}

void PredInter::ct(PredTable* t) {
	delete(_ct);
	delete(_pf);
	_ct = t;
	_pf = new PredTable(new InverseInternalPredTable(t->interntable()),t->universe());
}

void PredInter::cf(PredTable* t) {
	delete(_cf);
	delete(_pt);
	_cf = t;
	_pt = new PredTable(new InverseInternalPredTable(t->interntable()),t->universe());
}

void PredInter::pt(PredTable* t) {
	delete(_pt);
	delete(_cf);
	_pt = t;
	_cf = new PredTable(new InverseInternalPredTable(t->interntable()),t->universe());
}

void PredInter::pf(PredTable* t) {
	delete(_pf);
	delete(_ct);
	_pf = t;
	_ct = new PredTable(new InverseInternalPredTable(t->interntable()),t->universe());
}

void PredInter::ctpt(PredTable* t) {
	ct(t);
	PredTable* npt = new PredTable(t->interntable(),t->universe());
	pt(npt);
}

PredInter* PredInter::clone(const Universe& univ) const {
	PredTable* nctpf;
	bool ct;
	if(typeid(*(_pf->interntable())) == typeid(InverseInternalPredTable)) {
		nctpf = new PredTable(_ct->interntable(),univ);
		ct = true;
	}
	else {
		nctpf = new PredTable(_pf->interntable(),univ);
		ct = false;
	}
	if(approxtwovalued()) return new PredInter(nctpf,ct);
	else {
		PredTable* ncfpt;
		bool cf;
		if(typeid(*(_pt->interntable())) == typeid(InverseInternalPredTable)) {
			ncfpt = new PredTable(_cf->interntable(),univ);
			cf = true;
		}
		else {
			ncfpt = new PredTable(_pt->interntable(),univ);
			cf = false;
		}
		return new PredInter(nctpf,ncfpt,ct,cf);
	}
}

PredInter* EqualInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2,st));
	EqualInternalPredTable* eip = new EqualInternalPredTable();
	PredTable* ct = new PredTable(eip,univ);
	return new PredInter(ct,true);
}

PredInter* StrLessThanInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2,st));
	StrLessInternalPredTable* eip = new StrLessInternalPredTable();
	PredTable* ct = new PredTable(eip,univ);
	return new PredInter(ct,true);
}

PredInter* StrGreaterThanInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(2,st));
	StrGreaterInternalPredTable* eip = new StrGreaterInternalPredTable();
	PredTable* ct = new PredTable(eip,univ);
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
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft,true),ft->universe());
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
	PredTable* ct = new PredTable(new FuncInternalPredTable(ft,true),ft->universe());
	_graphinter = new PredInter(ct,true);
}

FuncInter* FuncInter::clone(const Universe& univ) const {
	if(_functable) {
		FuncTable* nft = new FuncTable(_functable->interntable(),univ);
		return new FuncInter(nft);
	}
	else {
		PredInter* npi = _graphinter->clone(univ);
		return new FuncInter(npi);
	}
}

FuncInter* MinInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	ElementTuple t;
	ElementFunc ef; ef[t] = st->first();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift,univ);
	return new FuncInter(ft);
}

FuncInter* MaxInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	Universe univ(vector<SortTable*>(1,st));
	ElementTuple t;
	ElementFunc ef; ef[t] = st->last();
	EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable(ef);
	FuncTable* ft = new FuncTable(ift,univ);
	return new FuncInter(ft);
}

FuncInter* SuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2,st);
	FuncTable* ft = 0; // TODO
	notyetimplemented("successor function");
	return new FuncInter(ft);
}

FuncInter* InvSuccInterGenerator::get(const AbstractStructure* structure) {
	SortTable* st = structure->inter(_sort);
	vector<SortTable*> univ(2,st);
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
		PredTable* t1 = new PredTable(new EnumeratedInternalPredTable(),univ);
		PredTable* t2 = new PredTable(new EnumeratedInternalPredTable(),univ);
		return new PredInter(t1,t2,true,true);
	}

	FuncInter* leastFuncInter(const Universe& univ) {
		PredInter* pt = leastPredInter(univ);
		return new FuncInter(pt);
	}

	Universe fullUniverse(unsigned int arity) {
		vector<SortTable*> vst(arity,VocabularyUtils::stringsort()->interpretation());
		return Universe(vst);
	}

	bool approxIsInverse(const PredTable* pt1, const PredTable* pt2) {
		tablesize univsize = pt1->universe().size();
		tablesize pt1size = pt1->size();
		tablesize pt2size = pt2->size();
		if(univsize.first && pt1size.first && pt2size.first) {
			return pt1size.second + pt2size.second == univsize.second;
		}
		else return false;
	}

	bool approxTotalityCheck(const FuncInter* funcinter) {
		vector<SortTable*> vst = funcinter->universe().tables();
		vst.pop_back();
		tablesize nroftuples = Universe(vst).size();
		tablesize nrofvalues = funcinter->graphinter()->ct()->size();
//cerr << "Checking totality of " << *function << " -- nroftuples=" << nroftuples.second << " and nrofvalues=" << nrofvalues.second;
//cerr << " (trust=" << (nroftuples.first && nrofvalues.first) << ")" << "\n";
		if(nroftuples.first && nrofvalues.first) {
			return nroftuples.second == nrofvalues.second;
		}
		else return false;
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
			if(!(*jt)->builtin()) {
				if(_sortinter.find(*jt) == _sortinter.end()) {
					SortTable* st = new SortTable(new EnumeratedInternalSortTable());
					_sortinter[*jt] = st;
					vector<SortTable*> univ(1,st);
					PredTable* pt = new PredTable(new FullInternalPredTable(),Universe(univ));
					_predinter[(*jt)->pred()] = new PredInter(pt,true);
				}
			}
		}
	}
	for(map<string,Predicate*>::const_iterator it = _vocabulary->firstpred(); it != _vocabulary->lastpred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for(set<Predicate*>::iterator jt = sp.begin(); jt != sp.end(); ++jt) {
			if(_predinter.find(*jt) == _predinter.end()) {
				vector<SortTable*> univ;
				for(vector<Sort*>::const_iterator kt = (*jt)->sorts().begin(); kt != (*jt)->sorts().end(); ++kt) {
					univ.push_back(inter(*kt));
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
					univ.push_back(inter(*kt));
				}
				_funcinter[(*jt)] = TableUtils::leastFuncInter(Universe(univ));
			}
		}
	}
}

void Structure::inter(Predicate* p, PredInter* i) {
	delete(_predinter[p]);
	_predinter[p] = i;
}

void Structure::inter(Function* f, FuncInter* i) {
	delete(_funcinter[f]);
	_funcinter[f] = i;
}


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

void completeSortTable(const PredTable* pt, PFSymbol* symbol, const string& structname) {
	if(pt->approxfinite()) {
		for(TableIterator jt = pt->begin(); jt.hasNext(); ++jt) {
			const ElementTuple& tuple = *jt;
			for(unsigned int col = 0; col < tuple.size(); ++col) {
				if(!symbol->sorts()[col]->builtin()) {
					pt->universe().tables()[col]->add(tuple[col]);
				}
				else if(!pt->universe().tables()[col]->contains(tuple[col])) {
					if(typeid(*symbol) == typeid(Predicate)) {
						Error::predelnotinsort(tuple[col]->to_string(),symbol->name(), 
											   symbol->sorts()[col]->name(),structname);
					}
					else {
						Error::funcelnotinsort(tuple[col]->to_string(),symbol->name(), 
											   symbol->sorts()[col]->name(),structname);
					}
				}
			}
		}
	}
}

void addUNAPattern(Function* ) {
	// TODO
	notyetimplemented("una pattern type");
}

void Structure::autocomplete() {
	// Adding elements from predicate interpretations to sorts
	for(map<Predicate*,PredInter*>::const_iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		if(it->first->arity() != 1 || it->first->sorts()[0]->pred() != it->first) {
			const PredTable* pt1 = it->second->ct();
			if(typeid(*(pt1->interntable())) == typeid(InverseInternalPredTable)) pt1 = it->second->pf();
			completeSortTable(pt1,it->first,_name);
			if(!it->second->approxtwovalued()) {
				const PredTable* pt2 = it->second->cf();
				if(typeid(*(pt2->interntable())) == typeid(InverseInternalPredTable)) pt2 = it->second->pt();
				completeSortTable(pt2,it->first,_name);
			}
		}
	}
	// Adding elements from function interpretations to sorts
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		if(it->second->functable() && typeid(*(it->second->functable()->interntable())) == typeid(UNAInternalFuncTable)) {
			addUNAPattern(it->first);
		}
		else {
			const PredTable* pt1 = it->second->graphinter()->ct();
			if(typeid(*(pt1->interntable())) == typeid(InverseInternalPredTable)) pt1 = it->second->graphinter()->pf();
			completeSortTable(pt1,it->first,_name);
			if(!it->second->approxtwovalued()) {
				const PredTable* pt2 = it->second->graphinter()->cf();
				if(typeid(*(pt2->interntable())) == typeid(InverseInternalPredTable)) pt2 = it->second->graphinter()->pt();
				completeSortTable(pt2,it->first,_name);
			}
		}
	}


	// Adding elements from subsorts to supersorts
	map<Sort*,unsigned int> scores;
	for(map<string,set<Sort*> >::const_iterator it = _vocabulary->firstsort(); it != _vocabulary->lastsort(); ++it) {
		for(set<Sort*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			computescore(*jt,scores);
		}
	}
	map<unsigned int,vector<Sort*> > invscores;
	for(map<Sort*,unsigned int>::const_iterator it = scores.begin(); it != scores.end(); ++it) {
		if(_vocabulary->contains(it->first)) {
			invscores[it->second].push_back(it->first);
		}
	}
	for(map<unsigned int,vector<Sort*> >::const_reverse_iterator it = invscores.rbegin(); it != invscores.rend(); ++it) {
		for(vector<Sort*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			Sort* s = *jt;
			set<Sort*> notextend;
			notextend.insert(s);
			vector<Sort*> toextend;
			vector<Sort*> tocheck;
			while(!(notextend.empty())) {
				Sort* e = *(notextend.begin());
				for(set<Sort*>::const_iterator kt = e->parents().begin(); kt != e->parents().end(); ++kt) {
					Sort* sp = *kt;
					if(_vocabulary->contains(sp)) {
						if(sp->builtin()) {
							tocheck.push_back(sp);
						}
						else {
							toextend.push_back(sp); 
						}
					}
					else notextend.insert(sp);
				}
				notextend.erase(e);
			}
			SortTable* st = inter(s);
			for(vector<Sort*>::const_iterator kt = toextend.begin(); kt != toextend.end(); ++kt) {
				SortTable* kst = inter(*kt);
				if(st->approxfinite()) {
					for(SortIterator lt = st->sortbegin(); lt.hasNext(); ++lt) kst->add(*lt);
				}
				else { 
					// TODO
				}
			}
			if(!s->builtin()) {
				for(vector<Sort*>::const_iterator kt = tocheck.begin(); kt != tocheck.end(); ++kt) {
					SortTable* kst = inter(*kt);
					if(st->approxfinite()) {
						for(SortIterator lt = st->sortbegin(); lt.hasNext(); ++lt) {
							if(!kst->contains(*lt)) 
								Error::sortelnotinsort((*lt)->to_string(),s->name(),(*kt)->name(),_name);
						}
					}
					else {
						// TODO
					}
				}
			}
		}
	}
}

void Structure::addStructure(AbstractStructure* ) {
	// TODO
}

void Structure::functioncheck() {
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		Function* f = it->first;
		FuncInter* ft = it->second;
		if(it->second->universe().approxfinite()) {
			PredInter* pt = ft->graphinter();
			const PredTable* ct = pt->ct();
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
				PredTable spt(new FullInternalPredTable(),Universe(vst));
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

Universe Structure::universe(const PFSymbol* s) const {
	vector<SortTable*> vst;
	for(vector<Sort*>::const_iterator it = s->sorts().begin(); it != s->sorts().end(); ++it) {
		vst.push_back(inter(*it));
	}
	return Universe(vst);
}

void Structure::clean() {
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		if(!it->second->approxtwovalued()) {
			if(TableUtils::approxIsInverse(it->second->ct(),it->second->cf())) {
				PredTable* npt = new PredTable(it->second->ct()->interntable(),it->second->ct()->universe());
				it->second->pt(npt);
			}
		}
	}
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		if(it->first->partial()) {
			SortTable* lastsorttable = it->second->universe().tables().back();
			for(TableIterator ctit = it->second->graphinter()->ct()->begin(); ctit.hasNext(); ++ctit) {
				ElementTuple tuple = *ctit;
				const DomainElement* ctvalue = tuple.back();
				for(SortIterator sortit = lastsorttable->sortbegin(); sortit.hasNext(); ++sortit) {
					const DomainElement* cfvalue = *sortit;
					if(*cfvalue != *ctvalue) {
						tuple.pop_back();
						tuple.push_back(*sortit);
						it->second->graphinter()->makeFalse(tuple);
					}
				}
			}
		}
		if(!it->second->approxtwovalued()) {
			if(((not it->first->partial()) && TableUtils::approxTotalityCheck(it->second))
			|| TableUtils::approxIsInverse(it->second->graphinter()->ct(),it->second->graphinter()->cf())) {
				EnumeratedInternalFuncTable* eift = new EnumeratedInternalFuncTable();
				for(TableIterator jt = it->second->graphinter()->ct()->begin(); jt.hasNext(); ++jt) {
					eift->add(*jt);
				}
				it->second->functable(new FuncTable(eift,it->second->graphinter()->ct()->universe()));
			}
		}
	}
}

/**************
	Visitor
**************/


void ProcInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void BDDInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void FullInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void FuncInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void UnionInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void EnumeratedInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void EqualInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void StrLessInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void StrGreaterInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void InverseInternalPredTable::accept(StructureVisitor* v) const { v->visit(this);	}
void UnionInternalSortTable::accept(StructureVisitor* v) const { v->visit(this);	}
void AllNaturalNumbers::accept(StructureVisitor* v) const { v->visit(this);	}
void AllIntegers::accept(StructureVisitor* v) const { v->visit(this);	}
void AllFloats::accept(StructureVisitor* v) const { v->visit(this);	}
void AllChars::accept(StructureVisitor* v) const { v->visit(this);	}
void AllStrings::accept(StructureVisitor* v) const { v->visit(this);	}
void EnumeratedInternalSortTable::accept(StructureVisitor* v) const { v->visit(this);	}
void IntRangeInternalSortTable::accept(StructureVisitor* v) const { v->visit(this);	}
void ProcInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void UNAInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void EnumeratedInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void PlusInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void MinusInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void TimesInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void DivInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void AbsInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void UminInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void ExpInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
void ModInternalFuncTable::accept(StructureVisitor* v) const { v->visit(this);	}
