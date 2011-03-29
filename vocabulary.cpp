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
			output << "::";
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

Variable::Variable(const std::string& name, Sort* sort, const ParseInfo& pi) : _name(name), _sort(sort), _pi(pi) { 
}

Variable::Variable(Sort* s) : _sort(s) {
	_name = "_var_" + s->name() + "_" + itos(Variable::_nvnr);
	++_nvnr;
}

inline void Variable::sort(Sort* s) {
	_sort = s;
}

inline const string& Variable::name() const {
	return _name;
}

inline Sort* Variable::sort() const {
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

PFSymbol::PFSymbol(const string& name, unsigned int nrsorts, bool infix) : 
	_name(name), _sorts(nrsorts,0), _infix(infix) {
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, bool infix) :
	_name(name), _sorts(sorts), _infix(infix) { 
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi, bool infix) :
	_name(name), _pi(pi), _sorts(sorts), _infix(infix) { 
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
	return _infix;
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

set<Sort*> Predicate::allsorts() const {
	set<Sort*> ss;
	ss.insert(_sorts.begin(),_sorts.end());
	if(_overpredgenerator) {
		set<Sort*> os = _overpredgenerator->allsorts();
		ss.insert(os.begin(),os.end());
	}
	ss.erase(0);
	return ss;
}

Predicate::~Predicate() {
	if(_interpretation) delete(_interpretation);
	if(_overpredgenerator) delete(_overpredgenerator);
}

void Predicate::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if(overloaded()) _overpredgenerator->removeVocabulary(vocabulary);
	if(_vocabularies.empty()) delete(this);
}

void Predicate::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
	if(overloaded()) _overpredgenerator->addVocabulary(vocabulary);
}


Predicate::Predicate(const std::string& name,const std::vector<Sort*>& sorts, const ParseInfo& pi, bool infix) :
	PFSymbol(name,sorts,pi,infix), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const std::string& name,const std::vector<Sort*>& sorts, bool infix) :
	PFSymbol(name,sorts,infix), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const vector<Sort*>& sorts) : 
	PFSymbol("",sorts,ParseInfo()), _interpretation(0), _overpredgenerator(0) {
	_name = "_internal_predicate_" + itos(_npnr) + "/" + itos(sorts.size());
	++_npnr;
}

Predicate::Predicate(const std::string& name, const std::vector<Sort*>& sorts, PredInterGenerator* inter, bool infix) :
	PFSymbol(name,sorts,infix), _interpretation(inter), _overpredgenerator(0) {
}

Predicate::Predicate(PredGenerator* generator) :
	PFSymbol(generator->name(),generator->arity(),generator->infix()), _interpretation(0), _overpredgenerator(generator) {
}

inline unsigned int Predicate::arity() const {
	return _sorts.size();	
}

inline bool Predicate::builtin() const {
	return _interpretation != 0;
}

inline bool Predicate::overloaded() const {
	return (_overpredgenerator != 0);
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
	else return false; 
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
Predicate* Predicate::resolve(const vector<Sort*>& sorts) {
	if(overloaded()) return _overpredgenerator->resolve(sorts); 
	else if(_sorts == sorts) return this;
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
Predicate* Predicate::disambiguate(const vector<Sort*>& sorts,const Vocabulary* vocabulary) {
	if(overloaded()) return _overpredgenerator->disambiguate(sorts,vocabulary); 
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
			output << "::";
			break;
		}
	}
	output << _name;
	if(!overloaded()) {
		if(nrSorts() > 0) {
			output << '[' << *_sorts[0];
			for(unsigned int n = 1; n < _sorts.size(); ++n) output << ',' << *_sorts[n];
			output << ']';
		}
	}
	return output;
}

ostream& operator<< (ostream& output, const Predicate& p) { return p.put(output); }

PredGenerator::PredGenerator(const string& name, unsigned int arity, bool infix) : _name(name), _arity(arity), _infix(infix) {
}

inline const string& PredGenerator::name() const {
	return _name;
}

inline unsigned int PredGenerator::arity() const {
	return _arity;
}

inline bool PredGenerator::infix() const {
	return _infix;
}

EnumeratedPredGenerator::EnumeratedPredGenerator(const set<Predicate*>& overpreds) :
	PredGenerator((*(overpreds.begin()))->name(),(*(overpreds.begin()))->arity(),(*(overpreds.begin()))->infix()), _overpreds(overpreds) {
}

bool EnumeratedPredGenerator::contains(const Predicate* predicate) const {
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		if((*it)->contains(predicate)) return true;
	}
	return false;
}

/**
 * \brief Returns the unique predicate that is contained in the generator and that has the given sorts.
 * \brief Returns a null-pointer if such a predicate does not exist or is not unique
 */
Predicate* EnumeratedPredGenerator::resolve(const vector<Sort*>& sorts) {
	Predicate* candidate = 0;
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		Predicate* newcandidate = (*it)->resolve(sorts);
		if(candidate && candidate != newcandidate) return 0;
		else candidate = newcandidate;
	}
	return candidate;
}

/**
 * \brief Returns the unique predicate that is contained in the generator and which sorts resolve with the given sorts.
 * \brief Returns a null-pointer if such a predicate does not exist or is not unique
 */
Predicate* EnumeratedPredGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	Predicate* candidate = 0;
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		Predicate* newcandidate = (*it)->disambiguate(sorts,vocabulary);
		if(candidate && candidate != newcandidate) return 0;
		else candidate = newcandidate;
	}
	return candidate;
}

set<Sort*> EnumeratedPredGenerator::allsorts() const {
	set<Sort*> ss;
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		set<Sort*> os = (*it)->allsorts();
		ss.insert(os.begin(),os.end());
	}
	ss.erase(0);
	return ss;
}

void EnumeratedPredGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		(*it)->addVocabulary(vocabulary);
	}
}

void EnumeratedPredGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for(set<Predicate*>::const_iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		(*it)->removeVocabulary(vocabulary);
	}
}

ComparisonPredGenerator::ComparisonPredGenerator(const string& name, PredInterGeneratorGenerator* inter) : 
	PredGenerator(name,2,true), _interpretation(inter) {
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
Predicate* ComparisonPredGenerator::resolve(const vector<Sort*>& sorts) {
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
Predicate* ComparisonPredGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
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
			pred = new Predicate(_name,predSorts,_interpretation->get(predSorts));
			_overpreds[predSort] = pred;
		}
	}
	return pred;
}

set<Sort*> ComparisonPredGenerator::allsorts() const {
	set<Sort*> ss;
	return ss;
}

void ComparisonPredGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for(map<Sort*,Predicate*>::iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		it->second->addVocabulary(vocabulary);
	}
}

void ComparisonPredGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for(map<Sort*,Predicate*>::iterator it = _overpreds.begin(); it != _overpreds.end(); ++it) {
		it->second->removeVocabulary(vocabulary);
	}
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
		else {
			EnumeratedPredGenerator* epg = new EnumeratedPredGenerator(sp);
			return new Predicate(epg);
		}
	}

}

set<Sort*> Function::allsorts() const {
	set<Sort*> ss;
	ss.insert(_sorts.begin(),_sorts.end());
	if(_overfuncgenerator) {
		set<Sort*> os = _overfuncgenerator->allsorts();
		ss.insert(os.begin(),os.end());
	}
	ss.erase(0);
	return ss;
}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi, unsigned int binding) : 
	PFSymbol(name,is,pi), _partial(false), _insorts(is), _outsort(os), _interpretation(0), _overfuncgenerator(0), _binding(binding) { 
	_sorts.push_back(os); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, unsigned int binding) : 
	PFSymbol(name,sorts,pi), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(0), _overfuncgenerator(0), _binding(binding) { 
	_insorts.pop_back(); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, unsigned int binding) : 
	PFSymbol(name,is), _partial(false), _insorts(is), _outsort(os), _interpretation(0), _overfuncgenerator(0), _binding(binding) { 
	_sorts.push_back(os); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, unsigned int binding) : 
	PFSymbol(name,sorts), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(0), _overfuncgenerator(0), _binding(binding) { 
	_insorts.pop_back(); 
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, FuncInterGenerator* inter, unsigned int binding) :
	PFSymbol(name,sorts,binding != 0), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(inter), _overfuncgenerator(0), _binding(binding) {	
		_insorts.pop_back();
}

Function::Function(FuncGenerator* generator) :
	PFSymbol(generator->name(),generator->arity()+1,generator->binding() != 0), _partial(true), _insorts(generator->arity(),0), _outsort(0), _interpretation(0), _overfuncgenerator(generator) {
}

Function::~Function() {
	if(_interpretation) delete(_interpretation);
	if(_overfuncgenerator) delete(_overfuncgenerator);
}

void Function::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if(overloaded()) _overfuncgenerator->removeVocabulary(vocabulary);
	if(_vocabularies.empty()) delete(this);
}

void Function::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
	if(overloaded()) _overfuncgenerator->addVocabulary(vocabulary);
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
	return (_overfuncgenerator != 0);
}

inline unsigned int Function::binding() const {
	return _binding;
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
	else return false;
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
Function* Function::resolve(const vector<Sort*>& sorts) {
	if(overloaded()) return _overfuncgenerator->resolve(sorts); 
	else if(_sorts == sorts) return this;
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
Function* Function::disambiguate(const vector<Sort*>& sorts,const Vocabulary* vocabulary) {
	if(overloaded()) return _overfuncgenerator->disambiguate(sorts,vocabulary); 
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
			output << "::";
			break;
		}
	}
	output << _name;
	if(!overloaded()) {
		output << _name << '[';
		if(_insorts.size() > 0) {
			output << *_insorts[0];
			for(unsigned int n = 1; n < _insorts.size(); ++n) output << ',' << *_insorts[n];
		}
		output << ':' << *_outsort << ']';
	}
	return output;
}

ostream& operator<< (ostream& output, const Function& f) { return f.put(output); }

inline const string& FuncGenerator::name() const {
	return _name;
}

inline unsigned	int FuncGenerator::arity() const {
	return _arity;
}

inline unsigned int FuncGenerator::binding() const {
	return _binding;
}

EnumeratedFuncGenerator::EnumeratedFuncGenerator(const set<Function*>& overfuncs) :
	FuncGenerator((*(overfuncs.begin()))->name(),(*(overfuncs.begin()))->arity(),(*(overfuncs.begin()))->binding()), _overfuncs(overfuncs) {
}

bool EnumeratedFuncGenerator::contains(const Function* function) const {
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		if((*it)->contains(function)) return true;
	}
	return false;
}

/**
 * \brief Returns the unique function that is contained in the generator and that has the given sorts.
 * \brief Returns a null-pointer if such a function does not exist or is not unique
 */
Function* EnumeratedFuncGenerator::resolve(const vector<Sort*>& sorts) {
	Function* candidate = 0;
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		Function* newcandidate = (*it)->resolve(sorts);
		if(candidate && candidate != newcandidate) return 0;
		else candidate = newcandidate;
	}
	return candidate;
}

/**
 * \brief Returns the unique function that is contained in the generator and which sorts resolve with the given sorts.
 * \brief Returns a null-pointer if such a function does not exist or is not unique
 */
Function* EnumeratedFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	Function* candidate = 0;
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		Function* newcandidate = (*it)->disambiguate(sorts,vocabulary);
		if(candidate && candidate != newcandidate) return 0;
		else candidate = newcandidate;
	}
	return candidate;
}

set<Sort*> EnumeratedFuncGenerator::allsorts() const {
	set<Sort*> ss;
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		set<Sort*> os = (*it)->allsorts();
		ss.insert(os.begin(),os.end());
	}
	ss.erase(0);
	return ss;
}

void EnumeratedFuncGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		(*it)->addVocabulary(vocabulary);
	}
}

void EnumeratedFuncGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for(set<Function*>::const_iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		(*it)->removeVocabulary(vocabulary);
	}
}

IntFloatFuncGenerator::IntFloatFuncGenerator(Function* intfunc, Function* floatfunc) : 
	FuncGenerator(intfunc->name(),intfunc->arity(),intfunc->binding()), _intfunction(intfunc), _floatfunction(floatfunc) {
}

bool IntFloatFuncGenerator::contains(const Function* function) const {
	return (function == _intfunction || function == _floatfunction);
}

/**
 * Returns the integer function if the vector of sorts only contains int, 
 * the float function if it only contains float, and a null-pointer otherwise.
 */
Function* IntFloatFuncGenerator::resolve(const vector<Sort*>& sorts) {
	assert(sorts.size() == 2 || sorts.size() == 3);
	if(sorts[0] == sorts[1] && (sorts.size() == 2 || sorts[1] == sorts[2])) {
		Sort* intsort = *((Vocabulary::std()->sort("int"))->begin());
		Sort* floatsort = *((Vocabulary::std()->sort("float"))->begin());
		if(sorts[0] == intsort) return _intfunction;
		else if(sorts[0] == floatsort) return _floatfunction;
		else return 0;
	}
	return 0;
}

/**
 * Returns a null-pointer if more than one of the sorts is a null-pointer, 
 * or one of the sorts that is not a null-pointer, is not a subsort of float.
 * Otherwise, return the integer function if all sorts are a subsort of int, 
 * and the float function if at least one sort is not a subsort of _int.
 */
Function* IntFloatFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	unsigned int zerocounter = 0;
	bool isfloat = false;
	Sort* intsort = *((Vocabulary::std()->sort("int"))->begin());
	Sort* floatsort = *((Vocabulary::std()->sort("float"))->begin());
	for(vector<Sort*>::const_iterator it = sorts.begin(); it != sorts.end(); ++it) {
		if(*it) {
			if(SortUtils::resolve(intsort,*it,vocabulary) != intsort) {
				if(SortUtils::resolve(floatsort,*it,vocabulary) == floatsort) isfloat = true;
				else return 0;
			}
		}
		else {
			++zerocounter;
			if(zerocounter > 1) return 0;
		}
	}
	return isfloat ? _floatfunction : _intfunction;
}

/**
 * \brief Returns sorts int and float
 */
set<Sort*> IntFloatFuncGenerator::allsorts() const {
	set<Sort*> ss;
	ss.insert(*(Vocabulary::std()->sort("int")->begin()));
	ss.insert(*(Vocabulary::std()->sort("float")->begin()));
	return ss;
}

void IntFloatFuncGenerator::addVocabulary(const Vocabulary* vocabulary) {
	_intfunction->addVocabulary(vocabulary);
	_floatfunction->addVocabulary(vocabulary);
}

void IntFloatFuncGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	_intfunction->removeVocabulary(vocabulary);
	_floatfunction->removeVocabulary(vocabulary);
}

OrderFuncGenerator::OrderFuncGenerator(const string& name, unsigned int arity, FuncInterGeneratorGenerator* inter) : 
	FuncGenerator(name,arity,0), _interpretation(inter) {
}

OrderFuncGenerator::~OrderFuncGenerator() {
	delete(_interpretation);
	for(map<Sort*,Function*>::iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		if(!it->second->hasVocabularies()) delete(it->second);
	}
}

/**
 * \brief Returns true iff function has the same name as the generator, and all sorts of function are equal.
 */
bool OrderFuncGenerator::contains(const Function* function) const {
	if(function->name() == _name) {
		for(unsigned int n = 0; n < _arity; ++n) {
			if(function->outsort() != function->insort(n)) return false;
		}
		return true;
	}
	else return false;
}

/**
 * \brief Returns the unique function that has the name of the generator and the given sorts
 */
Function* OrderFuncGenerator::resolve(const vector<Sort*>& sorts) {
	for(unsigned int n = 1; n < sorts.size(); ++n) {
		if(sorts[n] != sorts[n-1]) return 0;
	}
	assert(!sorts.empty());
	map<Sort*,Function*>::const_iterator it = _overfuncs.find(sorts[0]);
	if(it == _overfuncs.end()) return disambiguate(sorts);
	else return it->second;
}

/**
 * \brief Returns the function F[A,...,A:A], where F is the name of the generator and A is the only sort 
 * \brief among the given sorts.
 */
Function* OrderFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary*) {
	Sort* funcSort = 0;
	for(vector<Sort*>::const_iterator it = sorts.begin(); it != sorts.end(); ++it) {
		if(*it) {
			if(funcSort) {
				if(funcSort != *it) return 0;
			}
			else funcSort = *it;
		}
	}

	Function* func = 0;
	if(funcSort) {
		map<Sort*,Function*>::const_iterator it = _overfuncs.find(funcSort);
		if(it != _overfuncs.end()) func = it->second;
		else {
			vector<Sort*> funcSorts(_arity+1,funcSort); 
			func = new Function(_name,funcSorts,_interpretation->get(funcSorts),0);
			_overfuncs[funcSort] = func;
		}
	}
	return func;
}

set<Sort*> OrderFuncGenerator::allsorts() const {
	set<Sort*> ss;
	return ss;
}

void OrderFuncGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for(map<Sort*,Function*>::iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		it->second->addVocabulary(vocabulary);
	}
}

void OrderFuncGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for(map<Sort*,Function*>::iterator it = _overfuncs.begin(); it != _overfuncs.end(); ++it) {
		it->second->removeVocabulary(vocabulary);
	}
}


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
		else {
			EnumeratedFuncGenerator* efg = new EnumeratedFuncGenerator(sf);
			return new Function(efg);
		}
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
		p->addVocabulary(this);
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
		f->addVocabulary(this);
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

Vocabulary* Vocabulary::_std = 0;

Vocabulary* Vocabulary::std() {
	if(!_std) {
 		_std = new Vocabulary("std");

		// Create sort interpretations
		SortTable* allnats		= new SortTable(new AllNaturalNumbers());
		SortTable* allints		= new SortTable(new AllIntegers());
		SortTable* allfloats	= new SortTable(new AllFloats());
		SortTable* allstrings	= new SortTable(new AllStrings());
		SortTable* allchars		= new SortTable(new AllChars());

		// Create sorts
		Sort* natsort		= new Sort("nat",allnats);
		Sort* intsort		= new Sort("int",allints); 
		Sort* floatsort		= new Sort("float",allfloats);
		Sort* charsort		= new Sort("char",allchars);
		Sort* stringsort	= new Sort("string",allstrings);

		// Add the sorts
		_std->addSort(natsort);
		_std->addSort(intsort);
		_std->addSort(floatsort);
		_std->addSort(charsort);
		_std->addSort(stringsort);

		// Set sort hierarchy 
		intsort->addParent(floatsort);
		natsort->addParent(intsort);
		charsort->addParent(stringsort);

		// Create predicate interpretations
		EqualInterGeneratorGenerator* eqgen = new EqualInterGeneratorGenerator();
		StrLessThanInterGeneratorGenerator* ltgen = new StrLessThanInterGeneratorGenerator();
		StrGreaterThanInterGeneratorGenerator* gtgen = new StrGreaterThanInterGeneratorGenerator();
		
		// Create predicate overloaders
		ComparisonPredGenerator* eqpgen = new ComparisonPredGenerator("=/2",eqgen);
		ComparisonPredGenerator* ltpgen = new ComparisonPredGenerator("</2",ltgen);
		ComparisonPredGenerator* gtpgen = new ComparisonPredGenerator(">/2",gtgen);
		
		// Add predicates
		_std->addPred(new Predicate(eqpgen));
		_std->addPred(new Predicate(ltpgen));
		_std->addPred(new Predicate(gtpgen));

		// Create function interpretations
		SingleFuncInterGenerator* modgen = 
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new ModInternalFuncTable())));
		SingleFuncInterGenerator* expgen = 
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new ExpInternalFuncTable())));

		vector<Sort*> twoints(2,intsort);
		vector<Sort*> twofloats(2,floatsort);
		vector<Sort*> threeints(3,intsort);
		vector<Sort*> threefloats(3,floatsort);

		SingleFuncInterGenerator* intplusgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new PlusInternalFuncTable(true))));
		SingleFuncInterGenerator* floatplusgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new PlusInternalFuncTable(false))));
		Function* intplus = new Function("+/2",threeints,intplusgen,200);
		Function* floatplus = new Function("+/2",threefloats,floatplusgen,200);

		SingleFuncInterGenerator* intminusgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new MinusInternalFuncTable(true))));
		SingleFuncInterGenerator* floatminusgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new MinusInternalFuncTable(false))));
		Function* intminus = new Function("-/2",threeints,intminusgen,200);
		Function* floatminus = new Function("-/2",threefloats,floatminusgen,200);

		SingleFuncInterGenerator* inttimesgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new TimesInternalFuncTable(true))));
		SingleFuncInterGenerator* floattimesgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new TimesInternalFuncTable(false))));
		Function* inttimes = new Function("*/2",threeints,inttimesgen,300);
		Function* floattimes = new Function("*/2",threefloats,floattimesgen,300);

		SingleFuncInterGenerator* intdivgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new DivInternalFuncTable(true))));
		SingleFuncInterGenerator* floatdivgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new DivInternalFuncTable(false))));
		Function* intdiv = new Function("//2",threeints,intdivgen,300);
		Function* floatdiv = new Function("//2",threefloats,floatdivgen,300);

		SingleFuncInterGenerator* intabsgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new AbsInternalFuncTable(true))));
		SingleFuncInterGenerator* floatabsgen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new AbsInternalFuncTable(false))));
		Function* intabs = new Function("abs/1",twoints,intabsgen,0);
		Function* floatabs = new Function("abs/1",twofloats,floatabsgen,0);

		SingleFuncInterGenerator* intumingen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new UminInternalFuncTable(true))));
		SingleFuncInterGenerator* floatumingen =
			new SingleFuncInterGenerator(new FuncInter(new FuncTable(new UminInternalFuncTable(false))));
		Function* intumin = new Function("-/1",twoints,intumingen,500);
		Function* floatumin = new Function("-/1",twofloats,floatumingen,500);

		MinInterGeneratorGenerator* minigengen = new MinInterGeneratorGenerator();
		MaxInterGeneratorGenerator* maxigengen = new MaxInterGeneratorGenerator();
		SuccInterGeneratorGenerator* succigengen = new SuccInterGeneratorGenerator();
		InvSuccInterGeneratorGenerator* predigengen = new InvSuccInterGeneratorGenerator();
		
		// Create function overloaders
		IntFloatFuncGenerator* plusgen = new IntFloatFuncGenerator(intplus,floatplus);
		IntFloatFuncGenerator* minusgen = new IntFloatFuncGenerator(intminus,floatminus);
		IntFloatFuncGenerator* timesgen = new IntFloatFuncGenerator(inttimes,floattimes);
		IntFloatFuncGenerator* divgen = new IntFloatFuncGenerator(intdiv,floatdiv);
		IntFloatFuncGenerator* absgen = new IntFloatFuncGenerator(intabs,floatabs);
		IntFloatFuncGenerator* umingen = new IntFloatFuncGenerator(intumin,floatumin);
		OrderFuncGenerator* mingen = new OrderFuncGenerator("MIN/0",0,minigengen);
		OrderFuncGenerator* maxgen = new OrderFuncGenerator("MAX/0",0,maxigengen);
		OrderFuncGenerator* succgen = new OrderFuncGenerator("SUCC/1",1,succigengen);
		OrderFuncGenerator* predgen = new OrderFuncGenerator("PRED/1",1,predigengen);
		
		// Add functions
		Function* modfunc = new Function(string("%/2"),threeints,modgen,100); modfunc->partial(true);
		Function* expfunc = new Function(string("^/2"),threefloats,expgen,400);
		_std->addFunc(modfunc);
		_std->addFunc(expfunc);
		_std->addFunc(new Function(plusgen));
		_std->addFunc(new Function(minusgen));
		_std->addFunc(new Function(timesgen));
		_std->addFunc(new Function(divgen));
		_std->addFunc(new Function(absgen));
		_std->addFunc(new Function(umingen));
		_std->addFunc(new Function(mingen));
		_std->addFunc(new Function(maxgen));
		_std->addFunc(new Function(succgen));
		_std->addFunc(new Function(predgen));
	}
	return _std;
}

inline const string& Vocabulary::name() const {
	return _name;
}

inline const ParseInfo& Vocabulary::pi() const {
	return _pi;
}

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
	if(typeid(*s) == typeid(Predicate)) {
		return contains(dynamic_cast<Predicate*>(s));
	}
	else {
		assert(typeid(*s) == typeid(Function));
		return contains(dynamic_cast<Function*>(s));
	}
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

set<Predicate*> Vocabulary::pred_no_arity(const string& name) const {
	set<Predicate*> vp;
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vp.insert(it->second);
	}
	return vp;
}

set<Function*> Vocabulary::func_no_arity(const string& name) const {
	set<Function*> vf;
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		string nm = it->second->name();
		if(nm.substr(0,nm.find('/')) == name) vf.insert(it->second);
	}
	return vf;
}

ostream& Vocabulary::putname(ostream& output) const {
	if(_namespace) {
		// TODO uncomment this: _namespace->putname(output);
		output << "::";
	}
	output << _name;
	return output;
}

ostream& Vocabulary::put(ostream& output, unsigned int tabs) const {
	printtabs(output,tabs);
	output << "Vocabulary " << _name << ":\n";
	++tabs; printtabs(output,tabs);
	output << "Sorts:\n";
	++tabs;
	for(map<string,set<Sort*> >::const_iterator it = _name2sort.begin(); it != _name2sort.end(); ++it) {
		for(set<Sort*>::const_iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) {
			printtabs(output,tabs);
			output << *(*jt) << '\n';
		}
	}
	--tabs; printtabs(output,tabs);
	output << "Predicates:\n";
	++tabs;
	for(map<string,Predicate*>::const_iterator it = _name2pred.begin(); it != _name2pred.end(); ++it) {
		printtabs(output,tabs);
		output << *(it->second) << '\n';
	}
	--tabs; printtabs(output,tabs);
	output << "Functions:\n";
	++tabs;
	for(map<string,Function*>::const_iterator it = _name2func.begin(); it != _name2func.end(); ++it) {
		printtabs(output,tabs);
		output << *(it->second) << '\n';
	}
	return output;
}

string Vocabulary::to_string(unsigned int tabs) const {
	stringstream ss;
	put(ss,tabs);
	return ss.str();
}

ostream& operator<< (ostream& output,const Vocabulary& voc) {
	return voc.put(output);
}
