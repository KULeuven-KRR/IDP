/************************************
	structure.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.hpp"
#include "builtin.hpp"
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

FiniteSortTable* IntSortTable::add(int e) {
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
	_strtable.push_back(e);
	return this;
}

FiniteSortTable* StrSortTable::add(string* e) {
	_table.push_back(e);
	return this;
}

FiniteSortTable* IntSortTable::add(string* e) {
	MixedSortTable* mst = new MixedSortTable();
	for(unsigned int n = 0; n < _table.size(); ++n)
		mst->add(_table[n]);
	mst->add(e);
	return mst;
}

FiniteSortTable* FloatSortTable::add(string* e) {
	MixedSortTable* mst = new MixedSortTable(_table);
	mst->add(e);
	return mst;
}

FiniteSortTable* RanSortTable::add(string* e) {
	MixedSortTable* mst = new MixedSortTable();
	mst->add(_first,_last);
	mst->add(e);
	return mst;
}

FiniteSortTable* EmptySortTable::add(string* e) {
	StrSortTable* sst = new StrSortTable();
	sst->add(e);
	return sst;
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
	vector<string*>::iterator jt = unique(_strtable.begin(),_strtable.end());
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
	vector<string*>::iterator it = unique(_table.begin(),_table.end());
	_table.erase(it,_table.end());
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
		default:
			assert(false);
	}
	return false;
}

bool MixedSortTable::contains(string* s) const {
	unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),s) - _strtable.begin();
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
	return contains(IDPointer(itos(n)));
}

bool FloatSortTable::contains(int n) const {
	return contains(double(n));
}

bool MixedSortTable::contains(double d) const {
	unsigned int p = lower_bound(_numtable.begin(),_numtable.end(),d) - _numtable.begin();
	return (p != _numtable.size() && _numtable[p] == d);
}

bool StrSortTable::contains(double d) const {
	return contains(IDPointer(dtos(d)));
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

Element MixedSortTable::element(unsigned int n) const {
	Element e;
	if(n < _numtable.size()) {
		e._string = IDPointer(dtos(_numtable[n]));
	}
	else {
		e._string = _strtable[n-_numtable.size()];
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
	unsigned int pos = lower_bound(_table.begin(),_table.end(),el._double) - _table.begin();
	return pos;
}

unsigned int StrSortTable::position(Element e, ElementType t) const {
	assert(SortTable::contains(e,t));
	Element el = ElementUtil::convert(e,t,ELSTRING);
	unsigned int pos = lower_bound(_table.begin(),_table.end(),el._string) - _table.begin();
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
			pos = lower_bound(_numtable.begin(),_numtable.end(),e._double) - _numtable.begin();
			break;
		case ELSTRING: 
		{
			unsigned int p = lower_bound(_strtable.begin(),_strtable.end(),(e._string)) - _strtable.begin();
			if(p != _strtable.size() && _strtable[p] == (e._string)) pos = p;
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

string MixedSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	for(unsigned int n = 0; n < _numtable.size(); ++n) s = s + dtos(_numtable[n]) + ' ';
	for(unsigned int n = 0; n < _strtable.size(); ++n) s = s + *_strtable[n] + ' ';
	return s;
}

string RanSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces) + itos(_first) + ".." + itos(_last);
	return s;
}

string IntSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + itos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + itos(_table[n]);
	}
	return s;
}

string StrSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + *_table[0];
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + *_table[n];
	}
	return s;
}

string FloatSortTable::to_string(unsigned int spaces) const {
	string s = tabstring(spaces);
	if(size()) {
		s = s + dtos(_table[0]);
		for(unsigned int n = 1; n < size(); ++n) s = s + ' ' + dtos(_table[n]);
	}
	return s;
}


/********************************
	Predicate interpretations
********************************/

/** Finite tables **/

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
				if((x[n]._double) < (y[n]._double)) {
					return true;
				}
				else if((x[n]._double) > (y[n]._double)) {
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
				if((x[n]._double) != (y[n]._double)) return false;
				break;
			case ELSTRING:
				if((x[n]._string) != (y[n]._string)) return false;
				break;
			default:
				assert(false);
		}
	}
	return true;
}

FinitePredTable::FinitePredTable(const FinitePredTable& t) : 
	PredTable(), _types(t.types()), _table(t.size(),vector<Element>(t.arity())), _order(t.types()), _equality(t.types()) {
	for(unsigned int n = 0; n < t.size(); ++n) {
		for(unsigned int m = 0; m < _types.size(); ++m) {
			switch(_types[m]) {
				case ELINT:
					_table[n][m]._int = t[n][m]._int;
					break;
				case ELDOUBLE:
					_table[n][m]._double = (t[n][m]._double);
					break;
				case ELSTRING:
					_table[n][m]._string = (t[n][m]._string);
					break;
				default:
					assert(false);
			}
		}
	}
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

void FinitePredTable::changeElType(unsigned int col, ElementType t) {
	switch(_types[col]) {
		case ELINT:
			switch(t) {
				case ELDOUBLE:
					for(unsigned int n = 0; n < _table.size(); ++n) {
						_table[n][col]._double = double(_table[n][col]._int);
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
				_table[n][col]._string = IDPointer(string(dtos(_table[n][col]._double)));
			}
			break;
		default:
			assert(false);
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

void FinitePredTable::addRow(const vector<Element>& vi, const vector<ElementType>& vet) {
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
						_table[r][n]._double = double(vi[n]._int); 
						break;
					case ELSTRING:
						_table[r][n]._string = IDPointer(string(itos(vi[n]._int))); 
						break;
					default:
						assert(false);
				}
				break;
			case ELDOUBLE:
				switch(_types[n]) {
					case ELINT:
						changeElType(n,ELDOUBLE);
						_table[r][n]._double = (vi[n]._double);
						break;
					case ELDOUBLE:
						_table[r][n]._double = (vi[n]._double);
						break;
					case ELSTRING:
						_table[r][n]._string = IDPointer(string(dtos(vi[n]._double)));
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
				default:
					assert(false);
			}
			if(m < arity()-1) s = s + ", ";
		}
		s = s + '\n';
	}
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
	s = s + _ctpf->to_string(spaces+2);
	return s;
}

/*******************************
	Function interpretations
*******************************/

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

string FuncInter::to_string(unsigned int spaces) const {
	if(_ftable) return _ftable->to_string(spaces);
	else return _pinter->to_string(spaces);
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

FuncInter* leastFuncInter(const vector<ElementType>& t) {
	PredInter* pt = leastPredInter(t);
	return new FuncInter(0,pt);
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
			PredTable* pt = new FinitePredTable(vet);
			_predinter[n]->replace(pt,true,true);
		}
		else if(!(_predinter[n]->cfpt())) {
			PredTable* pt = new FinitePredTable(vet);
			_predinter[n]->replace(pt,false,true);
		}
	}
	for(unsigned int n = 0; n < _funcinter.size(); ++n) {
		vector<ElementType> vet(_vocabulary->func(n)->nrsorts(),ELINT);
		if(!_funcinter[n]) {
			_funcinter[n] = TableUtils::leastFuncInter(vet);
		}
		else if(!(_funcinter[n]->predinter()->ctpf())) {
			PredTable* pt = new FinitePredTable(vet);
			_funcinter[n]->predinter()->replace(pt,true,true);
		}
		else if(!(_funcinter[n]->predinter()->cfpt())) {
			PredTable* pt = new FinitePredTable(vet);
			_funcinter[n]->predinter()->replace(pt,false,true);
		}
	}
}

/** Inspectors **/

SortTable* Structure::inter(Sort* s) const {
	if(s->builtin()) return s->inter();
	return _sortinter[_vocabulary->index(s)];
}

PredInter* Structure::inter(Predicate* p) const {
	if(p->builtin()) {
		vector<SortTable*> vs(p->arity());
		for(unsigned int n = 0; n < p->arity(); ++n) vs[n] = inter(p->sort(n));
		return p->inter(vs);
	}
	return _predinter[_vocabulary->index(p)];
}

FuncInter* Structure::inter(Function* f) const {
	if(f->builtin()) {
		vector<SortTable*> vs(f->nrsorts());
		for(unsigned int n = 0; n < f->nrsorts(); ++n) vs[n] = inter(f->sort(n));
		return f->inter(vs);
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
	_returnvalue = new Theory("",s->vocabulary(),ParseInfo());
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
				Element e = pt->ctpf()->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->ctpf()->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,ParseInfo()));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->ctpf(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = comp->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(true,_currsymbol,vt,ParseInfo()));
		}
		delete(comp);
	}
	if(pt->cf()) {
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < pt->cfpt()->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = pt->cfpt()->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),pt->cfpt()->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,ParseInfo()));
		}
	}
	else {
		PredTable* comp = StructUtils::complement(pt->cfpt(),_currsymbol->sorts(),_structure);
		vector<Term*> vt(_currsymbol->nrsorts());
		for(unsigned int r = 0; r < comp->size(); ++r) {
			for(unsigned int c = 0; c < vt.size(); ++c) {
				Element e = comp->element(r,c);
				vt[c] = new DomainTerm(_currsymbol->sort(c),comp->type(c),e,ParseInfo());
			}
			_returnvalue->add(new PredForm(false,_currsymbol,vt,ParseInfo()));
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
		FinitePredTable* upt = new FinitePredTable(types);
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
						ve[n] = tuple[n]._element;
					}
					upt->addRow(ve,types);
				}
			} while(nexttuple(iter,limits));
			return upt;
		}
	}

}
