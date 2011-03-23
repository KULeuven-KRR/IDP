/************************************
	structure.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "vocabulary.hpp"
#include "structure.hpp"

/**********************
	Domain elements
**********************/

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
	if(c1.function() != c2.function()) return false;
	for(unsigned int n = 0; n < c1.function()->arity(); ++n) {
		if(c1.arg(n) != c2.arg(n)) return false;
	}
	return true;
}

bool operator!=(const Compound& c1,const Compound& c2) {
	return !(c1 == c2);
}

bool operator<=(const Compound& c1,const Compound& c2) {
	return (c1 == c2 || c1 < c2);
}

bool operator>=(const Compound& c1,const Compound& c2) {
	return (c1 == c2 || c2 < c1);
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


/**
 * DESCRIPTION
 *		Destructor for domain elements. Does not delete its value.
 */
DomainElement::~DomainElement() { }

/**
 * DESCRIPTION
 *		Constructor for domain elements that are integers
 */
DomainElement::DomainElement(int value) : _type(DET_INT) {
	_value._int = value;
}

/**
 * DESCRIPTION
 *		Constructor for domain elements that are floating point numbers but not integers
 */
DomainElement::DomainElement(double value) : _type(DET_DOUBLE) {
	assert(!isInt(value));
	_value._double = value;
}

/**
 * DESCRIPTION
 *		Constructor for domain elements that are strings but not floating point numbers
 */
DomainElement::DomainElement(const string* value) : _type(DET_STRING) {
	assert(!isDouble(*value));
	_value._string = string;
}

/**
 * DESCRIPTION
 *		Constructor for domain elements that are compounds
 */
DomainElement::DomainElement(const compound* value) : _type(DET_COMPOUND) {
	_value._compound = value;
}

/**
 * DESCRIPTION
 *		Destructor for compound domain element values. Does not delete its arguments.
 */
Compound::~Compound() { }

/**
 * DESCRIPTION
 *		Constructor for a domain element factory. The constructor gets two arguments, 
 *		specifying the range of integer for which creation of domain elements is optimized.
 *
 * PARAMETERS
 *		- firstfastint:	the lowest 'efficient' integer
 *		- lastfastint:	one past the highest 'efficient' integer
 */
DomainElementFactory::DomainElementFactory(int firstfastint, int lastfastint) : 
	_firstfastint(firstfastint), _lastfastint(lastfastint) {
	assert(firstfastint < lastfastint);
	_fastintelements = vector<DomainElement*>(lastfastint - firstfastint,0);
}

DomainElementFactory* DomainElementFactory::_instance = 0;

/**
 * DESCRIPTION
 *		Returns the unique instance of DomainElementFactory
 */
DomainElementFactory* DomainElementFactory::instance() {
	if(!_instance) _instance = new DomainElementFactory();
	return _instance;
}

/**
 * DESCRIPTION
 *		Destructor for DomainElementFactory. Deletes all domain elements and compounds it created.
 */
DomainElementFactory::~DomainElementFactory {
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
	for(map<const Function*,map<vector<DomainElement*>,Compound*> >::iterator it = _compounds.begin(); it != _compounds.end(); ++it) {
		for(map<vector<DomainElement*>,Compound*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			delete(jt->second);
		}
	}
}

/**
 * DESCRIPTION
 *		Return the unique compound that consists of the given function and arguments.
 * 
 * PARAMETERS
 *		- function:	the given function
 *		- args:		the given arguments
 */
Compound* DomainElementFactory::compound(const Function* function, const ElementTuple& args) {
	map<const Function*,map<ElementTuple,Compound*> >::const_iterator it = _compounds.find(function);
	if(it != _compounds.end()) {
		map<ElementTuple,Compound*>::const_iterator jt = it->second.find(args); 
		if(jt != it->second.end()) return jt->second;
	}
	Compound* newcompound = new Compound(function,args);
	_compounds[function][args] = newcompound;
	return newcompound;
}

/**
 * DESCRIPTION
 *		Returns the unique domain element of type int that has a given value
 *
 * PARAMETERS
 *		- value: the given value
 */
DomainElement* DomainElementFactory::create(int value) {
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
 * DESCRIPTION
 *		Returns the unique domain element that has a given floating point value
 *
 * PARAMETERS
 *		- value:		the given value
 *		- certnotint:	true iff the caller of this method asserts that the value is not an integer
 */
DomainElement* DomainElementFactory::create(double value, bool certnotint) {
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
 * DESCRIPTION
 *		Returns the unique domain element that has a given string value
 *
 * PARAMETERS
 *		- value:			the given value
 *		- certnotdouble:	true iff the caller of this method asserts that the value is not a floating point number
 */
DomainElement* DomainElementFactory::create(const string* value, bool certnotdouble) {
	if(!certnotdouble && isDouble(value)) return create(stod(value),false);

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
 * DESCRIPTION
 *		Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- value:	the given value
 */
DomainElement* DomainElementFactory::create(const Compound* value, bool certnotdouble) {
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
 * DESCRIPTION
 *		Returns the unique domain element that has a given compound value
 *
 * PARAMETERS
 *		- function:	the function of the given compound value
 *		- args:		the arguments of the given compound value
 */
DomainElement* DomainElementFactory::create(const Function* function, const ElementTuple& args) {
	assert(function != 0);
	Compound* value = compound(function,args);
	return create(value,true);
}

/****************
	PredTable
****************/

/**
 * DESCRIPTION
 *		Strict weak ordering on tuples of domain elements.
 *		Used as parameter for algorithms of the standard library.
 */
bool strictWeakOrdering(const ElementTuple& t1, const ElementTuple& t2) {
	assert(t1.size() == t2.size());
	for(unsigned int n = 0; n < t1.size(); ++n) {
		if(*(t1[n]) < *(t2[n])) return true;
		else if(*(t1[n] > *(t2[n]))) return false;
	}
	return false;
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EnumeratedInternalPredTable::contains(const ElementTuple& tuple) {
	assert(tuple.size() == arity());
	if(!sorted()) sortunique();
	ElementTable::const_iterator it = lower_bound(_table.begin(),_table.end(),tuple,&strictWeakOrdering); 
	return (it != _table.end());
}

/**
 * DESCRIPTION
 *		Sorts the table and remove all duplicates
 * POSTCONDITION
 *		_sorted = true
 */
void EnumeratedInternalPredTable::sortunique() {
	sort(_table.begin(),_table.end(),&strictWeakOrdering);
	ElementTable::iterator it = unique(_table.begin(),_table.end());
	_table.erase(it,_table.end());
	_sorted = true;
}

/**
 * DESCRIPTION
 *		Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
EnumeratedInternalPredTable* EnumeratedInternalPredTable::add(const ElementTuple& tuple) {
	_sorted = false;
	_table.push_back(tuple);
	return this;
}

/**
 * DESCRIPTION
 *		Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
EnumeratedInternalPredTable* EnumeratedInternalPredTable::remove(const ElementTuple& tuple) {
	ElementTable::iterator it = lower_bound(_table.begin(),_table.end(),&strictWeakOrdering);
	if((*it) == tuple) _table.erase(tuple);
	return this;
}

/**
 * DESCRIPTION
 *		Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* ComparisonInternalPredTable::add(const ElementTuple& tuple) {
	if(!contains(tuple)) {
		UnionInternalPredTable* upt = new UnionInternalPredTable();
		InternalPredTable* temp1 = upt->add(this);
		if(temp1 != upt) delete(upt);
		InternalPredTable* temp2 = temp1->add(tuple);
		if(temp1 != temp2) delete(temp1);
		return temp2;
	}
	else return this;
}

/**
 * DESCRIPTION
 *		Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* ComparisonInternalPredTable::remove(const ElementTuple& tuple) {
	if(PredTable::contains(tuple)) {
		UnionInternalPredTable* upt = new UnionInternalPredTable();
		InternalPredTable* temp1 = upt->add(this);
		if(temp1 != upt) delete(upt);
		InternalPredTable* temp2 = temp1->remove(tuple);
		if(temp1 != temp2) delete(temp1);
		return temp2;
	}
	else return this;
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool EqualInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return tuple[0] == tuple[1];
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrLessInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return *(tuple[0]) < *(tuple[1]);
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool StrGreaterInternalPredTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == 2);
	return *(tuple[0]) > *(tuple[1]);
}

/**
 * DESCRIPTION
 *		Returns true iff the table is finite
 */
bool EqualInternalPredTable::finite() const {
	if(approxfinite()) return true;
	else {
		if(_lefttable->finite() || _righttable->finite()) return true;
		else {
			notyetimplemented("Exact finiteness test on equality predicate tables");	
			return approxifinite();
		}
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table is finite
 */
bool StrLessInternalPredTable::finite() const {
	if(approxfinite() || empty()) return true;
	else {
		if(_lefttable->finite() && _righttable->finite()) return true;
		else {
			notyetimplemented("Exact finiteness test on </2 predicate tables");	
			return approxifinite();
		}
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table is finite
 */
bool StrGreaterInternalPredTable::finite() const {
	if(approxfinite() || empty()) return true;
	else {
		if(_lefttable->finite() && _righttable->finite()) return true;
		else {
			notyetimplemented("Exact finiteness test on >/2 predicate tables");	
			return approxifinite();
		}
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table is empty
 */
bool EqualInternalPredTable::empty() const {
	if(approxempty()) return true;
	else {
		if(_lefttable->empty() || _righttable->empty()) return true;
		else if(_lefttable->finite()) {
			for(SortTable::const_iterator it = _lefttable->begin(); it != _lefttable->end(); ++it) {
				if(_righttable->contains(*it)) return false;
			}
			return true;
		}
		else if(_righttable->finite()) {
			for(SortTable::const_iterator it = _righttable->begin(); it != _righttable->end(); ++it) {
				if(_lefttable->contains(*it)) return false;
			}
			return true;
		}
		else {
			notyetimplemented("Exact emptyness test on equality predicate tables");	
			return approxempty();
		}
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table is empty
 */
bool StrLessInternalPredTable::empty() const {
	return approxempty();
}

/**
 * DESCRIPTION
 *		Returns true iff the table is empty
 */
bool StrGreaterInternalPredTable::empty() const {
	return approxempty();
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool EqualInternalPredTable::approxfinite() const {
	return (_lefttable->approxfinite() || _righttable->approxfinite());
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxfinite() const {
	return (_lefttable->approxfinite() && _righttable->approxfinite());
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxfinite() const {
	return (_lefttable->approxfinite() && _righttable->approxfinite());
}

/**
 * DESCRIPTION
 *		Returns false if the table is not empty. May return true if the table is empty.
 */
inline bool EqualInternalPredTable::approxempty() const {
	return (_lefttable->approxempty() || _righttable->approxempty());
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrLessInternalPredTable::approxempty() const {
	return *(_lefttable->front()) >= *(_righttable->back());
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
inline bool StrGreaterInternalPredTable::approxempty() const {
	return *(_lefttable->back()) <= *(_righttable->front());
}

/**
 * DESCRIPTION
 *		Returns true iff the table is finite
 */
bool InverseInternalPredTable::finite() const {
	if(approxfinite()) return true;
	bool containsInfiniteSort = false;
	for(vector<SortTable*>::const_iterator it = _universe.begin(); it != _universe.end(); ++it) {
		if(!(*it)->finite()) containsInfiniteSort = true;
		if((*it)->empty()) return true;
	}
	if(!containsInfiniteSort) return true;
	else if(_invtable->finite()) return false;
	else {
		notyetimplemented("Exact finiteness test on inverse predicate tables");	
		return approxempty();
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table is empty
 */
bool InverseInternalPredTable::empty() const {
	if(approxempty()) return true;
	for(vector<SortTable*>::const_iterator it = _universe.begin(); it != _universe.end(); ++it) {
		if((*it)->empty()) return true;
	}
	if(finite()) {
		assert(false); // TODO: check of all tuples in the cartesian product of _universe belong to _table;
	}
	else {
		notyetimplemented("Exact emptyness test on inverse predicate tables");	
		return approxempty();
	}
}

/**
 * DESCRIPTION
 *		Returns false if the table is infinite. May return true if the table is finite.
 */
bool InverseInternalPredTable::approxfinite() const {
	bool containsInfiniteSort = false;
	for(vector<SortTable*>::const_iterator it = _universe.begin(); it != _universe.end(); ++it) {
		if(!(*it)->approxfinite()) containsInfiniteSort = true;
		if((*it)->approxempty()) return true;
	}
	return !containsInfiniteSort;
}

/**
 * DESCRIPTION
 *		Returns false if the table is non-empty. May return true if the table is empty.
 */
bool InverseInternalPredTable::approxempty() const {
	for(vector<SortTable*>::const_iterator it = _universe.begin(); it != _universe.end(); ++it) {
		if((*it)->approxempty()) return true;
	}
	return false;
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool InverseInternalPredTable::contains(const ElementTuple& tuple) {
	if(_table->contains(tuple)) return false;
	else {
		for(unsigned int col = 0; col < tuple.size(); ++col) {
			if(!(_universe[col]->contains(tuple[col]))) return false;
		}
		return true;
	}
}

/**
 * DESCRIPTION
 *		Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* InverseInternalPredTable::add(const ElementTuple& tuple) {
	if(_table->contains(tuple)) {
		InternalPredTable* temp = _table->remove(tuple);
		if(temp != _table) { delete(_table); _table = temp;	}
		return this;
	}
	else if(contains(tuple)) {
		return this;
	}
	else {
		for(unsigned int n = 0; n < tuple.size(); ++n) {
			_universe[n]->add(tuple[n]);
		}
	}
}

/**
 * DESCRIPTION
 *		Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* InverseInternalPredTable::remove(const ElementTuple& tuple) {
	InternalPredTable* temp = _table->add(tuple);
	if(temp != _table) { delete(_table); _table = temp;	}
	return this;
}

/**
 * DESCRIPTION
 *		Destructor for union predicate tables
 */
UnionInternalPredTable::~UnionInternalPredTable {
	for(vector<InternalPredTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it)
		delete(*it);
	for(vector<InternalPredTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it)
		delete(*it);
}

/**
 * DESCRIPTION
 *		Returns the number of columns in the table
 */
unsigned int UnionInternalPredTable::arity() const {
	if(!_intables.empty()) return _intables[0]->arity();
	else if(!_outtables.empty()) return _outtables[0]->arity();
	else {
		assert(false);
		return 0;
	}
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool UnionInternalPredTable::contains(const ElementTuple& tuple) {
	bool in = false;
	for(vector<InternalPredTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if((*it)->contains(tuple)) { in = true; break;	}
	}
	if(!in) return false;

	bool out = false;
	for(vector<InternalPredTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(tuple)) { out = true; break;	}
	}
	return !out;
}

/**
 * DESCRIPTION
 *		Returns true iff the table contains a given tuple
 * PARAMETERS
 *		tuple	- the given tuple
 */
bool UnionInternalPredTable::contains(const ElementTuple& tuple) {
	bool in = false;
	for(vector<InternalPredTable*>::iterator it = _intables.begin(); it != _intables.end(); ++it) {
		if((*it)->contains(tuple)) { in = true; break;	}
	}
	if(!in) return false;

	bool out = false;
	for(vector<InternalPredTable*>::iterator it = _outtables.begin(); it != _outtables.end(); ++it) {
		if((*it)->contains(tuple)) { out = true; break;	}
	}
	return !out;
}

/**
 * DESCRIPTION
 *		Add a tuple to the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* UnionInternalPredTable::add(const ElementTuple& tuple) {
	bool in = false;
	unsigned int finitetab = _intables.size();
	for(unsigned int n = 0; n < _intables.size(); ++n) {
		if(_intables[n]->contains(tuple)) { in = true; break;	}
		if(_intables[n]->approxfinite()) finitetab = n;
	}
	if(!in) {
		if(finitetab < _intables.size()) {
			InternalPredTable* temp = _intables[finitetab]->add(tuple);
			if(temp != _intables[finitetab]) {
				delete(_intables[finitetab]);
				_intables[finitetab] = temp;
			}
		}
		else {
			EnumeratedInternalPredTable* newintable = new EnumeratedInternalPredTable();
			InternalPredTable* temp = newintable->add(tuple);
			if(temp != newintable) { delete(newintable); newintable = temp;	}
			_intables.push_back(newintable);
		}
	}

	for(unsigned int n = 0; n < _outtables.size(); ++n) {
		InternalPredTable* temp = _outtables[n]->remove(tuple);
		if(temp != _outtables[n]) {
			delete(_outtables[n]);
			_outtables[n] = temp;
		}
	}

	return this;
}

/**
 * DESCRIPTION
 *		Remove a tuple from the table. 
 * 
 * PARAMETERS
 *		- tuple: the tuple
 *
 * RETURNS
 *		A pointer to the updated table
 */
InternalPredTable* UnionInternalPredTable::remove(const ElementTuple& tuple) {
	for(unsigned int n = 0; n < _outtables.size(); ++n) {
		if(_outtables[n]->approxfinite()) {
			InternalPredTable* temp = _outtables[n]->add(tuple);
			if(temp != _outtables[n]) {
				delete(_outtables[n]);
				_outtables[n] = temp;
				return this;
			}
		}
	}
	EnumeratedInternalPredTable* newouttable = new EnumeratedInternalPredTable();
	InternalPredTable* temp = newouttable->add(tuple);
	if(temp != newouttable) { delete(newouttable); newouttable = temp;	}
	_outtables.push_back(newouttable);
}



/****************
	FuncTable
****************/

/**
 * DESCRIPTION
 *		Given a tuple (a_1,...,a_{n-1},a_n), this procedure returns true iff the value of
 *		(a_1,...,a_{n-1}) according to the function is equal to a_n
 * PARAMETERS
 *		tuple	- the given tuple
 * PRECONDITION
 *		The type of each of the elements of the tuple should be the same as the type of the corresponding
 *		columns of the table.
 */
bool FuncTable::contains(const ElementTuple& tuple) const {
	assert(tuple.size() == arity()+1);
	ElementTuple key = tuple;
	DomainElement* value = key.back();
	key.pop_back();
	DomainElement* computedvalue = operator[](key);
	return value == computedvalue;
}

/********************* VANAF HIER OUD ******************/
#include "theory.hpp"
#include "builtin.hpp"
#include "common.hpp"
#include "error.hpp"
#include <iostream>
#include <algorithm>
#include <typeinfo>

/**************
	Domains
**************/

/** constructors **/

CopySortTable::CopySortTable(SortTable* s) : SortTable() { 
	if(typeid(*s) == typeid(CopySortTable)) {
		CopySortTable* cs = dynamic_cast<CopySortTable*>(s);
		assert(typeid(*(cs->table())) != typeid(CopySortTable));
		_table = cs->table();
		cs->table()->addref();
	}
	else {
		_table = s;
		s->addref();	
	}
}

UnionSortTable* UnionSortTable::clone() const {
	UnionSortTable* ust = new UnionSortTable();
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		ust->add(_tables[n]->clone());
	}
	ust->blacklist(_blacklist->clone());
	return ust;
}

IntSortTable* IntSortTable::clone() const {
	IntSortTable* ist = new IntSortTable();
	ist->table(_table);
	return ist;
}

FloatSortTable* FloatSortTable::clone() const {
	FloatSortTable* fst = new FloatSortTable();
	fst->table(_table);
	return fst;
}

StrSortTable* StrSortTable::clone() const {
	StrSortTable* sst = new StrSortTable();
	sst->table(_table);
	return sst;
}

MixedSortTable* MixedSortTable::clone() const {
	MixedSortTable* mst = new MixedSortTable();
	mst->numtable(_numtable);
	mst->strtable(_strtable);
	mst->comtable(_comtable);
	return mst;
}

/** destructors **/
UnionPredTable::~UnionPredTable() {
	for(unsigned int n = 0; n < _tables.size(); ++n)
		delete(_tables[n]);
	delete(_blacklist);
}

UnionSortTable::~UnionSortTable() {
	for(unsigned int n = 0; n < _tables.size(); ++n)
		delete(_tables[n]);
	delete(_blacklist);
}

/** add elements or intervals **/

FiniteSortTable* FiniteSortTable::add(Element e, ElementType t) {
	switch(t) {
		case ELINT:
			return add(e._int);
		case ELDOUBLE:
			return add(e._double);
		case ELSTRING:
			return add(e._string);
		case ELCOMPOUND:
			return add(e._compound);
		default:
			assert(false); return 0;
	}
}

FiniteSortTable* MixedSortTable::add(int e) {
	_numtable.push_back(double(e));
	return this;
}

FiniteSortTable* EmptySortTable::add(int e) {
	IntSortTable* ist = new IntSortTable();
	ist->add(e);
	return ist;
}

FiniteSortTable* StrSortTable::add(int e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

IntSortTable* IntSortTable::add(int e) {
	_table.push_back(e);
	return this;
}

FiniteSortTable* FloatSortTable::add(int e) {
	_table.push_back(double(e));
	return this;
}

FiniteSortTable* RanSortTable::add(int e) {
	if(e == _last + 1) _last = e;
	else if(e == _first - 1) _first = e;
	else if(e < _first || e > _last) {
		IntSortTable* ist = new IntSortTable();
		ist->add(_first,_last);
		ist->add(e);
		return ist;
	}
	return this;
}

FiniteSortTable* MixedSortTable::add(string* e) {
	if(isDouble(*e)) _numtable.push_back(stod(*e));
	else _strtable.push_back(e);
	return this;
}

FiniteSortTable* StrSortTable::add(string* e) {
	if(isDouble(*e)) return add(stod(*e));
	else {
		_table.push_back(e);
		return this;
	}
}

FiniteSortTable* IntSortTable::add(string* e) {
	if(isInt(*e)) return add(stoi(*e));
	else if(isDouble(*e)) return add(stod(*e));
	else {
		MixedSortTable* mst = new MixedSortTable();
		for(unsigned int n = 0; n < _table.size(); ++n)
			mst->add(_table[n]);
		mst->add(e);
		return mst;
	}
}

FiniteSortTable* FloatSortTable::add(string* e) {
	if(isDouble(*e)) return add(stod(*e));
	else {
		MixedSortTable* mst = new MixedSortTable(_table);
		mst->add(e);
		return mst;
	}
}

FiniteSortTable* RanSortTable::add(string* e) {
	if(isInt(*e)) return add(stoi(*e));
	else if(isDouble(*e)) return add(stod(*e));
	else {
		MixedSortTable* mst = new MixedSortTable();
		mst->add(_first,_last);
		mst->add(e);
		return mst;
	}
}

FiniteSortTable* EmptySortTable::add(string* e) {
	if(isInt(*e)) return add(stoi(*e));
	else if(isDouble(*e)) return add(stod(*e));
	else {
		StrSortTable* sst = new StrSortTable();
		sst->add(e);
		return sst;
	}
}

FiniteSortTable* MixedSortTable::add(int f, int l) {
	for(int e = f; e <= l; ++e) {
		_numtable.push_back(double(e));
	}
	return this;
}

FiniteSortTable* StrSortTable::add(int f, int l) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(f,l);
	return mst;
}

FiniteSortTable* IntSortTable::add(int f, int l) {
	for(int n = f; n <= l; ++n) 
		_table.push_back(n);
	return this;
}

FiniteSortTable* FloatSortTable::add(int f, int l) {
	for(int n = f; n <= l; ++n) 
		_table.push_back(double(n));
	return this;
}

FiniteSortTable* RanSortTable::add(int f, int l) {
	if(f >= _first && f <= _last+1) {
		_last = (l > _last ? l : _last);
		return this;
	}
	else if(_first >= f && _first <= l+1) {
		_first = f;
		_last = (l > _last ? l : _last);
		return this;
	}
	else {
		IntSortTable* ist = new IntSortTable();
		ist->add(_first,_last);
		ist->add(f,l);
		return ist;
	}
}

FiniteSortTable* EmptySortTable::add(int f, int l) {
	return new RanSortTable(f,l);
}

FiniteSortTable* MixedSortTable::add(char f, char l) {
	for(char n = f; n <= l; ++n) {
		_strtable.push_back(IDPointer(string(1,n)));
	}
	return this;
}

FiniteSortTable* StrSortTable::add(char f, char l) {
	for(char n = f; n <= l; ++n) {
		_table.push_back(IDPointer(string(1,n)));
	}
	return this;
}

FiniteSortTable* IntSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(f,l);
	for(unsigned int n = 0; n < _table.size(); ++n)
		mst->add(_table[n]);
	return mst;
}

FiniteSortTable* FloatSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(f,l);
	return mst;
}

FiniteSortTable* RanSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(_first,_last);
	mst->add(f,l);
	return mst;
}

FiniteSortTable* EmptySortTable::add(char f, char l) {
	StrSortTable* sst = new StrSortTable();
	sst->add(f,l);
	return sst;
}

FiniteSortTable* MixedSortTable::add(double e) {
	_numtable.push_back(e);
	return this;
}

FiniteSortTable* StrSortTable::add(double e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

FiniteSortTable* IntSortTable::add(double e) {
	if(double(int(e)) == e) return add(int(e));
	else {
		FloatSortTable* fst = new FloatSortTable();
		fst->add(e);
		for(unsigned int n = 0; n < _table.size(); ++n) 
			fst->add(double(_table[n]));
		return fst;
	}
}

FiniteSortTable* FloatSortTable::add(double e) {
	_table.push_back(double(e));
	return this;
}

FiniteSortTable* RanSortTable::add(double e) {
	if(double(int(e)) == e) return add(int(e));
	else {
		FloatSortTable* fst = new FloatSortTable();
		fst->add(e);
		fst->add(_first,_last);
		return fst;
	}
}

FiniteSortTable* EmptySortTable::add(double e) {
	if(double(int(e)) == e) return add(int(e));
	else {
		FloatSortTable* fst = new FloatSortTable();
		fst->add(e);
		return fst;
	}
}

FiniteSortTable* MixedSortTable::add(compound* c) {
	if(c->_function) {
		_comtable.push_back(c);
		return this;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

FiniteSortTable* StrSortTable::add(compound* c) {
	if(c->_function) {
		MixedSortTable* mst = new MixedSortTable(_table);
		mst->add(c);
		return mst;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

FiniteSortTable* IntSortTable::add(compound* c) {
	if(c->_function) {
		MixedSortTable* mst = new MixedSortTable();
		for(unsigned int n = 0; n < _table.size(); ++n) 
			mst->add(double(_table[n]));
		mst->add(c);
		return mst;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

FiniteSortTable* FloatSortTable::add(compound* c) {
	if(c->_function) {
		MixedSortTable* mst = new MixedSortTable(_table);
		mst->add(c);
		return mst;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

FiniteSortTable* RanSortTable::add(compound* c) {
	if(c->_function) {
		MixedSortTable* mst = new MixedSortTable();
		mst->add(_first,_last);
		mst->add(c);
		return mst;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

FiniteSortTable* EmptySortTable::add(compound* c) {
	if(c->_function) {
		MixedSortTable* mst = new MixedSortTable();
		mst->add(c);
		return mst;
	}
	else {
		switch((c->_args)[0]._type) {
			case ELINT:
				return add(((c->_args)[0]._element)._int);
			case ELDOUBLE:
				return add(((c->_args)[0]._element)._double);
			case ELSTRING:
				return add(((c->_args)[0]._element)._string);
			case ELCOMPOUND:
				return add(((c->_args)[0]._element)._compound);
			default:
				assert(false); return 0;
		}
	}
}

/** Mutators **/

FiniteSortTable* RanSortTable::remove(const vector<TypedElement>& tuple) {
	Element e = ElementUtil::convert(tuple[0],ELINT);
	if(ElementUtil::exists(e,ELINT)) {
		if(e._int == _first) _first = _first+1;
		else if(e._int == _last) _last = _last-1;
		else {
			IntSortTable* ist = new IntSortTable();
			for(int n = _first; n <= _last; ++n) {
				if(n != e._int) ist = ist->add(n);
			}
			return ist;
		}
	}
	return this;
}

IntSortTable* IntSortTable::remove(const vector<TypedElement>& tuple) {
	bool deleted = false;

	Element e = ElementUtil::convert(tuple[0],ELINT);
	if(ElementUtil::exists(e,ELINT)) {
		for(unsigned int n = 0; n < _table.size(); ++n) {
			if(e._int == _table[n]) {
				deleted = true;
			}
			else if(deleted) {
				_table[n-1] = _table[n];
			}
		}
		_table.pop_back();
	}
	return this;
}

FloatSortTable* FloatSortTable::remove(const vector<TypedElement>& tuple) {
	bool deleted = false;

	Element e = ElementUtil::convert(tuple[0],ELDOUBLE);
	if(ElementUtil::exists(e,ELDOUBLE)) {
		for(unsigned int n = 0; n < _table.size(); ++n) {
			if(e._double == _table[n]) {
				deleted = true;
			}
			else if(deleted) {
				_table[n-1] = _table[n];
			}
		}
		_table.pop_back();
	}
	return this;
}

StrSortTable* StrSortTable::remove(const vector<TypedElement>& tuple) {
	bool deleted = false;

	Element e = ElementUtil::convert(tuple[0],ELSTRING);
	if(ElementUtil::exists(e,ELSTRING)) {
		for(unsigned int n = 0; n < _table.size(); ++n) {
			if(e._string == _table[n]) {
				deleted = true;
			}
			else if(deleted) {
				_table[n-1] = _table[n];
			}
		}
		_table.pop_back();
	}
	return this;
}

FiniteSortTable* MixedSortTable::remove(const vector<TypedElement>& tuple) {
	switch(tuple[0]._type) {
		case ELINT: case ELDOUBLE:
		{
			bool deleted = false;
			Element e = ElementUtil::convert(tuple[0],ELDOUBLE);
			for(unsigned int n = 0; n < _numtable.size(); ++n) {
				if(e._double == _numtable[n]) {
					deleted = true;
				}
				else if(deleted) {
					_numtable[n-1] = _numtable[n];
				}
			}
			_numtable.pop_back();
		}
		break;
		case ELSTRING:
		{
			bool deleted = false;
			for(unsigned int n = 0; n < _numtable.size(); ++n) {
				if(tuple[0]._element._string == _strtable[n]) {
					deleted = true;
				}
				else if(deleted) {
					_strtable[n-1] = _strtable[n];
				}
			}
			_strtable.pop_back();
		}
		break;
		case ELCOMPOUND:
		{
			bool deleted = false;
			for(unsigned int n = 0; n < _comtable.size(); ++n) {
				if(tuple[0]._element._compound == _comtable[n]) {
					deleted = true;
				}
				else if(deleted) {
					_comtable[n-1] = _comtable[n];
				}
			}
			_comtable.pop_back();
		}
		break;
	}
	if(_comtable.empty()) {
		if(_numtable.empty()) {
			FiniteSortTable* fst = new EmptySortTable();
			for(unsigned int n = 0; n < _numtable.size(); ++n) {
				FiniteSortTable* nfst = fst->add(_numtable[n]);
				if(nfst != fst) delete(fst);
				fst = nfst;
			}
			return fst;
		}
		if(_strtable.empty()) {
			return new StrSortTable(_strtable);
		}
	}
	return this;
}




/** Sort and remove doubles **/

void IntSortTable::sortunique() {
	sort(_table.begin(),_table.end());
	vector<int>::iterator it = unique(_table.begin(),_table.end());
	_table.erase(it,_table.end());
}

void FloatSortTable::sortunique() {
	sort(_table.begin(),_table.end());
	vector<double>::iterator it = unique(_table.begin(),_table.end());
	_table.erase(it,_table.end());
}

bool stringptrequal(string* s1, string* s2) {
	return (s1 == s2 || ((*s1) == (*s2)));
}

bool stringptrslt(string* s1, string* s2) {
	return ((*s1) < (*s2));
}

void StrSortTable::sortunique() {
	sort(_table.begin(),_table.end(),&stringptrslt);
	vector<string*>::iterator it = unique(_table.begin(),_table.end(),&stringptrequal);
	_table.erase(it,_table.end());
}

bool compptrequal(compound* c1, compound* c2) {
	if(c1->_function != c2->_function) return false;
	else {
		for(unsigned int n = 0; n < (c1->_args).size(); ++n) {
			if(!((c1->_args)[n] == (c2->_args)[n])) return false;
		}
	}
	return true;
}

bool compptrslt(compound* c1, compound* c2) {
	if(c1->_function < c2->_function) return true;
	else if(c1->_function > c2->_function) return false;
	else {
		for(unsigned int n = 0; n < c1->_function->arity(); ++n) {
			if(((c1->_args)[n] < (c2->_args)[n])) return true;
			if(((c2->_args)[n] < (c1->_args)[n])) return false;
		}
		return false;
	}
	return false;
}

void MixedSortTable::sortunique() {

	sort(_numtable.begin(),_numtable.end());
	vector<double>::iterator it = unique(_numtable.begin(),_numtable.end());
	_numtable.erase(it,_numtable.end());

	sort(_strtable.begin(),_strtable.end(),&stringptrslt);
	vector<string*>::iterator jt = unique(_strtable.begin(),_strtable.end(),&stringptrequal);
	_strtable.erase(jt,_strtable.end());

	sort(_comtable.begin(),_comtable.end(),&compptrslt);
	vector<compound*>::iterator kt = unique(_comtable.begin(),_comtable.end(),&compptrequal);
	_comtable.erase(kt,_comtable.end());
}

void UnionSortTable::sortunique() {
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		_tables[n]->sortunique();
	}
	_blacklist->sortunique();
}

UnionSortTable* UnionSortTable::add(const vector<TypedElement>& tuple) {
	if(_blacklist->contains(tuple[0])) _blacklist->remove(tuple);
	bool added = false;
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		if(_tables[n]->contains(tuple)) {
			added = true;
			break;
		}
		else if(typeid(*(_tables[n])) == typeid(FiniteSortTable)) {
			_tables[n]->add(tuple);
			added = true;
			break;
		}
	}
	if(!added) {
		FiniteSortTable* fpt = new EmptySortTable();
		FiniteSortTable* cp = fpt;
		fpt = fpt->add(tuple);
		if(fpt != cp) delete(cp);
		_tables.push_back(fpt);
	}
	return this;
}

UnionSortTable* UnionSortTable::remove(const vector<TypedElement>& tuple) {
	if(!(_blacklist->contains(tuple[0]))) _blacklist->add(tuple);
	return this;
}

/** Check if the domains contains a given element **/

bool SortTable::contains(Element e, ElementType t) const {
	switch(t) {
		case ELINT: 
			return contains(e._int);
		case ELDOUBLE:
			return contains(e._double);
		case ELSTRING:
			return contains(e._string);
		case ELCOMPOUND:
			return contains(e._compound);
		default:
			assert(false);
	}
	return false;
}

bool UnionSortTable::contains(string* s) const {
	if(_blacklist->contains(s)) return false;
	for(unsigned int n = 0; n < _tables.size(); ++n)
		if(_tables[n]->contains(s)) return true;
	return false;
}

bool MixedSortTable::contains(string* s) const {
	unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),s,&stringptrslt) - _strtable.begin();
	if(p != _strtable.size() && _strtable[p] == s) return true;
	else {
		double d = stod(*s);
		if(d || isDouble(*s)) {
			unsigned int pd = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
			return (pd != _numtable.size() && _numtable[p] == d);
		}
		else return false;
	}
}

bool RanSortTable::contains(string* s) const {
	int n = stoi(*s);
	if(n || *s == "0") return contains(n);
	else return false;
}

bool FloatSortTable::contains(string* s) const {
	double d = stod(*s);
	if(d || isDouble(*s)) return contains(d);
	else return false;
}

bool IntSortTable::contains(string* s) const {
	int n = stoi(*s);
	if(n || *s == "0") return contains(n);
	else return false;
}

bool StrSortTable::contains(string* s) const {
	unsigned int p = lower_bound(_table.begin(),_table.end(),s,&stringptrslt) - _table.begin();
	return (p != _table.size() && _table[p] == s);
}

bool UnionSortTable::contains(int i) const {
	if(_blacklist->contains(i)) return false;
	for(unsigned int n = 0; n < _tables.size(); ++n)
		if(_tables[n]->contains(i)) return true;
	return false;
}

bool MixedSortTable::contains(int n) const {
	return contains(double(n));
}

bool RanSortTable::contains(int n) const {
	return (n >= _first && n <= _last);
}

bool IntSortTable::contains(int n) const {
	unsigned int p = lower_bound(_table.begin(),_table.end(),n) - _table.begin();
	return (p != _table.size() && _table[p] == n);
}

bool StrSortTable::contains(int) const {
	return false;
}

bool FloatSortTable::contains(int n) const {
	return contains(double(n));
}

bool UnionSortTable::contains(double d) const {
	if(_blacklist->contains(d)) return false;
	for(unsigned int n = 0; n < _tables.size(); ++n)
		if(_tables[n]->contains(d)) return true;
	return false;
}

bool MixedSortTable::contains(double d) const {
	unsigned int p = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
	return (p != _numtable.size() && _numtable[p] == d);
}

bool StrSortTable::contains(double) const {
	return false;
}

bool IntSortTable::contains(double d) const {
	if(double(int(d)) == d) return contains(int(d));
	else return false;
}

bool RanSortTable::contains(double d) const {
	if(double(int(d)) == d) return contains(int(d));
	else return false;
}

bool FloatSortTable::contains(double d) const {
	unsigned int p = lower_bound(_table.begin(),_table.end(),d) - _table.begin();
	return (p != _table.size() && _table[p] == d);
}

bool UnionSortTable::contains(compound* c) const {
	if(_blacklist->contains(c)) return false;
	for(unsigned int n = 0; n < _tables.size(); ++n)
		if(_tables[n]->contains(c)) return true;
	return false;
}

bool MixedSortTable::contains(compound* c) const {
	if(c->_function) {
		unsigned int p = lower_bound(_comtable.begin(),_comtable.end(),c,&compptrslt) - _comtable.begin();
		return (p != _comtable.size() && _comtable[p] == c);
	}
	else return contains((c->_args)[0]);
}

bool StrSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return contains((c->_args)[0]);
}

bool IntSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return contains((c->_args)[0]);
}

bool RanSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return contains((c->_args)[0]);
}

bool FloatSortTable::contains(compound* c) const {
	if(c->_function) return false;
	else return contains((c->_args)[0]);
}


/** Inspectors **/

domelement SortTable::delement(unsigned int n) const {
	//TODO: Optimize by dynamic programming
	Element e = element(n);
	return CPPointer(e,type());
}

ElementType MixedSortTable::type() const {
	assert(!(_strtable.empty() && _comtable.empty()));
	return (_comtable.empty() ? ELSTRING : ELCOMPOUND);
}

Element MixedSortTable::element(unsigned int n) const {
	Element e;
	if(n < _numtable.size()) {
		if(type() == ELSTRING) e._string = IDPointer(dtos(_numtable[n]));
		else {
			Element a; a._double = _numtable[n];
			e._compound = CPPointer(TypedElement(a,ELDOUBLE));
		}
	}
	else if(n < _numtable.size() + _strtable.size()) {
		if(type() == ELSTRING) e._string = _strtable[n-_numtable.size()];
		else {
			Element a; a._string = _strtable[n];
			e._compound = CPPointer(TypedElement(a,ELSTRING));
		}
	}
	else {
		e._compound = _comtable[n-_numtable.size()-_strtable.size()];
	}
	return e;
}

bool UnionSortTable::empty() const {
	if(_tables.empty()) return true;
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		if(!(_tables[n]->finite())) return false;
	}
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		TypedElement te;
		te._type = _tables[n]->type();
		for(unsigned int m = 0; m < _tables[n]->size(); ++m) {
			te._element = _tables[n]->element(m);
			if(!(_blacklist->contains(te))) return false;
		}
	}
	return true;
}

/** Return the position of an element **/

unsigned int IntSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELINT);
	return lower_bound(_table.begin(),_table.end(),el._int) - _table.begin();
}

unsigned int RanSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELINT);
	return el._int - _first;
}

unsigned int FloatSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELDOUBLE);
	unsigned int pos = lower_bound(_table.begin(),_table.end(),el._double) - _table.begin();
	return pos;
}

unsigned int StrSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELSTRING);
	unsigned int pos = lower_bound(_table.begin(),_table.end(),el._string,&stringptrslt) - _table.begin();
	return pos;
}

unsigned int MixedSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	unsigned int pos;
	switch(t) {
		case ELINT:
			pos = lower_bound(_numtable.begin(),_numtable.end(),double(e._int)) - _numtable.begin();
			break;
		case ELDOUBLE:
			pos = lower_bound(_numtable.begin(),_numtable.end(),e._double) - _numtable.begin();
			break;
		case ELSTRING: 
		{
			unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),(e._string),&stringptrslt) - _strtable.begin();
			if(p != _strtable.size() && _strtable[p] == (e._string)) pos = p;
			else {
				double d = stod(*(e._string));
				assert(d || isDouble(*(e._string)));
				pos = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
			}
			break;
		}
		case ELCOMPOUND:
			if(e._compound->_function) {
				pos = lower_bound(_comtable.begin(),_comtable.end(),e._compound,&compptrslt) - _comtable.begin();
			}
			else return MixedSortTable::position((e._compound->_args)[0]._element,(e._compound->_args)[0]._type);
			break;
		default:
			assert(false);
			return 0;
	}
	return pos;
}


/** Debugging **/

string UnionSortTable::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "All tuples in the following table: \n";
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		s += _tables[n]->to_string(spaces+3);
	}
	s += tab + "except \n";
	s += _blacklist->to_string(spaces+3);
	return s;
}

string MixedSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	for(unsigned int n = 0; n < _numtable.size(); ++n) s = s + dtos(_numtable[n]) + ' ';
	for(unsigned int n = 0; n < _strtable.size(); ++n) s = s + *_strtable[n] + ' ';
	for(unsigned int n = 0; n < _comtable.size(); ++n) s = s + _comtable[n]->to_string() + ' ';
	return s + '\n';
}

string RanSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces) + itos(_first) + ".." + itos(_last);
	return s + '\n';
}

string IntSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + itos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + itos(_table[n]);
	}
	return s + '\n';
}

string StrSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + *_table[0];
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + *_table[n];
	}
	return s + '\n';
}

string FloatSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + dtos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + dtos(_table[n]);
	}
	return s + '\n';
}


/********************************
	Predicate interpretations
********************************/

/** Constructors **/

CopyPredTable::CopyPredTable(PredTable* t) : PredTable() { 
	if(typeid(*t) == typeid(CopyPredTable)) {
		CopyPredTable* ct = dynamic_cast<CopyPredTable*>(t);
		assert(typeid(*(ct->table())) != typeid(CopyPredTable));
		_table = ct->table();
		ct->table()->addref();
	}
	else {
		_table = t;
		t->addref();	
	}
}

/** Mutators **/

SortTable* CopySortTable::add(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		SortTable* old = _table;
		_table = _table->add(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		SortTable* pt = _table->clone();
		return pt->add(tuple);
	}
}

SortTable* CopySortTable::remove(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		SortTable* old = _table;
		_table = _table->remove(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		SortTable* pt = _table->clone();
		return pt->remove(tuple);
	}
}

PredTable* CopyPredTable::add(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		PredTable* old = _table;
		_table = _table->add(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		PredTable* pt = _table->clone();
		return pt->add(tuple);
	}
}

PredTable* CopyPredTable::remove(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		PredTable* old = _table;
		_table = _table->remove(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		PredTable* pt = _table->clone();
		return pt->remove(tuple);
	}
}

/*FuncTable* CopyFuncTable::add(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		FuncTable* old = _table;
		_table = _table->add(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		FuncTable* pt = _table->clone();
		return pt->add(tuple);
	}
}

FuncTable* CopyFuncTable::remove(const vector<TypedElement>& tuple) {
	if(_table->nrofrefs() == 1) {
		FuncTable* old = _table;
		_table = _table->remove(tuple);
		if(old != _table) {
			delete(old);
			_table->addref();
		}
		return _table;
	}
	else {
		FuncTable* pt = _table->clone();
		return pt->remove(tuple);
	}
}*/

/*FiniteFuncTable* FiniteFuncTable::add(const vector<TypedElement>& tuple) {
	FinitePredTable* fpt = _ftable->add(tuple);
	if(fpt != _ftable) {
		delete(_ftable);
		_ftable = fpt;
	}
	return this;
}

FiniteFuncTable* FiniteFuncTable::remove(const vector<TypedElement>& tuple) {
	FinitePredTable* fpt = _ftable->remove(tuple);
	if(fpt != _ftable) {
		delete(_ftable);
		_ftable = fpt;
	}
	return this;
}*/


/** Inspectors **/

vector<ElementType> PredTable::types() const {
	vector<ElementType> vet(arity());
	for(unsigned int n = 0; n < vet.size(); ++n) vet[n] = type(n);
	return vet;
}

bool UnionPredTable::empty() const {
	if(_tables.empty()) return true;
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		if(!(_tables[n]->finite())) return false;
	}
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		vector<TypedElement> vte(_tables[n]->arity());
		for(unsigned int c = 0; c < vte.size(); ++c) vte[c]._type = _tables[n]->type(c);
		for(unsigned int m = 0; m < _tables[n]->size(); ++m) {
			for(unsigned int c = 0; c < vte.size(); ++c) vte[c]._element = _tables[n]->element(m,c);
			if(!(_blacklist->PredTable::contains(vte))) return false;
		}
	}
	return true;
}

bool UnionPredTable::contains(const vector<Element>& tuple) const {
	vector<TypedElement> vte(tuple.size());
	for(unsigned int c = 0; c < vte.size(); ++c) {
		vte[c]._type = ELCOMPOUND;
		vte[c]._element = tuple[c];
	}
	if(_blacklist->PredTable::contains(vte)) return false;
	for(unsigned int n = 0; n < _tables.size(); ++n)
		if(_tables[n]->contains(vte)) return true;
	return false;
}

/** Finite tables **/

bool ElementWeakOrdering::operator()(const vector<Element>& x,const vector<Element>& y) const {
	for(unsigned int n = 0; n < _types.size(); ++n) {
		if(ElementUtil::strlessthan(x[n],_types[n],y[n],_types[n])) return true;
		else if(ElementUtil::strlessthan(y[n],_types[n],x[n],_types[n])) return false;
	}
	return false;
}

bool ElementEquality::operator()(const vector<Element>& x,const vector<Element>& y) const {
	for(unsigned int n = 0; n < _types.size(); ++n) {
		if(!ElementUtil::equal(x[n],_types[n],y[n],_types[n])) return false;
	}
	return true;
}

FinitePredTable::FinitePredTable(const FinitePredTable& t) : 
	PredTable(), _types(t.types()), _table(t.table()), _order(t.types()), _equality(t.types()) { }

FinitePredTable::FinitePredTable(const FiniteSortTable& t) :
	PredTable(), _types(t.types()), _table(t.size(),vector<Element>(1)), _order(t.types()), _equality(t.types()) {
	for(unsigned int n = 0; n < t.size(); ++n) {
		_table[n][0] = t.element(n);
	}
}

UnionPredTable* UnionPredTable::clone() const {
	UnionPredTable* upt = new UnionPredTable();
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		upt->add(_tables[n]->clone());
	}
	upt->blacklist(_blacklist->clone());
	return upt;
}

void FinitePredTable::sortunique() {
	sort(_table.begin(),_table.end(),_order);
	vector<unsigned int> doublepos;
	for(unsigned int r = 1; r < _table.size(); ++r) {
		if(_equality(_table[r-1],_table[r])) doublepos.push_back(r);
	}
	doublepos.push_back(_table.size());
	for(unsigned int n = 1; n < doublepos.size(); ++n) {
		for(unsigned int p = doublepos[n-1]+1; p < doublepos[n]; ++p) { 
			_table[p-n] = _table[p];
		}
	}
	for(unsigned int n = 0; n < doublepos.size()-1; ++n) _table.pop_back();
}

void UnionPredTable::sortunique() {
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		_tables[n]->sortunique();
	}
	_blacklist->sortunique();
}

void FinitePredTable::changeElType(unsigned int col, ElementType t) {
	for(unsigned int n = 0; n < _table.size(); ++n) {
		_table[n][col] = ElementUtil::convert(_table[n][col],_types[col],t);
	}
	_types[col] = t;
	_order.changeType(col,t);
	_equality.changeType(col,t);
}

void FinitePredTable::addColumn(ElementType t) {
	assert(_table.size() == 1);
	_types.push_back(t);
	_order.addType(t);
	_equality.addType(t);
	Element e;
	_table[0].push_back(e);
}

bool FinitePredTable::contains(const vector<Element>& vi) const {
	vector<vector<Element> >::const_iterator it = lower_bound(_table.begin(),_table.end(),vi,_order);
	return (it != _table.end() && _equality(*it,vi));
}

FinitePredTable* FinitePredTable::add(const vector<TypedElement>& tuple) {
	vector<Element> ve(tuple.size());
	vector<ElementType> vt(tuple.size());
	for(unsigned int n = 0; n < tuple.size(); ++n) {
		ve[n] = tuple[n]._element;
		vt[n] = tuple[n]._type;
	}
	addRow(ve,vt);
	return this;
}

FinitePredTable* FinitePredTable::remove(const vector<TypedElement>& tuple) {
	bool deleted = false;
	for(unsigned int n = 0; n < _table.size(); ++n) {
		if(ElementUtil::equal(tuple,_table[n],_types)) {
			deleted = true;
		}
		else if(deleted) {
			_table[n-1] = _table[n];
		}
	}
	_table.pop_back();
	return this;
}

UnionPredTable* UnionPredTable::add(const vector<TypedElement>& tuple) {
	if(_blacklist->PredTable::contains(tuple)) _blacklist->remove(tuple);
	bool added = false;
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		if(_tables[n]->contains(tuple)) {
			added = true;
			break;
		}
		else if(typeid(*(_tables[n])) == typeid(FinitePredTable)) {
			_tables[n]->add(tuple);
			added = true;
			break;
		}
	}
	if(!added) {
		vector<ElementType> vet(tuple.size(),ELINT);
		FinitePredTable* fpt = new FinitePredTable(vet);
		fpt->add(tuple);
		_tables.push_back(fpt);
	}
	return this;
}

UnionPredTable* UnionPredTable::remove(const vector<TypedElement>& tuple) {
	if(!(_blacklist->PredTable::contains(tuple))) _blacklist->add(tuple);
	return this;
}

PredTable* FuncPredTable::add(const vector<TypedElement>& tuple) {
	if(!PredTable::contains(tuple)) {
		UnionPredTable* upt = new UnionPredTable();
		upt->add(this);
		upt->add(tuple);
		return upt;
	}
	else return this;
}

PredTable* FuncPredTable::remove(const vector<TypedElement>& tuple) {
	if(PredTable::contains(tuple)) {
		UnionPredTable* upt = new UnionPredTable();
		upt->add(this);
		upt->remove(tuple);
		return upt;
	}
	else return this;
}

void FinitePredTable::addRow(const vector<Element>& vi, const vector<ElementType>& vet) {
	assert(vet.size() == _types.size());
	vector<Element> cvi(vi.size());
	for(unsigned int n = 0; n < vet.size(); ++n) {
		Element e = ElementUtil::convert(vi[n],vet[n],_types[n]);
		if(vet[n] <= _types[n] || ElementUtil::exists(e,_types[n])) cvi[n] = e;
		else {
			ElementType t = ElementUtil::reduce(vi[n],vet[n]);
			cvi[n] = ElementUtil::convert(vi[n],vet[n],t);
			changeElType(n,t);
		}
	}
	_table.push_back(cvi);
}

void PredInter::replace(PredTable* pt, bool ctpf, bool c) {
	if(ctpf) {
		//if(_ctpf != _cfpt) delete(_ctpf);
		_ctpf = pt;
		_ct = c;
	}
	else {
		//if(_ctpf != _cfpt) delete(_cfpt);
		_cfpt = pt;
		_cf = c;
	}
}

void PredInter::add(const vector<TypedElement>& tuple, bool ctpf, bool c) {
	if(ctpf) {
		PredTable* old = _ctpf;
		if(c == _ct) {
			_ctpf = _ctpf->add(tuple);	
		}
		else {
			_ctpf = _ctpf->remove(tuple);
		}
		if(old != _ctpf && old != _cfpt) {
			delete(old);
		}
	}
	else {
		PredTable* old = _cfpt;
		if(c == _cf) {
			_cfpt = _cfpt->add(tuple);
		}
		else {
			_cfpt = _cfpt->remove(tuple);
		}
		if(old != _cfpt && old != _ctpf) delete(old);
	}
}

void PredInter::forcetwovalued() {
	if(_ctpf != _cfpt) {
		delete(_cfpt);
		_cfpt = _ctpf;
		_cf = !_ct;
	}
}

void FuncInter::forcetwovalued() { 
	_pinter->forcetwovalued();	
	if(!_ftable) {
		if(_pinter->ct()) {
			PredTable* pt = _pinter->ctpf();
			if(pt->finite()) {
				if(typeid(*pt) == typeid(CopyPredTable)) {
					pt = dynamic_cast<CopyPredTable*>(pt)->table();
				}
				if(typeid(*pt) == typeid(FinitePredTable)) {
					_ftable = new FiniteFuncTable(dynamic_cast<FinitePredTable*>(pt)->clone());
				}
				else {
					FinitePredTable* npt = new FinitePredTable(pt->types());
					for(unsigned int row = 0; row < pt->size(); ++row) {
						npt->addRow();
						for(unsigned int col = 0; col < pt->arity(); ++col) {
							(*npt)[row][col] = pt->element(row,col);
						}
					}
					_ftable = new FiniteFuncTable(npt);
				}
			}
			else {
				assert(false); // TODO
			}
		}
		else {
			assert(false); // TODO
		}
	}
}


PredInter* PredInter::clone() {
	CopyPredTable* copyctpf1 = new CopyPredTable(_ctpf);
	CopyPredTable* copyctpf2 = new CopyPredTable(_ctpf);
	CopyPredTable* copycfpt1;
	CopyPredTable* copycfpt2;
	if(_ctpf == _cfpt) {
		copycfpt1 = copyctpf1;
		copycfpt2 = copyctpf2;
	}
	else {
		copycfpt1 = new CopyPredTable(_cfpt);
		copycfpt2 = new CopyPredTable(_cfpt);
	}
	_ctpf = copyctpf1;
	_cfpt = copycfpt1;
	return new PredInter(copyctpf2,copycfpt2,_ct,_cf);
}

PredInter::~PredInter() {
	if(_ctpf) {
		if(_ctpf == _cfpt) {
			delete(_ctpf); return;
		}
		else delete(_ctpf);
	}
	if(_cfpt) {
		delete(_cfpt);
	}
}

bool PredInter::istrue(const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],_ctpf->type(n));
	}
	bool result = istrue(ve);
	return result;
}

bool PredTable::contains(const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],type(n));
	}
	bool result = contains(ve);
	return result;
}

bool PredTable::contains(const vector<domelement>& vd) const {
	vector<TypedElement> vte(vd.size());
	for(unsigned int n = 0; n < vd.size(); ++n) {
		vte[n]._element._compound = vd[n];
		vte[n]._type = ELCOMPOUND;
	}
	return contains(vte);
}

bool FinitePredTable::contains(const vector<domelement>& vd) const {
	// NOTE: OPTIMIZATION?? first check the invdyntable?
	if(_dyntable.find(vd) != _dyntable.end()) return true;
	else if(_invdyntable.find(vd) != _invdyntable.end()) return false;
	else {
		if(PredTable::contains(ElementUtil::convert(vd))) {
			_dyntable.insert(vd);
			return true;
		}
		else {
			_invdyntable.insert(vd);
			return false;
		}
	}
}

bool PredTable::contains(const vector<TypedElement*>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(*(vte[n]),type(n));
	}
	return contains(ve);
}

domelement PredTable::delement(unsigned int r, unsigned int c) const {
	//TODO: OPTIMIZE by dynamic programming
	Element e = element(r,c);
	return CPPointer(e,type(c));
}

bool PredInter::isfalse(const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],_cfpt->type(n));
	}
	bool result = isfalse(ve);
	return result;
}

/** Debugging **/

string FinitePredTable::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s;
	for(unsigned int n = 0; n < size(); ++n) {
		s = s + tab;
		for(unsigned int m = 0; m < arity(); ++m) {
			switch(_types[m]) {
				case ELINT:
					s = s + itos(_table[n][m]._int);
					break;
				case ELDOUBLE:
					s = s + dtos((_table[n][m]._double));
					break;
				case ELSTRING:
					s = s + (*(_table[n][m]._string));
					break;
				case ELCOMPOUND:
					s = s + (_table[n][m]._compound)->to_string();
				default:
					assert(false);
			}
			if(m < arity()-1) s = s + ", ";
		}
		s = s + '\n';
	}
	return s;
}

string UnionPredTable::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "All tuples in the following table: \n";
	for(unsigned int n = 0; n < _tables.size(); ++n) {
		s += _tables[n]->to_string(spaces+3);
	}
	s += tab + "except \n";
	s += _blacklist->to_string(spaces+3);
	return s;
}

string PredInter::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s; 
	if(_ct) s = tab + "certainly true tuples:\n";
	else s = tab + "possibly false tuples:\n";
	s = s + _ctpf->to_string(spaces+2);
	if(_cf) s = s + tab + "certainly false tuples:\n";
	else s = s + tab + "possibly true tuples:\n";
	s = s + _cfpt->to_string(spaces+2);
	return s;
}

/*******************************
	Function interpretations
*******************************/

CopyFuncTable::CopyFuncTable(FuncTable* t) : FuncTable() {
	if(typeid(*t) == typeid(CopyFuncTable)) {
		CopyFuncTable* ct = dynamic_cast<CopyFuncTable*>(t);
		assert(typeid(*(ct->table())) != typeid(CopyFuncTable));
		_table = ct->table();
		ct->table()->addref();
	}
	else {
		_table = t;
		t->addref();	
	}
}

FiniteFuncTable::FiniteFuncTable(FinitePredTable* ft) : FuncTable(), _ftable(ft) {
	for(unsigned int n = 0; n < arity(); ++n) {
		_order.addType(ft->type(n));
		_equality.addType(ft->type(n));
	}
}

FuncInter* FuncInter::clone() {
	PredInter* piclone = _pinter->clone();
	CopyFuncTable* cft1 = 0;
	CopyFuncTable* cft2 = 0;
	if(_ftable) {
		cft1 = new CopyFuncTable(_ftable);
		cft2 = new CopyFuncTable(_ftable);
	}
	_ftable = cft1;
	return new FuncInter(cft2,piclone);
}

void FuncInter::add(const vector<TypedElement>& tuple, bool ctpf, bool c) {
	if(_ftable) {
		if(_ftable->finite()) {
			delete(_ftable);
		}
		_ftable = 0;
	}
	_pinter->add(tuple,ctpf,c);
}

Element FiniteFuncTable::operator[](const vector<Element>& vi) const {
	VVE::const_iterator it = lower_bound(_ftable->begin(),_ftable->end(),vi,_order);
	if(it != _ftable->end() && _equality(*it,vi)) return it->back();
	else return ElementUtil::nonexist(type(arity()));
}

Element FuncTable::operator[](const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],type(n));
	}
	Element result = operator[](ve);
	return result;
}

Element FuncTable::operator[](const vector<TypedElement*>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(*(vte[n]),type(n));
	}
	Element result = operator[](ve);
	return result;
}

domelement FuncTable::operator[](const vector<domelement>& vd) const {
	map<vector<domelement>,domelement>::const_iterator it = _dyntable.find(vd);
	if(it != _dyntable.end()) return it->second;
	else {
		Element e = operator[](ElementUtil::convert(vd));
		domelement d = CPPointer(e,outtype());
		_dyntable[vd] = d;
		return d;
	}
}

bool FuncPredTable::contains(const vector<Element>& ve) const {
	vector<Element> in = ve;
	Element out = in.back();
	in.pop_back();
	ElementType t = type(arity()-1);
	return ElementUtil::equal(out,t,(*_ftable)[in],t);
}

string FuncInter::to_string(unsigned int spaces) const {
	if(_ftable) return _ftable->to_string(spaces);
	else return _pinter->to_string(spaces);
}

string FiniteFuncTable::to_string(unsigned int spaces) const {
	return _ftable->to_string(spaces);
}

/*****************
	TableUtils
*****************/

namespace TableUtils {

	PredInter* leastPredInter(const vector<ElementType>& t) {
		FinitePredTable* t1 = new FinitePredTable(t);
		FinitePredTable* t2 = new FinitePredTable(t);
		return new PredInter(t1,t2,true,true);
	}

	PredInter* leastPredInter(unsigned int n) {
		vector<ElementType> vet(n,ELINT);
		return leastPredInter(vet);
	}

	FuncInter* leastFuncInter(const vector<ElementType>& t) {
		PredInter* pt = leastPredInter(t);
		return new FuncInter(0,pt);
	}

	FuncInter* leastFuncInter(unsigned int n) {
		vector<ElementType> vet(n,ELINT);
		return leastFuncInter(vet);
	}

	PredTable* intersection(PredTable* pt1,PredTable* pt2) {
		// this function may only be used in certain cases!
		assert(pt1->arity() == pt2->arity());
		assert(pt1->types() == pt2->types());
		assert(pt1->finite());
		FinitePredTable* upt = new FinitePredTable(pt1->types());
		// add tuples from pt1 that are also in pt2
		for(unsigned int n = 0; n < pt1->size(); ++n)
			if(pt2->contains(pt1->tuple(n)))
				upt->addRow(pt1->tuple(n),pt1->types());
		return upt;
    }

	PredTable* difference(PredTable* pt1,PredTable* pt2) {
		// this function may only be used in certain cases!
		assert(pt1->arity() == pt2->arity());
		assert(pt1->types() == pt2->types());
		assert(pt1->finite());
		FinitePredTable* upt = new FinitePredTable(pt1->types());
		// add tuples from pt1 that are not in pt2
		for(unsigned int n = 0; n < pt1->size(); ++n)
			if(!pt2->contains(pt1->tuple(n)))
				upt->addRow(pt1->tuple(n),pt1->types());
		return upt;
    }

	FiniteSortTable* singletonSort(Element e, ElementType t) {
		EmptySortTable est;
		switch(t) {
			case ELINT: return est.add(e._int);
			case ELDOUBLE: return est.add(e._double);
			case ELSTRING: return est.add(e._string);
			case ELCOMPOUND: return est.add(e._compound);
			default: assert(false); return 0;
		}
	}

	FiniteSortTable* singletonSort(TypedElement t) {
		return singletonSort(t._element,t._type);
	}

	PredTable* project(PredTable* pt, const vector<TypedElement>& vte, const vector<bool>& vb) {
		assert(pt->finite());
		vector<ElementType> vet;
		for(unsigned int n = 0; n < vb.size(); ++n) {
			if(!vb[n]) vet.push_back(pt->type(n));
		}
		FinitePredTable* fpt = new FinitePredTable(vet);
		// TODO: optimize this if vb is of the form (1,1,...,1,0,0,...,0)?
		for(unsigned int r = 0; r < pt->size(); ++r) {
			unsigned int c = 0;
			for( ; c < vb.size(); ++c) {
				if(vb[c]) {
					if(!ElementUtil::equal(vte[c]._element,vte[c]._type,pt->element(r,c),pt->type(c))) break;
				}
			}
			if(c == vb.size()) {
				fpt->addRow();
				unsigned int nc = 0;
				for(unsigned int n = 0; n < vb.size(); ++n) { 
					if(!vb[n]) (*fpt)[fpt->size()-1][nc] = pt->element(r,n);
					++nc;
				}
			}
		}
		return fpt;
	}
}

/*****************
	Structures
*****************/

/** Destructor **/

Structure::~Structure() {
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it)
		delete(it->second);
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it)
		delete(it->second);
	// NOTE: the interpretations of the sorts are deleted when 
	// when the corresponding predicate interpretations are deleted
}

/** Mutators **/

void Structure::forcetwovalued() {
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it)
		it->second->forcetwovalued();
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it)
		it->second->forcetwovalued();
}

void Structure::sortall() {
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ++it)
		it->second->sortunique();
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it)
		it->second->sortunique();
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it)
		it->second->sortunique();
}

Structure* Structure::clone() {
	Structure*	s = new Structure("",ParseInfo());
	s->vocabulary(_vocabulary);
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ++it) {
		CopySortTable* t1 = new CopySortTable(it->second);
		CopySortTable* t2 = new CopySortTable(it->second);
		inter(it->first,t1);
		s->inter(it->first,t2);
	}
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		PredInter* pic = it->second->clone();
		s->inter(it->first,pic);
	}
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		FuncInter* fic = it->second->clone();
		s->inter(it->first,fic);
	}
	return s;
}

void Structure::vocabulary(Vocabulary* v) {
	_vocabulary = v;
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ) {
		map<Sort*,SortTable*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) _sortinter.erase(jt);
	}
	for(map<Predicate*,PredInter*>::iterator it = _predinter.begin(); it != _predinter.end(); ) {
		map<Predicate*,PredInter*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) _predinter.erase(jt);
	}
	for(map<Function*,FuncInter*>::iterator it = _funcinter.begin(); it != _funcinter.end(); ) {
		map<Function*,FuncInter*>::iterator jt = it;
		++it;
		if(!v->contains(jt->first)) _funcinter.erase(jt);
	}
}

void Structure::inter(Sort* s, SortTable* d) const {
	_sortinter[s] = d;
}

void Structure::inter(Predicate* p, PredInter* i) const {
	_predinter[p] = i;
}

void Structure::inter(Function* f, FuncInter* i) const {
	_funcinter[f] = i;
}

void computescore(Sort* s, map<Sort*,unsigned int>& scores) {
	if(scores.find(s) == scores.end()) {
		unsigned int sc = 0;
		for(unsigned int n = 0; n < s->nrParents(); ++n) {
			computescore(s->parent(n),scores);
			if(scores[s->parent(n)] >= sc) sc = scores[s->parent(n)] + 1;
		}
		scores[s] = sc;
	}
}

void Structure::autocomplete() {
	// Assign least tables to every symbol that has no interpretation
/*	for(unsigned int n = 0; n < _predinter.size(); ++n) {
		vector<ElementType> vet(_vocabulary->nbpred(n)->arity(),ELINT);
		if(!_predinter[n]) _predinter[n] = TableUtils::leastPredInter(vet);
		else if(!(_predinter[n]->ctpf())) {
			PredTable* pt = new FinitePredTable(vet);
			_predinter[n]->replace(pt,true,true);
		}
		else if(!(_predinter[n]->cfpt())) {
			PredTable* pt = new FinitePredTable(vet);
			_predinter[n]->replace(pt,false,true);
		}
	}
	for(unsigned int n = 0; n < _funcinter.size(); ++n) {
		vector<ElementType> vet(_vocabulary->nbfunc(n)->nrSorts(),ELINT);
		if(!_funcinter[n]) _funcinter[n] = TableUtils::leastFuncInter(vet);
		else if(!(_funcinter[n]->predinter()->ctpf())) {
			PredTable* pt = new FinitePredTable(vet);
			_funcinter[n]->predinter()->replace(pt,true,true);
		}
		else if(!(_funcinter[n]->predinter()->cfpt())) {
			PredTable* pt = new FinitePredTable(vet);
			_funcinter[n]->predinter()->replace(pt,false,true);
		}
	}
	for(unsigned int n = 0; n < _sortinter.size(); ++n) {
		if(!_sortinter[n]) _sortinter[n] = new EmptySortTable();
	}
*/
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
	
	// Synchronizing sort predicates
	for(unsigned int n = 0; n < _vocabulary->nrNBSorts(); ++n) {
		Predicate* p = _vocabulary->nbsort(n)->pred();
		PredInter* pri = inter(p);
		pri->replace(inter(_vocabulary->nbsort(n)),true,true);
		pri->replace(inter(_vocabulary->nbsort(n)),false,false);
	}

	// Sort the tables
	for(map<Sort*,SortTable*>::iterator it = _sortinter.begin(); it != _sortinter.end(); ++it) {
		it->second->sortunique();
	}
	
}

void Structure::addElement(Element e, ElementType t, Sort* s) {
	string el = ElementUtil::ElementToString(e,t);
	Warning::addingeltosort(el,s->name(),_name);
	SortTable* oldstab = inter(s);
	SortTable* newstab = (dynamic_cast<FiniteSortTable*>(oldstab))->add(e,t);
	inter(s,newstab);
//	if(_sortinter[i] != oldstab) delete(oldstab);	TODO: geeft segfault op 15puzzle.idp
}

void Structure::functioncheck() {
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		Function* f = it->first;
		FuncInter* ft = it->second;
		if(ft) {
			PredInter* pt = ft->predinter();
			PredTable* ct = pt->ctpf();
			PredTable* cf = pt->cfpt();
			// Check if the interpretation is indeed a function
			bool isfunc = true;
			if(ct) {
				vector<ElementType> vet = ct->types(); vet.pop_back();
				ElementEquality eq(vet);
				for(unsigned int r = 1; r < ct->size(); ) {
					if(eq(ct->tuple(r-1),ct->tuple(r))) {
						vector<Element> vel = ct->tuple(r);
						vector<string> vstr(vel.size()-1);
						for(unsigned int c = 0; c < vel.size()-1; ++c) 
							vstr[c] = ElementUtil::ElementToString(vel[c],ct->type(c));
						Error::notfunction(f->name(),name(),vstr);
						while(eq(ct->tuple(r-1),ct->tuple(r))) ++r;
						isfunc = false;
					}
					else ++r;
				}
			}
			// Check if the interpretation is total
			if(isfunc && !(f->partial()) && ct && (ct == cf || (!(pt->cf()) && cf->size() == 0))) {
				unsigned int c = 0;
				unsigned int s = 1;
				for(; c < f->arity(); ++c) {
					if(inter(f->insort(c))) {
						s = s * inter(f->insort(c))->size();
					}
					else break;
				}
				if(c == f->arity()) {
					assert(ct->size() <= s);
					if(ct->size() < s) {
						Error::nottotal(f->name(),name());
					}
				}
			}
		}
	}
}


/** Inspectors **/

SortTable* Structure::inter(Sort* s) const {
	if(s->builtin()) return s->inter();
	map<Sort*,SortTable*>::const_iterator it = _sortinter.find(s);
	if(it != _sortinter.end())
		return it->second;
	else {
		FiniteSortTable* st = new EmptySortTable();
		set<Sort*> ds = s->descendents(_vocabulary);
		for(set<Sort*>::const_iterator it = ds.begin(); it != ds.end(); ++it) {
			SortTable* di = inter(*it);
			assert(di->finite());
			for(unsigned int n = 0; n < di->size(); ++n) {
				FiniteSortTable* tmp = st->add(di->element(n),di->type());
				if(tmp != st) delete(st);
				st = tmp;
			}
		}
		inter(s,st);
		return st;
	}
}

PredInter* Structure::inter(Predicate* p) const {
	if(p->builtin()) return p->inter(*this);
	map<Predicate*,PredInter*>::const_iterator it = _predinter.find(p);
	if(it != _predinter.end())
		return it->second;
	else {
		PredInter* pit = TableUtils::leastPredInter(p->arity());
		inter(p,pit);
		return pit;
	}
}

FuncInter* Structure::inter(Function* f) const {
	if(f->builtin()) return f->inter(*this);
	map<Function*,FuncInter*>::const_iterator it = _funcinter.find(f);
	if(it != _funcinter.end())
		return it->second;
	else {
		FuncInter* fit = TableUtils::leastFuncInter(f->arity()+1);
		inter(f,fit);
		return fit;
	}
}

PredInter* Structure::inter(PFSymbol* s) const {
	if(s->ispred()) return inter(dynamic_cast<Predicate*>(s));
	else return inter(dynamic_cast<Function*>(s))->predinter();
}

/** Debugging **/

string Structure::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "Structure " + _name + " over vocabulary " + _vocabulary->name() + ":\n";
	s = s + tab + "  Sorts:\n";
	for(map<Sort*,SortTable*>::const_iterator it = _sortinter.begin(); it != _sortinter.end(); ++it) {
		s = s + tab + "    " + it->first->to_string() + '\n';
		s = s + tab + "      " + it->second->to_string();
	}
	s = s + tab + "  Predicates:\n";
	for(map<Predicate*,PredInter*>::const_iterator it = _predinter.begin(); it != _predinter.end(); ++it) {
		s = s + tab + "    " + it->first->to_string() + '\n';
		s = s + it->second->to_string(spaces+6);
	}
	s = s + tab + "  Functions:\n";
	for(map<Function*,FuncInter*>::const_iterator it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		s = s + tab + "    " + it->first->to_string() + '\n';
		s = s + it->second->to_string(spaces+6);
	}
	return s;
}

/**********************
	Structure utils
**********************/

/** Convert a structure to a theory of facts **/

class StructConvertor : public Visitor {

	private:
		PFSymbol*					_currsymbol;
		AbstractTheory*				_returnvalue;
		const AbstractStructure*	_structure;

	public:
		StructConvertor(const AbstractStructure* s) : Visitor(), _currsymbol(0), _returnvalue(0), _structure(s) { s->accept(this);	}

		void			visit(const Structure*);
		void			visit(const PredInter*);
		void			visit(const FuncInter*);
		AbstractTheory*	returnvalue()	const { return _returnvalue;	}
		
};

void StructConvertor::visit(const Structure* s) {
	_returnvalue = new Theory("",s->vocabulary(),ParseInfo());
	for(unsigned int n = 0; n < s->vocabulary()->nrNBPreds(); ++n) {
		_currsymbol = s->vocabulary()->nbpred(n);
		visit(s->inter(_currsymbol));
	}
	for(unsigned int n = 0; n < s->vocabulary()->nrNBFuncs(); ++n) {
		_currsymbol = s->vocabulary()->nbfunc(n);
		visit(s->inter(_currsymbol));
	}
}

void StructConvertor::visit(const PredInter* pt) {
	if(pt->ct()) {
		vector<Term*> vt(_currsymbol->nrSorts());
		for(unsigned int r = 0; r < pt->ctpf()->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = pt->ctpf()->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->ctpf()->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,FormParseInfo()));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->ctpf(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrSorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = comp->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,FormParseInfo()));
		}
		delete(comp);
	}
	if(pt->cf()) {
		vector<Term*> vt(_currsymbol->nrSorts());
		for(unsigned int r = 0; r < pt->cfpt()->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = pt->cfpt()->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->cfpt()->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,FormParseInfo()));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->cfpt(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrSorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = comp->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,FormParseInfo()));
		}
		delete(comp);
	}
}

void StructConvertor::visit(const FuncInter* ft) {
	visit(ft->predinter());
	// TODO: do something smarter here ...
}

/** Structure utils **/

namespace StructUtils {
	AbstractTheory*		convert_to_theory(const AbstractStructure* s) { StructConvertor sc(s); return sc.returnvalue();	}

	void changevoc(AbstractStructure* s, Vocabulary* v) {
		s->vocabulary(v);
	}

	PredTable*	complement(const PredTable* pt, const vector<Sort*>& vs, const AbstractStructure* s) {
		vector<SortTable*> tables;
		vector<TypedElement> tuple;
		vector<ElementType> types;
		for(unsigned int n = 0; n < vs.size(); ++n) {
			SortTable* st = s->inter(vs[n]);
			assert(st);
			tables.push_back(st);
			TypedElement e; e._type = st->type();
			tuple.push_back(e);
			types.push_back(st->type());
		}
		FinitePredTable* upt = new FinitePredTable(types);
		SortTableTupleIterator stti(tables);
		if(!stti.empty()) {
			do {
				for(unsigned int n = 0; n < tuple.size(); ++n) {
					tuple[n]._element = stti.value(n);
				}
				if(!pt->contains(tuple)) {
					vector<Element> ve(tuple.size());
					for(unsigned int n = 0; n < tuple.size(); ++n) {
						ve[n] = tuple[n]._element;
					}
					upt->addRow(ve,types);
				}
			} while(stti.nextvalue());
		}
		return upt;
	}

}


/** Iterate over all elements in the cross product of a tuple of SortTables **/

SortTableTupleIterator::SortTableTupleIterator(const vector<SortTable*>& vs) : _tables(vs) {
	for(unsigned int n = 0; n < vs.size(); ++n) {
		_currvalue.push_back(0);
		assert(vs[0]->finite());
		_limits.push_back(vs[0]->size());
	}
}

SortTableTupleIterator::SortTableTupleIterator(const vector<Variable*>& vv, AbstractStructure* str) {
	for(unsigned int n = 0; n < vv.size(); ++n) {
		_currvalue.push_back(0);
		assert(vv[n]->sort());
		SortTable* st = str->inter(vv[n]->sort());
		assert(st);
		assert(st->finite());
		_tables.push_back(st);
		_limits.push_back(st->size());
	}
}

bool SortTableTupleIterator::empty() const {
	for(unsigned int n = 0; n < _limits.size(); ++n) {
		if(_limits[n] == 0) return true;
	}
	return false;
}

bool SortTableTupleIterator::singleton() const {
	for(unsigned int n = 0; n < _limits.size(); ++n) {
		if(_limits[n] != 1) return false;
	}
	return true;
}

bool SortTableTupleIterator::nextvalue() {
	return nexttuple(_currvalue,_limits);
}

ElementType SortTableTupleIterator::type(unsigned int n) const {
	return _tables[n]->type();
}

Element SortTableTupleIterator::value(unsigned int n) const {
	return _tables[n]->element(_currvalue[n]);
}
