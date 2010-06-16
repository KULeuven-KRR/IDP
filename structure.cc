/************************************
	structure.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.h"
#include "structure.h"
#include "builtin.h"
#include <iostream>
#include <algorithm>

extern int stoi(const string&);
extern string itos(int);
extern double stod(const string&);
extern string dtos(double);
extern bool isDouble(const string&);
extern string tabstring(unsigned int);
extern bool nexttuple(vector<unsigned int>&, const vector<unsigned int>&);

/**************
	Domains
**************/

/** add elements or intervals **/

UserSortTable* MixedSortTable::add(int e) {
	_numtable.push_back(double(e));
	_numstrmem.push_back(0);
	return this;
}

UserSortTable* EmptySortTable::add(int e) {
	IntSortTable* ist = new IntSortTable();
	ist->add(e);
	return ist;
}

UserSortTable* StrSortTable::add(int e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

UserSortTable* IntSortTable::add(int e) {
	_table.push_back(e);
	return this;
}

UserSortTable* FloatSortTable::add(int e) {
	_table.push_back(double(e));
	return this;
}

UserSortTable* RanSortTable::add(int e) {
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

UserSortTable* MixedSortTable::add(const string& e) {
	_strtable.push_back(e);
	return this;
}

UserSortTable* StrSortTable::add(const string& e) {
	_table.push_back(e);
	return this;
}

UserSortTable* IntSortTable::add(const string& e) {
	MixedSortTable* mst = new MixedSortTable();
	for(unsigned int n = 0; n < _table.size(); ++n)
		mst->add(_table[n]);
	mst->add(e);
	return mst;
}

UserSortTable* FloatSortTable::add(const string& e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

UserSortTable* RanSortTable::add(const string& e) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(_first,_last);
	mst->add(e);
	return mst;
}

UserSortTable* EmptySortTable::add(const string& e) {
	StrSortTable* sst = new StrSortTable();
	sst->add(e);
	return sst;
}

UserSortTable* MixedSortTable::add(int f, int l) {
	for(int e = f; e <= l; ++e) {
		_numtable.push_back(double(e));
		_numstrmem.push_back(0);
	}
	return this;
}

UserSortTable* StrSortTable::add(int f, int l) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(f,l);
	return mst;
}

UserSortTable* IntSortTable::add(int f, int l) {
	for(int n = f; n <= l; ++n) 
		_table.push_back(n);
	return this;
}

UserSortTable* FloatSortTable::add(int f, int l) {
	for(int n = f; n <= l; ++n) 
		_table.push_back(double(n));
	return this;
}

UserSortTable* RanSortTable::add(int f, int l) {
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

UserSortTable* EmptySortTable::add(int f, int l) {
	return new RanSortTable(f,l);
}

UserSortTable* MixedSortTable::add(char f, char l) {
	for(char n = f; n <= l; ++n) {
		_strtable.push_back(string(1,n));
	}
	return this;
}

UserSortTable* StrSortTable::add(char f, char l) {
	for(char n = f; n <= l; ++n) {
		_table.push_back(string(1,n));
	}
	return this;
}

UserSortTable* IntSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(f,l);
	for(unsigned int n = 0; n < _table.size(); ++n)
		mst->add(_table[n]);
	return mst;
}

UserSortTable* FloatSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(f,l);
	return mst;
}

UserSortTable* RanSortTable::add(char f, char l) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(_first,_last);
	mst->add(f,l);
	return mst;
}

UserSortTable* EmptySortTable::add(char f, char l) {
	StrSortTable* sst = new StrSortTable();
	sst->add(f,l);
	return sst;
}

UserSortTable* MixedSortTable::add(double e) {
	_numtable.push_back(e);
	_numstrmem.push_back(0);
	return this;
}

UserSortTable* StrSortTable::add(double e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

UserSortTable* IntSortTable::add(double e) {
	if(double(int(e)) == e) return add(int(e));
	else {
		FloatSortTable* fst = new FloatSortTable();
		fst->add(e);
		for(unsigned int n = 0; n < _table.size(); ++n) 
			fst->add(double(_table[n]));
		return fst;
	}
}

UserSortTable* FloatSortTable::add(double e) {
	_table.push_back(double(e));
	return this;
}

UserSortTable* RanSortTable::add(double e) {
	if(double(int(e)) == e) return add(int(e));
	else {
		FloatSortTable* fst = new FloatSortTable();
		fst->add(e);
		fst->add(_first,_last);
		return fst;
	}
}

UserSortTable* EmptySortTable::add(double e) {
	FloatSortTable* fst = new FloatSortTable();
	fst->add(e);
	return fst;
}

/** Sort and remove doubles **/

void MixedSortTable::sortunique() {
	sort(_numtable.begin(),_numtable.end());
	vector<double>::iterator it = unique(_numtable.begin(),_numtable.end());
	_numtable.erase(it,_numtable.end());
	sort(_strtable.begin(),_strtable.end());
	vector<string>::iterator jt = unique(_strtable.begin(),_strtable.end());
	_strtable.erase(jt,_strtable.end());
}

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

void StrSortTable::sortunique() {
	sort(_table.begin(),_table.end());
	vector<string>::iterator it = unique(_table.begin(),_table.end());
	_table.erase(it,_table.end());
}

/** Check if the domains contains a given element **/

bool SortTable::contains(Element e, ElementType t) const {
	switch(t) {
		case ELINT: 
			return contains(e._int);
		case ELDOUBLE:
			return contains(*(e._double));
		case ELSTRING:
			return contains(*(e._string));
		default:
			assert(false);
	}
	return false;
}

bool MixedSortTable::contains(const string& s) const {
	unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),s) - _strtable.begin();
	if(p != _strtable.size() && _strtable[p] == s) return true;
	else {
		double d = stod(s);
		if(d || isDouble(s)) {
			unsigned int pd = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
			return (pd != _numtable.size() && _numtable[p] == d);
		}
		else return false;
	}
}

bool RanSortTable::contains(const string& s) const {
	int n = stoi(s);
	if(n || s == "0") return contains(n);
	else return false;
}

bool FloatSortTable::contains(const string& s) const {
	double d = stod(s);
	if(d || isDouble(s)) return contains(d);
	else return false;
}

bool IntSortTable::contains(const string& s) const {
	int n = stoi(s);
	if(n || s == "0") return contains(n);
	else return false;
}

bool StrSortTable::contains(const string& s) const {
	unsigned int p = lower_bound(_table.begin(),_table.end(),s) - _table.begin();
	return (p != _table.size() && _table[p] == s);
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

bool StrSortTable::contains(int n) const {
	return contains(itos(n));
}

bool FloatSortTable::contains(int n) const {
	return contains(double(n));
}

bool MixedSortTable::contains(double d) const {
	unsigned int p = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
	return (p != _numtable.size() && _numtable[p] == d);
}

bool StrSortTable::contains(double d) const {
	return contains(dtos(d));
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

/** Inspectors **/

Element MixedSortTable::element(unsigned int n) {
	Element e;
	if(n < _numtable.size()) {
		if(!_numstrmem[n]) _numstrmem[n] = new string(dtos(_numtable[n]));
		e._string = _numstrmem[n]; 
	}
	else {
		e._string = &(_strtable[n-_numtable.size()]);
	}
	return e;
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
	unsigned int pos = lower_bound(_table.begin(),_table.end(),*(el._double)) - _table.begin();
	if(t != ELDOUBLE) delete(el._double);
	return pos;
}

unsigned int StrSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELSTRING);
	unsigned int pos = lower_bound(_table.begin(),_table.end(),*(el._string)) - _table.begin();
	if(t != ELSTRING) delete(el._string);
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
			pos = lower_bound(_numtable.begin(),_numtable.end(),*(e._double)) - _numtable.begin();
			break;
		case ELSTRING: 
		{
			unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),(*(e._string))) - _strtable.begin();
			if(p != _strtable.size() && _strtable[p] == (*(e._string))) pos = p;
			else {
				double d = stod(*(e._string));
				assert(d || isDouble(*(e._string)));
				pos = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
			}
		}
	}
	return pos;
}



/** Debugging **/

string MixedSortTable::to_string() const {
	string s;
	for(unsigned int n = 0; n < _numtable.size(); ++n) s = s + dtos(_numtable[n]) + ' ';
	for(unsigned int n = 0; n < _strtable.size(); ++n) s = s + _strtable[n] + ' ';
	return s;
}

string RanSortTable::to_string() const {
	string s = itos(_first) + ".." + itos(_last);
	return s;
}

string IntSortTable::to_string() const {
	if(size()) {
		string s = itos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + itos(_table[n]);
		return s;
	}
	else return "";
}

string StrSortTable::to_string() const {
	if(size()) {
		string s = _table[0];
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + _table[n];
		return s;
	}
	else return "";
}

string FloatSortTable::to_string() const {
	if(size()) {
		string s = dtos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + dtos(_table[n]);
		return s;
	}
	else return "";
}


/********************************
	Predicate interpretations
********************************/

/** Finite tables **/
SortPredTable::SortPredTable(UserSortTable* t) : FinitePredTable(vector<ElementType>(1,t->type())), _table(t) { }

UserPredTable::~UserPredTable() {
	for(unsigned int c = 0; c < _types.size(); ++c) {
		switch(_types[c]) {
			case ELINT:
				break;
			case ELDOUBLE:
				for(unsigned int r = 0; r < _table.size(); ++r) {
					delete(_table[r][c]._double);
				}
				break;
			case ELSTRING:
				for(unsigned int r = 0; r < _table.size(); ++r) {
					delete(_table[r][c]._string);
				}
				break;
			default:
				assert(false);
		}
	}
}

bool ElementWeakOrdering::operator()(const vector<Element>& x,const vector<Element>& y) const {
	for(unsigned int n = 0; n < _types.size(); ++n) {
		switch(_types[n]){
			case ELINT:
				if(x[n]._int < y[n]._int) {
					return true;
				}
				else if(x[n]._int > y[n]._int) {
					return false;
				}
				break;
			case ELDOUBLE:
				if(*(x[n]._double) < *(y[n]._double)) {
					return true;
				}
				else if(*(x[n]._double) > *(y[n]._double)) {
					return false;
				}
				break;
			case ELSTRING:
				if(isDouble(*(x[n]._string))) {
					if(isDouble(*(y[n]._string))) {
						double a = stod(*(x[n]._string));
						double b = stod(*(y[n]._string));
						if(a < b) return true;
						else if(a > b) return false;
					}
					else return true;
				}
				else if(isDouble(*(y[n]._string))) {
					return false;
				}
				else {
					if(*(x[n]._string) < *(y[n]._string)) {
						return true;
					}
					else if(*(x[n]._string) > *(y[n]._string)) {
						return false;
					}
				}
				break;
			default:
				assert(false);
		}
	}
	return false;
}

bool ElementEquality::operator()(const vector<Element>& x,const vector<Element>& y) const {
	for(unsigned int n = 0; n < _types.size(); ++n) {
		switch(_types[n]) {
			case ELINT:
				if(x[n]._int != y[n]._int) return false;
				break;
			case ELDOUBLE:
				if(*(x[n]._double) != *(y[n]._double)) return false;
				break;
			case ELSTRING:
				if(*(x[n]._string) != *(y[n]._string)) return false;
				break;
			default:
				assert(false);
		}
	}
	return true;
}

UserPredTable::UserPredTable(const UserPredTable& t) : 
	FinitePredTable(t.types()), _table(t.size(),vector<Element>(t.arity())), _order(t.types()), _equality(t.types()) {
	for(unsigned int n = 0; n < t.size(); ++n) {
		for(unsigned int m = 0; m < _types.size(); ++m) {
			switch(_types[m]) {
				case ELINT:
					_table[n][m]._int = t[n][m]._int;
					break;
				case ELDOUBLE:
					_table[n][m]._double = new double(*(t[n][m]._double));
					break;
				case ELSTRING:
					_table[n][m]._string = new string(*(t[n][m]._string));
					break;
				default:
					assert(false);
			}
		}
	}
}

void UserPredTable::sortunique() {
	sort(_table.begin(),_table.end(),_order);
	vector<unsigned int> doublepos;
	for(unsigned int r = 1; r < _table.size(); ++r) {
		if(_equality(_table[r-1],_table[r])) doublepos.push_back(r);
	}
	for(unsigned int n = 0; n < doublepos.size(); ++n) {
		for(unsigned int c = 0; c < _types.size(); ++c) {
			switch(_types[c]) {
				case ELINT:
					break;
				case ELDOUBLE:
					delete(_table[doublepos[n]][c]._double);
					break;
				case ELSTRING:
					delete(_table[doublepos[n]][c]._string);
					break;
				default:
					assert(false);
			}
		}
	}
	doublepos.push_back(_table.size());
	for(unsigned int n = 1; n < doublepos.size(); ++n) {
		for(unsigned int p = doublepos[n-1]+1; p < doublepos[n]; ++p) { 
			_table[p-n] = _table[p];
		}
	}
	for(unsigned int n = 0; n < doublepos.size()-1; ++n) _table.pop_back();
}

void UserPredTable::changeElType(unsigned int col, ElementType t) {
	switch(_types[col]) {
		case ELINT:
			switch(t) {
				case ELDOUBLE:
					for(unsigned int n = 0; n < _table.size(); ++n) {
						_table[n][col]._double = new double(_table[n][col]._int);
					}
					break;
				case ELSTRING:
					for(unsigned int n = 0; n < _table.size(); ++n) {
						_table[n][col]._string = new string(itos(_table[n][col]._int));
					}
					break;
				default:
					assert(false);
			}
			break;
		case ELDOUBLE:
			assert(t == ELSTRING);
			for(unsigned int n = 0; n < _table.size(); ++n) {
				_table[n][col]._string = new string(dtos(*(_table[n][col]._double)));
			}
			break;
		default:
			assert(false);
	}
	_types[col] = t;
	_order.changeType(col,t);
	_equality.changeType(col,t);
}

void UserPredTable::addColumn(ElementType t) {
	assert(_table.size() == 1);
	_types.push_back(t);
	_order.addType(t);
	_equality.addType(t);
	Element e;
	_table[0].push_back(e);
}

bool UserPredTable::contains(const vector<Element>& vi) const {
	vector<vector<Element> >::const_iterator it = lower_bound(_table.begin(),_table.end(),vi,_order);
	return (it != _table.end() && _equality(*it,vi));
}

void UserPredTable::addRow(const vector<Element>& vi, const vector<ElementType>& vet) {
	assert(vet.size() == _types.size());
	unsigned int r = _table.size();
	addRow();
	for(unsigned int n = 0; n < vet.size(); ++n) {
		switch(vet[n]) {
			case ELINT:
				switch(_types[n]) {
					case ELINT:
						_table[r][n]._int = vi[n]._int;
						break;
					case ELDOUBLE:
						_table[r][n]._double = new double(vi[n]._int); 
						break;
					case ELSTRING:
						_table[r][n]._string = new string(itos(vi[n]._int)); 
						break;
					default:
						assert(false);
				}
				break;
			case ELDOUBLE:
				switch(_types[n]) {
					case ELINT:
						changeElType(n,ELDOUBLE);
						_table[r][n]._double = new double(*(vi[n]._double));
						break;
					case ELDOUBLE:
						_table[r][n]._double = new double(*(vi[n]._double));
						break;
					case ELSTRING:
						_table[r][n]._string = new string(dtos(*(vi[n]._double)));
						break;
					default:
						assert(false);
				}
				break;
			case ELSTRING:
				switch(_types[n]) {
					case ELINT:
						changeElType(n,ELSTRING);
						_table[r][n]._string = new string(*(vi[n]._string));
						break;
					case ELDOUBLE:
						changeElType(n,ELSTRING);
						_table[r][n]._string = new string(*(vi[n]._string));
						break;
					case ELSTRING:
						_table[r][n]._string = new string(*(vi[n]._string));
						break;
					default:
						assert(false);
				}
				break;
			default:
				assert(false);
		}
	}
}

void SortPredTable::addRow(const vector<Element>& vi, const vector<ElementType>& vet) {
	assert(vi.size() == 1);
	switch(vet[0]) {
		case ELINT:
			_table = _table->add(vi[0]._int);
			break;
		case ELDOUBLE:
			_table = _table->add(*(vi[0]._double));
			break;
		case ELSTRING:
			_table = _table->add(*(vi[0]._string));
			break;
		default:
			assert(false);
	}
	_types[0] = _table->type();
}

bool SortPredTable::contains(const vector<Element>& vi) const {
	assert(vi.size() == 1);
	switch(_types[0]) {
		case ELINT:
			return _table->contains(vi[0]._int);
		case ELDOUBLE:
			return _table->contains(*(vi[0]._double));
		case ELSTRING:
			return _table->contains(*(vi[0]._string));
		default:
			assert(false);
	}
	return false;
}

void PredInter::replace(PredTable* pt, bool ctpf, bool c) {
	if(ctpf) {
		if(_ctpf != _cfpt) delete(_ctpf);
		_ctpf = pt;
		_ct = c;
	}
	else {
		if(_ctpf != _cfpt) delete(_cfpt);
		_cfpt = pt;
		_cf = c;
	}
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
	for(unsigned int n = 0; n < vte.size(); ++n) {
		if(_ctpf->type(n) == ELSTRING) {
			if(ve[n]._string != vte[n]._element._string) delete(ve[n]._string);
		}
		else if(_ctpf->type(n) == ELDOUBLE) {
			if(ve[n]._double != vte[n]._element._double) delete(ve[n]._double);
		}
	}
	return result;
}

bool PredTable::contains(const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],_types[n]);
	}
	bool result = contains(ve);
	for(unsigned int n = 0; n < vte.size(); ++n) {
		if(_types[n] == ELSTRING) {
			if(ve[n]._string != vte[n]._element._string) delete(ve[n]._string);
		}
		else if(_types[n] == ELDOUBLE) {
			if(ve[n]._double != vte[n]._element._double) delete(ve[n]._double);
		}
	}
	return result;
}


bool PredInter::isfalse(const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],_cfpt->type(n));
	}
	bool result = isfalse(ve);
	for(unsigned int n = 0; n < vte.size(); ++n) {
		if(_cfpt->type(n) == ELSTRING) {
			if(ve[n]._string != vte[n]._element._string) delete(ve[n]._string);
		}
		else if(_cfpt->type(n) == ELDOUBLE) {
			if(ve[n]._double != vte[n]._element._double) delete(ve[n]._double);
		}
	}
	return result;
}

/** Debugging **/

string UserPredTable::to_string(unsigned int spaces) const {
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
					s = s + dtos(*(_table[n][m]._double));
					break;
				case ELSTRING:
					s = s + (*(_table[n][m]._string));
					break;
				default:
					assert(false);
			}
			if(m < arity()-1) s = s + ", ";
		}
		s = s + '\n';
	}
	return s;
}

string SortPredTable::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	return tab + _table->to_string() + '\n';
}

string PredInter::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s; 
	if(_ct) s = tab + "certainly true tuples:\n";
	else s = tab + "possibly false tuples:\n";
	s = s + _ctpf->to_string(spaces+2);
	if(_cf) s = s + tab + "certainly false tuples:\n";
	else s = s + tab + "possibly true tuples:\n";
	s = s + _ctpf->to_string(spaces+2);
	return s;
}

/*******************************
	Function interpretations
*******************************/

Element UserFuncInter::operator[](const vector<Element>& vi) const {
	if(_ftable) {
		VVE::const_iterator it = lower_bound(_ftable->begin(),_ftable->end(),vi,_order);
		if(it != _ftable->end() && _equality(*it,vi)) return it->back();
	}
	return ElementUtil::nonexist(_outtype);
}

Element FuncInter::operator[](const vector<TypedElement>& vte) const {
	vector<Element> ve(vte.size());
	for(unsigned int n = 0; n < vte.size(); ++n) {
		ve[n] = ElementUtil::convert(vte[n],_intypes[n]);
	}
	Element result = operator[](ve);
	for(unsigned int n = 0; n < vte.size(); ++n) {
		if(_intypes[n] == ELSTRING) {
			if(ve[n]._string != vte[n]._element._string) delete(ve[n]._string);
		}
		else if(_intypes[n] == ELDOUBLE) {
			if(ve[n]._double != vte[n]._element._double) delete(ve[n]._double);
		}
	}
	return result;
}

string UserFuncInter::to_string(unsigned int spaces) const {
	return _ptable->to_string(spaces);
}

/*****************
	TableUtils
*****************/

namespace TableUtils {

PredInter* leastPredInter(const vector<ElementType>& t) {
	UserPredTable* t1 = new UserPredTable(t);
	UserPredTable* t2 = new UserPredTable(t);
	return new PredInter(t1,t2,true,true);
}

FuncInter* leastFuncInter(const vector<ElementType>& t) {
	PredInter* pt = leastPredInter(t);
	vector<ElementType> in = t;
	ElementType out = in.back();
	in.pop_back();
	return new UserFuncInter(in,out,pt);
}

}

/*****************
	Structures
*****************/

/** Destructor **/

Structure::~Structure() {
	for(unsigned int n = 0; n < _predinter.size(); ++n) 
		if(_predinter[n]) delete(_predinter[n]);
	for(unsigned int n = 0; n < _funcinter.size(); ++n) 
		if(_funcinter[n]) delete(_funcinter[n]);
	// NOTE: the interpretations of the sorts are deleted when 
	// when the corresponding predicate interpretations are deleted
}

/** Mutators **/

void Structure::vocabulary(Vocabulary* v) {
	_sortinter = vector<SortTable*>(v->nrSorts(),0);
	_predinter = vector<PredInter*>(v->nrPreds(),0);
	_funcinter = vector<FuncInter*>(v->nrFuncs(),0);
	_vocabulary = v;
}

void Structure::inter(Sort* s, SortTable* d) {
	_sortinter[_vocabulary->index(s)] = d;
}

void Structure::inter(Predicate* p, PredInter* i) {
	_predinter[_vocabulary->index(p)] = i;
}

void Structure::inter(Function* f, FuncInter* i) {
	_funcinter[_vocabulary->index(f)] = i;
}

void Structure::close() {
	for(unsigned int n = 0; n < _predinter.size(); ++n) {
		vector<ElementType> vet(_vocabulary->pred(n)->arity(),ELINT);
		if(!_predinter[n]) {
			_predinter[n] = TableUtils::leastPredInter(vet);
		}
		else if(!(_predinter[n]->ctpf())) {
			PredTable* pt = new UserPredTable(vet);
			_predinter[n]->replace(pt,true,true);
		}
		else if(!(_predinter[n]->cfpt())) {
			PredTable* pt = new UserPredTable(vet);
			_predinter[n]->replace(pt,false,true);
		}
	}
	for(unsigned int n = 0; n < _funcinter.size(); ++n) {
		vector<ElementType> vet(_vocabulary->func(n)->nrsorts(),ELINT);
		if(!_funcinter[n]) {
			_funcinter[n] = TableUtils::leastFuncInter(vet);
		}
		else if(!(_funcinter[n]->predinter()->ctpf())) {
			PredTable* pt = new UserPredTable(vet);
			_funcinter[n]->predinter()->replace(pt,true,true);
		}
		else if(!(_funcinter[n]->predinter()->cfpt())) {
			PredTable* pt = new UserPredTable(vet);
			_funcinter[n]->predinter()->replace(pt,false,true);
		}
	}
}

/** Inspectors **/

SortTable* Structure::inter(Sort* s) const {
	if(s->builtin()) return Builtin::inter(s);
	return _sortinter[_vocabulary->index(s)];
}

PredInter* Structure::inter(Predicate* p) const {
	if(p->builtin()) {
		vector<SortTable*> vs(p->arity());
		for(unsigned int n = 0; n < p->arity(); ++n) vs[n] = inter(p->sort(n));
		return Builtin::inter(p,vs);
	}
	return _predinter[_vocabulary->index(p)];
}

FuncInter* Structure::inter(Function* f) const {
	if(f->builtin()) {
		vector<SortTable*> vs(f->nrsorts());
		for(unsigned int n = 0; n < f->nrsorts(); ++n) vs[n] = inter(f->sort(n));
		return Builtin::inter(f,vs);
	}
	return _funcinter[_vocabulary->index(f)];
}

PredInter* Structure::inter(PFSymbol* s) const {
	if(s->ispred()) return inter(dynamic_cast<Predicate*>(s));
	else return inter(dynamic_cast<Function*>(s))->predinter();
}

bool Structure::hasInter(Sort* s) const {
	return (inter(s) != 0);
}

bool Structure::hasInter(Predicate* p) const {
	return (inter(p) != 0);
}

bool Structure::hasInter(Function* f) const {
	return (inter(f) != 0);
}

/** Debugging **/

string Structure::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "Structure " + _name + " over vocabulary " + _vocabulary->name() + ":\n";
	s = s + tab + "  Sorts:\n";
	for(unsigned int n = 0; n < _sortinter.size(); ++n) {
		s = s + tab + "    " + _vocabulary->sort(n)->to_string() + '\n';
		if(_sortinter[n]) s = s + tab + "      " + _sortinter[n]->to_string() + '\n';
		else s = s + tab + "      no domain\n";
	}
	s = s + tab + "  Predicates:\n";
	for(unsigned int n = 0; n < _predinter.size(); ++n) {
		s = s + tab + "    " + _vocabulary->pred(n)->to_string() + '\n';
		if(_predinter[n]) s = s + _predinter[n]->to_string(spaces+6);
		else s = s + tab + "      no interpretation\n";
	}
	s = s + tab + "  Functions:\n";
	for(unsigned int n = 0; n < _funcinter.size(); ++n) {
		s = s + tab + "    " + _vocabulary->func(n)->to_string() + '\n';
		if(_funcinter[n]) s = s + _funcinter[n]->to_string(spaces+6);
		else s = s + tab + "      no interpretation\n";
	}
	return s;
}

/**********************
	Structure utils
**********************/

/** Convert a structure to a theory of facts **/

class StructConvertor : public Visitor {

	private:
		PFSymbol*	_currsymbol;
		Theory*		_returnvalue;
		Structure*	_structure;

	public:
		StructConvertor(Structure* s) : Visitor(), _currsymbol(0), _returnvalue(0), _structure(s) { s->accept(this);	}

		void	visit(Structure*);
		void	visit(PredInter*);
		void	visit(FuncInter*);
		Theory*	returnvalue()	const { return _returnvalue;	}
		
};

void StructConvertor::visit(Structure* s) {
	_returnvalue = new Theory("",s->vocabulary(),0);
	for(unsigned int n = 0; n < s->vocabulary()->nrPreds(); ++n) {
		_currsymbol = s->vocabulary()->pred(n);
		visit(s->predinter(n));
	}
	for(unsigned int n = 0; n < s->vocabulary()->nrFuncs(); ++n) {
		_currsymbol = s->vocabulary()->func(n);
		visit(s->funcinter(n));
	}
}

void StructConvertor::visit(PredInter* pt) {
	if(pt->ct()) {
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < pt->ctpf()->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = ElementUtil::clone(pt->ctpf()->element(r,c),pt->ctpf()->type(c));
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->ctpf()->type(c),e,0);
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,0));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->ctpf(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = ElementUtil::clone(comp->element(r,c),comp->type(c));
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,0);
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,0));
		}
		delete(comp);
	}
	if(pt->cf()) {
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < pt->cfpt()->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = ElementUtil::clone(pt->cfpt()->element(r,c),pt->cfpt()->type(c));
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->cfpt()->type(c),e,0);
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,0));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->cfpt(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = ElementUtil::clone(comp->element(r,c),comp->type(c));
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,0);
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,0));
		}
		delete(comp);
	}
}

void StructConvertor::visit(FuncInter* ft) {
	visit(ft->predinter());
	// TODO: do something smarter here ...
}

/** Structure utils **/

namespace StructUtils {
	Theory*		convert_to_theory(Structure* s) { StructConvertor sc(s); return sc.returnvalue();	}

	PredTable*	complement(PredTable* pt,const vector<Sort*>& vs, Structure* s) {
		vector<unsigned int> limits;
		vector<SortTable*> tables;
		vector<TypedElement> tuple;
		vector<ElementType> types;
		bool empty = false;
		for(unsigned int n = 0; n < vs.size(); ++n) {
			SortTable* st = s->inter(vs[n]);
			assert(st);
			assert(st->finite());
			if(st->empty()) empty = true;
			limits.push_back(st->size());
			tables.push_back(st);
			TypedElement e; e._type = st->type();
			tuple.push_back(e);
			types.push_back(st->type());
		}
		UserPredTable* upt = new UserPredTable(types);
		if(empty) return upt;
		else {
			vector<unsigned int> iter(limits.size(),0);
			do {
				for(unsigned int n = 0; n < tuple.size(); ++n) {
					tuple[n]._element = tables[n]->element(iter[n]);
				}
				if(!pt->contains(tuple)) {
					vector<Element> ve(tuple.size());
					for(unsigned int n = 0; n < tuple.size(); ++n) {
						ve[n] = ElementUtil::clone(tuple[n]);
					}
					upt->addRow(ve,types);
				}
			} while(nexttuple(iter,limits));
			return upt;
		}
	}

}
