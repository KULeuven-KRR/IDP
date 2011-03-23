/************************************
	vocabulary.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include "vocabulary.hpp"
#include "structure.hpp"
#include "common.hpp"

using namespace std;

/************
	Sorts
************/

/**
 * Destructor for sorts. 
 * Deletes the built-in interpretation and removes the sort from the sort hierarchy.
 */
Sort::~Sort() {
	for(set<Sort*>::iterator it = _parents.begin(); it != _parents.end(); ++it)
		(*it)->removeChild(this);
	for(set<Sort*>::iterator it = _children.begin(); it != _children.end(); ++it)
		(*it)->removeParent(this);
	delete(_pred);	
	if(_interpretation) delete(_interpretation);
}

void Sort::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if(_vocabularies.empty()) delete(this);
}

void Sort::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
}

/**
 * Removes a sort from the set of parents
 *
 * PARAMETERS
 *		- parent: the parent that is removed
 */
void Sort::removeParent(Sort* parent) {
	_parents.erase(parent);
}

/**
 * Removes a sort from the set of children
 *
 * PARAMETERS
 *		- child: the child that is removed
 */
void Sort::removeChild(Sort* child) {
	_children.erase(child);
}

/**
 * Generate the predicate that corresponds to the sort
 */
void Sort::generatePred() {
	string predname(_name + "/1");
	vector<Sort*> predsorts(1,this);
	_pred = new Predicate(predname,predsorts,_pi);
}

/**
 * Create an internal sort
 */
Sort::Sort(const string& name, SortTable* inter) : _name(name), _pi(), _interpretation(inter) { 
	generatePred();
}

/**
 * Create a user-declared sort
 */
Sort::Sort(const string& name, const ParseInfo& pi, SortTable* inter) : _name(name), _pi(pi), _interpretation(inter) { 
	generatePred();
}

/**
 * Add p as a parent
 */
void Sort::addParent(Sort* p) {
	pair<set<Sort*>::iterator,bool> changed = _parents.insert(p);
	if(changed.second) p->addChild(this);
}

void Sort::addChild(Sort* c) {
	pair<set<Sort*>::iterator,bool> changed = _children.insert(c);
	if(changed.second) c->addParent(this);
}

inline const string& Sort::name() const { 
	return _name;	
}

inline const ParseInfo& Sort::pi() const { 
	return _pi;		
}

inline Predicate* Sort::pred() const {
	return _pred;
}


/**
 * Compute all ancestors of the sort in the sort hierarchy
 *
 * PARAMETERS
 *		- vocabulary:	if this is not a null-pointer, the set of ancestors is restricted to the ancestors in vocabulary
 */
set<Sort*> Sort::ancestors(const Vocabulary* vocabulary) const {
	set<Sort*> ancest;
	for(set<Sort*>::const_iterator it = _parents.begin(); it != _parents.end(); ++it) {
		if((!vocabulary) || vocabulary->contains(*it)) ancest.insert(*it);
		set<Sort*> temp = (*it)->ancestors(vocabulary);
		ancest.insert(temp.begin(),temp.end());
	}
	return ancest;
}

/**
 * Compute all ancestors of the sort in the sort hierarchy
 *
 * PARAMETERS
 *		- vocabulary:	if this is not a null-pointer, the set of descendents is restricted to the descendents in vocabulary
 */
set<Sort*> Sort::descendents(const Vocabulary* vocabulary) const {
	set<Sort*> descend;
	for(set<Sort*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
		if((!vocabulary) || vocabulary->contains(*it)) descend.insert(*it);
		set<Sort*> temp = (*it)->descendents(vocabulary);
		descend.insert(temp.begin(),temp.end());
	}
	return descend;
}

inline bool Sort::builtin() const {
	return _interpretation != 0;
}

inline const SortTable* Sort::interpretation() const {
	return _interpretation;
}

ostream& Sort::put(ostream& output) const {
	for(set<const Vocabulary*>::iterator it = _vocabularies.begin(); it != _vocabularies.end(); ++it) {
		if((*it)->sort(_name)->size() == 1) {
			(*it)->putname(output);
			break;
		}
	}
	output << _name;
	return output;
}

string Sort::to_string() const {
	stringstream output;
	put(output);
	return output.str();
}

ostream& operator<< (ostream& output, const Sort& sort) { return sort.put(output);	}

namespace SortUtils {

	/**
	 * DESCRIPTION
	 *		Return the unique nearest common ancestor of two sorts. 
	 *
	 * PARAMETERS
	 *		- s1:			the first sort
	 *		- s2:			the second sort
	 *		- vocabulary:	if not 0, search for the nearest common ancestor in the projection of the sort hiearchy on
	 *						this vocabulary
	 *
	 * RETURNS
	 *		The unique nearest common ancestor if it exists, a null-pointer otherwise.
	 */ 
	Sort* resolve(Sort* s1, Sort* s2, const Vocabulary* vocabulary) {
		set<Sort*> ss1 = s1->ancestors(vocabulary); ss1.insert(s1);
		set<Sort*> ss2 = s2->ancestors(vocabulary); ss2.insert(s2);
		set<Sort*> ss;
		for(set<Sort*>::iterator it = ss1.begin(); it != ss1.end(); ++it) {
			if(ss2.find(*it) != ss2.end()) ss.insert(*it);
		}
		vector<Sort*> vs = vector<Sort*>(ss.begin(),ss.end());
		if(vs.empty()) return 0;
		else if(vs.size() == 1) return vs[0];
		else {
			for(unsigned int n = 0; n < vs.size(); ++n) {
				set<Sort*> ds = vs[n]->ancestors(vocabulary);
				for(set<Sort*>::const_iterator it = ds.begin(); it != ds.end(); ++it) ss.erase(*it);
			}
			vs = vector<Sort*>(ss.begin(),ss.end());
			if(vs.size() == 1) return vs[0];
			else return 0;
		}
	}

}

/****************
	Variables
****************/

int Variable::_nvnr = 0;

Variable::~Variable() { 
}

Variable::Variable(const std::string& name, const Sort* sort, const ParseInfo& pi) : _name(name), _sort(sort), _pi(pi) { 
}

Variable::Variable(const Sort* s) : _sort(s) {
	_name = "_var_" + s->name() + "_" + itos(Variable::_nvnr);
	++_nvnr;
}

inline void Variable::sort(const Sort* s) {
	_sort = s;
}

inline const string& Variable::name() const {
	return _name;
}

inline const Sort* Variable::sort() const {
	return _sort;
}

inline const ParseInfo& Variable::pi() const {
	return _pi;
}

ostream& Variable::put(ostream& output) const {
	output << _name;
	if(_sort) output << '[' << *_sort << ']';
	return output;
}

string Variable::to_string() const {
	stringstream output;
	put(output);
	return output.str();
}

ostream& operator<< (ostream& output, const Variable& var) { return var.put(output);	}


/*******************************
	Predicates and functions
*******************************/

PFSymbol::~PFSymbol() {
}

void PFSymbol::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if(_vocabularies.empty()) delete(this);
}

void PFSymbol::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
}

PFSymbol::PFSymbol(const string& name, unsigned int nrsorts, unsigned int binding) : 
	_name(name), _binding(binding), _sorts(nrsorts,0) {
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, unsigned int binding) :
	_name(name), _binding(binding), _sorts(sorts) { 
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi, unsigned int binding) :
	_name(name), _pi(pi), _binding(binding), _sorts(sorts) { 
}

inline const string& PFSymbol::name() const {
	return _name;
}

inline const ParseInfo& PFSymbol::pi() const {
	return _pi;
}

inline unsigned	int PFSymbol::nrSorts() const {
	return _sorts.size();
}

inline Sort* PFSymbol::sort(unsigned int n) const {
	return _sorts[n];
}

inline const vector<Sort*>& PFSymbol::sorts() const {
	return _sorts;
}

inline bool PFSymbol::infix() const {
	return _binding != 0;
}

inline unsigned int PFSymbol::binding() const {
	return _binding;
}

inline bool PFSymbol::hasVocabularies() const {
	return !(_vocabularies.empty());
}

string PFSymbol::to_string() const {
	stringstream output;
	put(output);
	return output.str();
}

int Predicate::_npnr = 0;

Predicate::~Predicate() {
	if(_interpretation) delete(_interpretation);
	if(_overpredgenerator) delete(_overpredgenerator);
	for(set<Predicate*>::iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		if(!(*it)->hasVocabularies()) delete(*it);
	}
}

Predicate::Predicate(const std::string& name,const std::vector<Sort*>& sorts, const ParseInfo& pi, unsigned int binding ) :
	PFSymbol(name,sorts,pi,binding), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const std::string& name,const std::vector<Sort*>& sorts, unsigned int binding) :
	PFSymbol(name,sorts,binding), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const std::set<Predicate*>& overpreds) : 
	PFSymbol((*(overpreds.begin()))->name(),(*(overpreds.begin()))->arity(),(*(overpreds.begin()))->binding()), _interpretation(0), _overpreds(overpreds), _overpredgenerator(0) {
}

Predicate::Predicate(const vector<Sort*>& sorts) : 
	PFSymbol("",sorts,ParseInfo()), _interpretation(0), _overpredgenerator(0) {
	_name = "_internal_predicate_" + itos(_npnr) + "/" + itos(sorts.size());
	++_npnr;
}

Predicate::Predicate(const std::string& name, const std::vector<Sort*>& sorts, PredInterGenerator* inter, unsigned int binding) :
	PFSymbol(name,sorts,binding), _interpretation(inter), _overpredgenerator(0) {
}

inline unsigned int Predicate::arity() const {
	return _sorts.size();	
}

inline bool Predicate::builtin() const {
	return _interpretation != 0;
}

inline bool Predicate::overloaded() const {
	return ((!_overpreds.empty()) || _overpredgenerator != 0);
}

/**
 * \brief Returns the interpretation of a built-in predicate
 *
 * PARAMETERS
 *		 - structure: for some predicates, e.g. =/2 over a type A, the interpretation of A is 
 *		 needed to generate the interpretation for =/2. The structure contains the interpretation of the 
 *		 relevant sorts.
 */
PredInter* Predicate::interpretation(const AbstractStructure& structure) const {
	if(_interpretation) return _interpretation->get(structure);
	else return 0;
}

/**
 * \brief Returns true iff the predicate is equal to, or overloads a given predicate
 *
 * PARAMETERS
 *		- predicate: the given predicate
 */
bool Predicate::contains(const Predicate* predicate) const {
	if(this == predicate) return true;
	else if(_overpredgenerator && _overpredgenerator->contains(predicate)) return true;
	else {
		for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
			if((*it)->contains(predicate)) return true;
		}
		return false;
	}
}

/**
 * \brief Return a unique predicate that is overloaded by the predicate and which has given sorts.
 *
 * PARAMETERS
 *		- sorts: the sorts the returned predicate should have.
 *
 * RETURNS
 *		- The predicate itself if it is not overloaded and matches the given sorts.
 *		- A null-pointer if there is more than one predicate that is overloaded by the predicate
 *		  and matches the given sorts.
 *		- Otherwise, the unique predicate that is overloaded by the predicate and matches the given sorts.
 */
const Predicate* Predicate::resolve(const vector<Sort*>& sorts) const {
	if(overloaded()) {
		const Predicate* candidate = 0;
		if(_overpredgenerator) candidate = _overpredgenerator->resolve(sorts);
		for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
			const Predicate* newcandidate = (*it)->resolve(sorts);
			if(candidate && candidate != newcandidate) return 0;
			else candidate = newcandidate;
		}
		return candidate;
	}
	else if(_sorts == sorts) {
		return this;
	}
	else return 0;
}

/**
 *		\brief Returns a predicate that is overloaded by the predicate and which sorts resolve with the given sorts.
 *		Which predicate is returned may depend on the overpredgenerator. Returns a null-pointer if no
 *		suitable predicate is found.
 *
 * PARAMETERS
 *		- sorts:		the given sorts
 *		- vocabulary:	the vocabulary used for resolving the sorts. Defaults to 0.
 */ 
const Predicate* Predicate::disambiguate(const vector<Sort*>& sorts,const Vocabulary* vocabulary) const {
	if(overloaded()) {
		const Predicate* candidate = 0;
		if(_overpredgenerator) candidate = _overpredgenerator->disambiguate(sorts,vocabulary);
		for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
			const Predicate* newcandidate = (*it)->disambiguate(sorts,vocabulary);
			if(candidate && candidate != newcandidate) return 0;
			else candidate = newcandidate;
		}
		return candidate;
	}
	else {
		for(unsigned int n = 0; n < _sorts.size(); ++n) {
			if(!SortUtils::resolve(sorts[n],_sorts[n],vocabulary)) return 0;
		}
		return this;
	}
}

ostream& Predicate::put(ostream& output) const {
	for(set<const Vocabulary*>::iterator it = _vocabularies.begin(); it != _vocabularies.end(); ++it) {
		if(!(*it)->pred(_name)->overloaded()) {
			(*it)->putname(output);
			break;
		}
	}
	output << _name;
	if(nrSorts() > 0) {
		output << '[' << *_sorts[0];
		for(unsigned int n = 1; n < _sorts.size(); ++n) output << ',' << *_sorts[n];
		output << ']';
	}
	return output;
}

ostream& operator<< (ostream& output, const Predicate& p) { return p.put(output); }

PredGenerator::PredGenerator(const string& name) : _name(name) {
}

ComparisonPredGenerator::ComparisonPredGenerator(const string& name, PredInterGeneratorGenerator* inter, unsigned int binding) : 
	PredGenerator(name), _interpretation(inter), _binding(binding) {
}

ComparisonPredGenerator::~ComparisonPredGenerator() {
	delete(_interpretation);
	for(map<Sort*,Predicate*>::iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		if(!it->second->hasVocabularies()) delete(it->second);
	}
}

/**
 * \brief Returns true iff predicate has the same name as the generator, and both sorts of the predicate are equal
 */
bool ComparisonPredGenerator::contains(const Predicate* predicate) const {
	if(predicate->name() == _name) {
		assert(predicate->arity() == 2);
		return predicate->sort(0) == predicate->sort(1);
	}
	else return false;
}

/**
 * \brief Returns the unique predicate that has the name of the generator and the given sorts
 */
const Predicate* ComparisonPredGenerator::resolve(const vector<Sort*>& sorts) const {
	if(sorts.size() == 2 && sorts[0] == sorts[1]) {
		map<Sort*,Predicate*>::const_iterator it = _overpreds.find(sorts[0]);
		if(it == _overpreds.end()) return disambiguate(sorts);
		else return it->second;
	}
	return 0;
}

/**
 * \brief Returns the predicate P[A,A], where P is the name of the generator and A is the unique least common
 * \brief ancestor of all the given sorts.
 *
 * Returns null-pointer if
 *	- there is no least common ancestor of the non-null pointers among the given sorts.
 *	- the vector of given sorts contains a null-pointer and the least common ancestor has an ancestor in the
 *	given vocabulary
 */
const Predicate* ComparisonPredGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) const {
	Sort* predSort = 0;
	bool sortsContainsZero = false;
	for(vector<Sort*>::const_iterator it = sorts.begin(); it != sorts.end(); ++it) {
		if(*it) {
			if(predSort) {
				predSort = SortUtils::resolve(predSort,*it,vocabulary);
				if(!predSort) return 0;
			}
			else predSort = *it;
		}
		else sortsContainsZero = true;
	}

	Predicate* pred = 0;
	if(predSort && (!sortsContainsZero || !predSort->ancestors(vocabulary).empty())) {
		map<Sort*,Predicate*>::const_iterator it = _overpreds.find(predSort);
		if(it != _overpreds.end()) pred = it->second;
		else {
			vector<Sort*> predSorts(2,predSort); 
			pred = new Predicate(_name,predSorts,_interpretation->get(predSorts),_binding);
			_overpreds[predSort] = pred;
		}
	}
	return pred;
}

namespace PredUtils {

	Predicate* overload(Predicate* p1, Predicate* p2) {
		assert(p1->name() == p2->name());
		if(p1 == p2) return p1;
		set<Predicate*> sp; sp.insert(p1); sp.insert(p2);
		return overload(sp);
	}

	Predicate* overload(const set<Predicate*>& sp) {
		if(sp.empty()) return 0;
		else if(sp.size() == 1) return *(sp.begin());
		else return new Predicate(sp);
	}

}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi) : 
	PFSymbol(name,is,pi), _partial(false), _insorts(is), _outsort(os), _interpretation(0), _overfuncgenerator(0) { 
	_sorts.push_back(os); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi) : 
	PFSymbol(name,sorts,pi), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(0), _overfuncgenerator(0) { 
	_insorts.pop_back(); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os) : 
	PFSymbol(name,is), _partial(false), _insorts(is), _outsort(os), _interpretation(0), _overfuncgenerator(0) { 
	_sorts.push_back(os); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts) : 
	PFSymbol(name,sorts), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(0), _overfuncgenerator(0) { 
	_insorts.pop_back(); 
}

Function::Function(const std::set<Function*>& overfuncs) :
	PFSymbol((*(overfuncs.begin()))->name(),(*(overfuncs.begin()))->nrSorts(),(*(overfuncs.begin()))->binding()), _partial(false), _insorts((*(overfuncs.begin()))->nrSorts() -1,0), _outsort(0), _interpretation(0), _overfuncs(overfuncs), _overfuncgenerator(0) {
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, FuncInterGenerator* inter, unsigned int binding) :
	PFSymbol(name,sorts,binding), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(inter), _overfuncgenerator(0) {	
		_insorts.pop_back();
}

Function::~Function() {
	if(_interpretation) delete(_interpretation);
	if(_overfuncgenerator) delete(_overfuncgenerator);
	for(set<Function*>::iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		if(!(*it)->hasVocabularies()) delete(*it);
	}
}

inline void Function::partial(bool b) {
	_partial = b;
}

inline const vector<Sort*>& Function::insorts() const {
	return _insorts;
}

inline unsigned int Function::arity() const {
	return _insorts.size();
}

inline Sort* Function::insort(unsigned int n) const {
	return _insorts[n];
}

inline Sort* Function::outsort() const {
	return _outsort;
}

inline bool Function::partial() const {
	return _partial;
}

inline bool Function::builtin() const {
	return _interpretation != 0;
}

inline bool Function::overloaded() const {
	return ((!_overfuncs.empty()) || _overfuncgenerator != 0);
}

/**
 * \brief Returns the interpretation of a built-in function
 *
 * PARAMETERS
 *		 - structure: for some functions, e.g. //2 over a type A, the interpretation of A is 
 *		 needed to generate the interpretation for //2. The structure contains the interpretation of the 
 *		 relevant sorts.
 */
FuncInter* Function::interpretation(const AbstractStructure& structure) const {
	if(_interpretation) return _interpretation->get(structure);
	else return 0;
}

/**
 * \brief Returns true iff the function is equal to, or overloads a given function
 *
 * PARAMETERS
 *		- function: the given function
 */
bool Function::contains(const Function* function) const {
	if(this == function) return true;
	else if(_overfuncgenerator && _overfuncgenerator->contains(function)) return true;
	else {
		for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
			if((*it)->contains(function)) return true;
		}
		return false;
	}
}

/**
 * \brief Return a unique function that is overloaded by the function and which has given sorts.
 *
 * PARAMETERS
 *		- sorts: the sorts the returned function should have. Includes the return sort
 *
 * RETURNS
 *		- The function itself if it is not overloaded and matches the given sorts.
 *		- A null-pointer if there is more than one function that is overloaded by the function
 *		  and matches the given sorts.
 *		- Otherwise, the unique function that is overloaded by the function and matches the given sorts.
 */
const Function* Function::resolve(const vector<Sort*>& sorts) const {
	if(overloaded()) {
		const Function* candidate = 0;
		if(_overfuncgenerator) candidate = _overfuncgenerator->resolve(sorts);
		for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
			const Function* newcandidate = (*it)->resolve(sorts);
			if(candidate && candidate != newcandidate) return 0;
			else candidate = newcandidate;
		}
		return candidate;
	}
	else if(_sorts == sorts) {
		return this;
	}
	else return 0;
}

/**
 *		\brief Returns a function that is overloaded by the function and which sorts resolve with the given sorts.
 *		Which function is returned may depend on the overfuncgenerator. Returns a null-pointer if no
 *		suitable function is found.
 *
 * PARAMETERS
 *		- sorts:		the given sorts (includes the output sort)
 *		- vocabulary:	the vocabulary used for resolving the sorts. Defaults to 0.
 */ 
const Function* Function::disambiguate(const vector<Sort*>& sorts,const Vocabulary* vocabulary) const {
	if(overloaded()) {
		const Function* candidate = 0;
		if(_overfuncgenerator) candidate = _overfuncgenerator->disambiguate(sorts,vocabulary);
		for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
			const Function* newcandidate = (*it)->disambiguate(sorts,vocabulary);
			if(candidate && candidate != newcandidate) return 0;
			else candidate = newcandidate;
		}
		return candidate;
	}
	else {
		for(unsigned int n = 0; n < _sorts.size(); ++n) {
			if(!SortUtils::resolve(sorts[n],_sorts[n],vocabulary)) return 0;
		}
		return this;
	}
}

ostream& Function::put(ostream& output) const {
	for(set<const Vocabulary*>::iterator it = _vocabularies.begin(); it != _vocabularies.end(); ++it) {
		if(!(*it)->pred(_name)->overloaded()) {
			(*it)->putname(output);
			break;
		}
	}
	output << _name << '[';
	if(_insorts.size() > 0) {
		output << *_insorts[0];
		for(unsigned int n = 1; n < _insorts.size(); ++n) output << ',' << *_insorts[n];
	}
	output << ':' << *_outsort << ']';
	return output;
}

ostream& operator<< (ostream& output, const Function& f) { return f.put(output); }

namespace FuncUtils {

	Function* overload(Function* f1, Function* f2) {
		assert(f1->name() == f2->name());
		if(f1 == f2) return f1;
		set<Function*> sf; sf.insert(f1); sf.insert(f2);
		return overload(sf);
	}

	Function* overload(const set<Function*>& sf) {
		if(sf.empty()) return 0;
		else if(sf.size() == 1) return *(sf.begin());
		else return new Function(sf);
	}

}

/*****************
	Vocabulary
*****************/

Vocabulary::Vocabulary(const string& name) : _name(name) {
	if(_name != "std") addVocabulary(Vocabulary::std());
}

Vocabulary::Vocabulary(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	if(_name != "std") addVocabulary(Vocabulary::std());
}

Vocabulary::~Vocabulary() {
	for(map<string,set<Sort*> >::iterator it = _name2sort.begin(); it != _name2sort.end(); ++it) {
		for(set<Sort*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			(*jt)->removeVocabulary(this);
		}
	}
	for(map<string,Predicate*>::iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		it->second->removeVocabulary(this);
	}
	for(map<string,Function*>::iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		it->second->removeVocabulary(this);
	}
}

void Vocabulary::addVocabulary(Vocabulary* v) {
	for(map<string,set<Sort*> >::iterator it = v->firstsort(); it != v->lastsort(); ++it) {
		for(set<Sort*>::iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) addSort(*jt);
	}
	for(map<string,Predicate*>::iterator it = v->firstpred(); it != v->lastpred(); ++it) {
		addPred(it->second);
	}
	for(map<string,Function*>::iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
		addFunc(it->second);
	}
}

void Vocabulary::addSort(Sort* s) {
	if(!contains(s)) {
		_name2sort[s->name()].insert(s);
		s->addVocabulary(this);
		addPred(s->pred());
	}
}

void Vocabulary::addPred(Predicate* p) {
	if(!contains(p)) {
		if(_name2pred.find(p->name()) == _name2pred.end()) {
			_name2pred[p->name()] = p;
		}
		else {
			Predicate* ovp = PredUtils::overload(p,_name2pred[p->name()]);
			_name2pred[p->name()] = ovp;
		}
		set<Sort*> ss = p->allsorts();
		for(set<Sort*>::iterator it = ss.begin(); it != ss.end(); ++it) 
			addSort(*it);
	}
}

void Vocabulary::addFunc(Function* f) {
	if(!contains(f)) {
		if(_name2func.find(f->name()) == _name2func.end()) {
			_name2func[f->name()] = f;
		}
		else {
			Function* ovf = FuncUtils::overload(f,_name2func[f->name()]);
			_name2func[f->name()] = ovf;
		}
		set<Sort*> ss = f->allsorts();
		for(set<Sort*>::iterator it = ss.begin(); it != ss.end(); ++it) 
			addSort(*it);
	}
}

/** Inspectors **/

bool Vocabulary::contains(Sort* s) const {
	map<string,set<Sort*> >::const_iterator it = _name2sort.find(s->name());
	if(it != _name2sort.end()) {
		if((it->second).find(s) != (it->second).end()) return true; 
	}
	return false;
}

bool Vocabulary::contains(Predicate* p) const {
	map<string,Predicate*>::const_iterator it = _name2pred.find(p->name());
	if(it != _name2pred.end()) return it->second->contains(p);
	else return false;
}

bool Vocabulary::contains(Function* f) const {
	map<string,Function*>::const_iterator it = _name2func.find(f->name());
	if(it != _name2func.end()) return it->second->contains(f);
	else return false;
}

bool Vocabulary::contains(PFSymbol* s) const {
	if(s->ispred()) {
		return contains(dynamic_cast<Predicate*>(s));
	}
	else {
		return contains(dynamic_cast<Function*>(s));
	}
}

unsigned int Vocabulary::index(Sort* s) const {
	assert(_sort2index.find(s) != _sort2index.end());
	return _sort2index.find(s)->second;
}

unsigned int Vocabulary::index(Predicate* p) const {
	assert(_predicate2index.find(p) != _predicate2index.end());
	return _predicate2index.find(p)->second;
}

unsigned int Vocabulary::index(Function* f) const {
	assert(_function2index.find(f) != _function2index.end());
	return _function2index.find(f)->second;
}

const set<Sort*>* Vocabulary::sort(const string& name) const {
	map<string,set<Sort*> >::const_iterator it = _name2sort.find(name);
	if(it != _name2sort.end()) return &(it->second);
	else return 0;
}

Predicate* Vocabulary::pred(const string& name) const {
	map<string,Predicate*>::const_iterator it = _name2pred.find(name);
	if(it != _name2pred.end()) return it->second;
	else return 0;
}

Function* Vocabulary::func(const string& name) const {
	map<string,Function*>::const_iterator it = _name2func.find(name);
	if(it != _name2func.end())  return it->second;
	else  return 0;
}

vector<Predicate*> Vocabulary::pred_no_arity(const string& name) const {
	vector<Predicate*> vp;
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vp.push_back(it->second);
	}
	return vp;
}

vector<Function*> Vocabulary::func_no_arity(const string& name) const {
	vector<Function*> vf;
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vf.push_back(it->second);
	}
	return vf;
}

/** Debugging **/

string PFSymbol::to_string() const {
	string s  = _name.substr(0,_name.find('/'));	
	if(nrSorts()) {
		if(sort(0)) s += '[' + sort(0)->to_string();
		else s += '[';
		for(unsigned int n = 1; n < nrSorts(); ++n) {
			if(sort(n)) s += ',' + sort(n)->to_string();
			else s += ',';
		}
		s += ']';
	}
	return s;
}

string OverloadedPredicate::to_string() const {
	string s = " ";
	for(unsigned int n = 0; n < _overpreds.size(); ++n) {
		s += _overpreds[n]->to_string();
		s += ' ';
	}
	return s;
}

string OverloadedFunction::to_string() const {
	string s = " ";
	for(unsigned int n = 0; n < _overfuncs.size(); ++n) {
		s += _overfuncs[n]->to_string();
		s += ' ';
	}
	return s;
}

string Vocabulary::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "Vocabulary " + _name + ":\n";
	s = s + tab + "  Sorts:\n";
	for(map<string,set<Sort*> >::const_iterator it = _name2sort.begin(); it != _name2sort.end(); ++it) {
		for(set<Sort*>::const_iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) {
			s = s + tab + "    " + (*jt)->to_string() + '\n';
		}
	}
	s = s + tab + "  Predicates:\n";
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		s = s + tab + "    " + it->second->to_string() + '\n';
	}
	s = s + tab + "  Functions:\n";
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		s = s + tab + "    " + it->second->to_string() + '\n';
	}
	return s;
}
