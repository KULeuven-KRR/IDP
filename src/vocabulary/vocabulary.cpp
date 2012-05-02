/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

using namespace std;

/***********
 *	Sorts
 **********/

/**
 * Destructor for sorts. 
 * Deletes the built-in interpretation and removes the sort from the sort hierarchy.
 */
Sort::~Sort() {
	for (auto it = _parents.cbegin(); it != _parents.cend(); ++it) {
		(*it)->removeChild(this);
	}
	for (auto it = _children.cbegin(); it != _children.cend(); ++it) {
		(*it)->removeParent(this);
	}
	if (_interpretation) {
		delete (_interpretation);
	}
}

void Sort::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if (_vocabularies.empty()) {
		delete (this);
	}
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
void Sort::generatePred(SortTable* inter) {
	string predname(_name + "/1");
	vector<Sort*> predsorts(1, this);
	if (inter != NULL) {
		Universe univ(vector<SortTable*>(1, inter));
		PredTable* pt = new PredTable(new FullInternalPredTable(), univ);
		PredInter* pinter = new PredInter(pt, true);
		PredInterGenerator* pig = new SinglePredInterGenerator(pinter);
		_pred = new Predicate(predname, predsorts, pig, false);
	} else {
		_pred = new Predicate(predname, predsorts, _pi);
	}
}

/**
 * Only to be used from unionsort!
 */
Sort::Sort()
		: _name(""), _pi(), _interpretation(NULL) {
}

/**
 * Create an internal sort
 */
Sort::Sort(const string& name, SortTable* inter)
		: _name(name), _pi(), _interpretation(inter) {
	generatePred(inter);
}

/**
 * Create a user-declared sort
 */
Sort::Sort(const string& name, const ParseInfo& pi, SortTable* inter)
		: _name(name), _pi(pi), _interpretation(inter) {
	generatePred(inter);
}

/**
 * Add p as a parent
 */
void Sort::addParent(Sort* p) {
	pair<set<Sort*>::iterator, bool> changed = _parents.insert(p);
	if (changed.second) {
		p->addChild(this);
	}
}

void Sort::addChild(Sort* c) {
	pair<set<Sort*>::iterator, bool> changed = _children.insert(c);
	if (changed.second) {
		c->addParent(this);
	}
}

const string& Sort::name() const {
	return _name;
}

const ParseInfo& Sort::pi() const {
	return _pi;
}

Predicate* Sort::pred() const {
	return _pred;
}

const std::set<Sort*>& Sort::parents() const {
	return _parents;
}

const std::set<Sort*>& Sort::children() const {
	return _children;
}

/**
 * Compute all ancestors of the sort in the sort hierarchy
 *
 * PARAMETERS
 *		- vocabulary:	if this is not a null-pointer, the set of ancestors is restricted to the ancestors in vocabulary
 */
set<Sort*> Sort::ancestors(const Vocabulary* vocabulary) const {
	set<Sort*> ancest;
	for (auto it = _parents.cbegin(); it != _parents.cend(); ++it) {
		if ((not vocabulary) || vocabulary->contains(*it)) {
			ancest.insert(*it);
		}
		set<Sort*> temp = (*it)->ancestors(vocabulary);
		ancest.insert(temp.cbegin(), temp.cend());
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
	for (auto it = _children.cbegin(); it != _children.cend(); ++it) {
		if ((not vocabulary) || vocabulary->contains(*it)) {
			descend.insert(*it);
		}
		set<Sort*> temp = (*it)->descendents(vocabulary);
		descend.insert(temp.cbegin(), temp.cend());
	}
	return descend;
}

bool Sort::builtin() const {
	return (_interpretation != 0);
}

SortTable* Sort::interpretation() const {
	return _interpretation;
}

std::set<const Vocabulary*>::const_iterator Sort::firstVocabulary() const {
	return _vocabularies.cbegin();
}
std::set<const Vocabulary*>::const_iterator Sort::lastVocabulary() const {
	return _vocabularies.cend();
}

ostream& Sort::put(ostream& output) const {
	if (getOption(BoolType::LONGNAMES)) {
		for (auto it = _vocabularies.cbegin(); it != _vocabularies.cend(); ++it) {
			if ((*it)->sort(_name) != NULL) {
				(*it)->putName(output);
				output << "::";
				break;
			}
		}
	}
	output << _name;
	return output;
}

UnionSort::UnionSort(const std::vector<Sort*>& sorts)
		: sorts(sorts) {
	stringstream ss;
	for (auto i = sorts.cbegin(); i < sorts.cend(); ++i) {
		ss << (*i)->name() << "-";
	}
	setPred(new Predicate(ss.str() + "/1", { this }, pi()));
}

bool UnionSort::builtin() const {
	return false;
}

namespace SortUtils {
/**
 *	\brief	Return the unique nearest common ancestor of two sorts.
 *
 *	\param s1			the first sort
 *	\param s2			the second sort
 *	\param vocabulary	if not NULL, search for the nearest common ancestor in the projection 
 *						of the sort hiearchy on this vocabulary
 *
 *	\return	The unique nearest common ancestor if it exists, a null-pointer otherwise.
 */
Sort* resolve(Sort* s1, Sort* s2, const Vocabulary* voc) {
	if ((s1 == NULL) || s2 == NULL) {
		return NULL;
	}
	auto ss1 = s1->ancestors(voc);
	ss1.insert(s1);
	auto ss2 = s2->ancestors(voc);
	ss2.insert(s2);
	set<Sort*> ss;
	for (auto it = ss1.cbegin(); it != ss1.cend(); ++it) {
		if (ss2.find(*it) != ss2.cend()) {
			ss.insert(*it);
		}
	}
	auto vs = vector<Sort*>(ss.cbegin(), ss.cend());
	if (vs.empty()) {
		return NULL;
	} else if (vs.size() == 1) {
		return vs[0];
	} else {
		for (size_t n = 0; n < vs.size(); ++n) {
			auto ds = vs[n]->ancestors(voc);
			for (auto it = ds.cbegin(); it != ds.cend(); ++it) {
				ss.erase(*it);
			}
		}
		vs = vector<Sort*>(ss.cbegin(), ss.cend());
		if (vs.size() == 1) {
			return vs[0];
		} else {
			return NULL;
		}
	}
}

bool isSubsort(Sort* a, Sort* b, const Vocabulary* voc) {
	return resolve(a, b, voc) == b;
}

}

/***************
 *	Variables
 **************/

Variable::~Variable() {
}

Variable::Variable(const std::string& name, Sort* sort, const ParseInfo& pi)
		: _name(name), _sort(sort), _pi(pi) {
}

Variable::Variable(Sort* s)
		: _sort(s) {
	_name = "_var_" + s->name() + "_" + convertToString(getGlobal()->getNewID());
}

void Variable::sort(Sort* s) {
	_sort = s;
}

const string& Variable::name() const {
	return _name;
}

Sort* Variable::sort() const {
	return _sort;
}

const ParseInfo& Variable::pi() const {
	return _pi;
}

ostream& Variable::put(ostream& output) const {
	output << _name;
	if (_sort) {
		output << '[';
		_sort->put(output);
		output << ']';
	}
	return output;
}

ostream& operator<<(ostream& output, const Variable& var) {
	return var.put(output);
}

vector<Variable*> VarUtils::makeNewVariables(const vector<Sort*>& sorts) {
	vector<Variable*> vars;
	for (auto it = sorts.cbegin(); it != sorts.cend(); ++it) {
		vars.push_back(new Variable(*it));
	}
	return vars;
}

/******************************
 *	Predicates and functions
 *****************************/

PFSymbol::~PFSymbol() {
	for (auto it = _derivedsymbols.cbegin(); it != _derivedsymbols.cend(); ++it) {
		delete (it->second);
	}
}

PFSymbol::PFSymbol(const string& name, size_t nrsorts, bool infix)
		: _name(name), _sorts(nrsorts, 0), _infix(infix) {
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, bool infix)
		: _name(name), _sorts(sorts), _infix(infix) {
}

PFSymbol::PFSymbol(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi, bool infix)
		: _name(name), _pi(pi), _sorts(sorts), _infix(infix) {
}

const string& PFSymbol::name() const {
	return _name;
}

const ParseInfo& PFSymbol::pi() const {
	return _pi;
}

size_t PFSymbol::nrSorts() const {
	return _sorts.size();
}

Sort* PFSymbol::sort(size_t n) const {
	return _sorts[n];
}

const vector<Sort*>& PFSymbol::sorts() const {
	return _sorts;
}

bool PFSymbol::infix() const {
	return _infix;
}

bool PFSymbol::hasVocabularies() const {
	return not _vocabularies.empty();
}

Predicate* PFSymbol::derivedSymbol(SymbolType type) {
	Assert(type != ST_NONE);
	auto it = _derivedsymbols.find(type);
	if (it == _derivedsymbols.cend()) {
		Predicate* derp = new Predicate(_name, _sorts, _pi, _infix);
		derp->type(type, this);
		_derivedsymbols[type] = derp;
		return derp;
	} else {
		return it->second;
	}
}

vector<unsigned int> PFSymbol::argumentNrs(const Sort* soort) const {
	vector<unsigned int> result;
	for (unsigned int i = 0; i < nrSorts(); ++i) {
		if (sort(i) == soort) {
			result.push_back(i);
		}
	}
	return result;
}

ostream& operator<<(ostream& output, const PFSymbol& s) {
	return s.put(output);
}

set<Sort*> Predicate::allsorts() const {
	set<Sort*> ss;
	ss.insert(_sorts.cbegin(), _sorts.cend());
	if (_overpredgenerator) {
		set<Sort*> os = _overpredgenerator->allsorts();
		ss.insert(os.cbegin(), os.cend());
	}
	ss.erase(0);
	return ss;
}

Predicate::~Predicate() {
	if (_interpretation) {
		delete (_interpretation);
	}
	if (_overpredgenerator) {
		delete (_overpredgenerator);
	}
}

bool Predicate::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if (overloaded()) {
		_overpredgenerator->removeVocabulary(vocabulary);
	}
	if (_vocabularies.empty()) {
		delete (this);
		return true;
	}
	return false;
}

void Predicate::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
	if (overloaded()) {
		_overpredgenerator->addVocabulary(vocabulary);
	}
}

Predicate::Predicate(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, bool infix)
		: PFSymbol(name, sorts, pi, infix), _type(ST_NONE), _parent(0), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const std::string& name, const std::vector<Sort*>& sorts, bool infix)
		: PFSymbol(name, sorts, infix), _type(ST_NONE), _parent(0), _interpretation(0), _overpredgenerator(0) {
}

Predicate::Predicate(const vector<Sort*>& sorts)
		: PFSymbol("", sorts, ParseInfo()), _type(ST_NONE), _parent(0), _interpretation(0), _overpredgenerator(0) {
	_name = "_internal_predicate_" + convertToString(getGlobal()->getNewID()) + "/" + convertToString(sorts.size());
}

Predicate::Predicate(const std::string& name, const std::vector<Sort*>& sorts, PredInterGenerator* inter, bool infix)
		: PFSymbol(name, sorts, infix), _type(ST_NONE), _parent(0), _interpretation(inter), _overpredgenerator(0) {
}

Predicate::Predicate(PredGenerator* generator)
		: PFSymbol(generator->name(), generator->arity(), generator->infix()), _type(ST_NONE), _parent(0), _interpretation(0), _overpredgenerator(generator) {
}

unsigned int Predicate::arity() const {
	return _sorts.size();
}

bool Predicate::builtin() const {
	return _interpretation != 0;
}

bool Predicate::overloaded() const {
	return (_overpredgenerator != 0);
}

void Predicate::type(SymbolType type, PFSymbol* parent) {
	_type = type;
	_parent = parent;
}

/**
 * \brief Returns the interpretation of a built-in predicate
 *
 * PARAMETERS
 *		 - structure: for some predicates, e.g. =/2 over a type A, the interpretation of A is 
 *		 needed to generate the interpretation for =/2. The structure contains the interpretation of the 
 *		 relevant sorts.
 */
PredInter* Predicate::interpretation(const AbstractStructure* structure) const {
	if (_interpretation) {
		return _interpretation->get(structure);
	} else {
		return 0;
	}
}

/**
 * \brief Returns true iff the predicate is equal to, or overloads a given predicate
 *
 * PARAMETERS
 *		- predicate: the given predicate
 */
bool Predicate::contains(const Predicate* predicate) const {
	if (this == predicate) {
		return true;
	} else if (_overpredgenerator && _overpredgenerator->contains(predicate)) {
		return true;
	} else {
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
Predicate* Predicate::resolve(const vector<Sort*>& sorts) {
	if (overloaded()) {
		return _overpredgenerator->resolve(sorts);
	} else if (_sorts == sorts) {
		return this;
	} else {
		return NULL;
	}
}

/**
 * \brief Returns a predicate that is overloaded by the predicate and which has sorts that resolve with the given sorts.
 * Which predicate is returned may depend on the overpredgenerator. Returns a null-pointer if no
 * suitable predicate is found.
 *
 * PARAMETERS
 *		- sorts:		the given sorts
 *		- vocabulary:	the vocabulary used for resolving the sorts. Defaults to 0.
 */
Predicate* Predicate::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	if (overloaded()) {
		return _overpredgenerator->disambiguate(sorts, vocabulary);
	} else {
		for (size_t n = 0; n < _sorts.size(); ++n) {
			if (_sorts[n] && not SortUtils::resolve(sorts[n], _sorts[n], vocabulary)) {
				return NULL;
			}
		}
		return this;
	}
}

set<Predicate*> Predicate::nonbuiltins() {
	if (_overpredgenerator) {
		return _overpredgenerator->nonbuiltins();
	} else {
		set<Predicate*> sp;
		if (not _interpretation) {
			sp.insert(this);
		}
		return sp;
	}
}

ostream& Predicate::put(ostream& output) const {
	if (getOption(BoolType::LONGNAMES)) {
		for (auto it = _vocabularies.cbegin(); it != _vocabularies.cend(); ++it) {
			if (not (*it)->pred(_name)->overloaded()) {
				(*it)->putName(output);
				output << "::";
				break;
			}
		}
	}
	output << _name.substr(0, _name.rfind('/'));
	if (getOption(BoolType::LONGNAMES) && not overloaded()) {
		if (nrSorts() > 0) {
			output << '[';
			sort(0)->put(output);
			for (size_t n = 1; n < nrSorts(); ++n) {
				output << ',';
				sort(n)->put(output);
			}
			output << ']';
		}
	}
	switch (_type) {
	case ST_NONE:
		break;
	case ST_CT:
		output << "<ct>";
		break;
	case ST_CF:
		output << "<cf>";
		break;
	case ST_PT:
		output << "<pt>";
		break;
	case ST_PF:
		output << "<pf>";
		break;
	}
	return output;
}

ostream& operator<<(ostream& output, const Predicate& p) {
	return p.put(output);
}

PredGenerator::PredGenerator(const string& name, unsigned int arity, bool infix)
		: _name(name), _arity(arity), _infix(infix) {
}

const string& PredGenerator::name() const {
	return _name;
}

unsigned int PredGenerator::arity() const {
	return _arity;
}

bool PredGenerator::infix() const {
	return _infix;
}

EnumeratedPredGenerator::EnumeratedPredGenerator(const set<Predicate*>& overpreds)
		: PredGenerator((*(overpreds.cbegin()))->name(), (*(overpreds.cbegin()))->arity(), (*(overpreds.cbegin()))->infix()), _overpreds(overpreds) {
}

bool EnumeratedPredGenerator::contains(const Predicate* predicate) const {
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		if ((*it)->contains(predicate)) {
			return true;
		}
	}
	return false;
}

/**
 * \brief Returns the unique predicate that is contained in the generator and that has the given sorts.
 * \brief Returns a null-pointer if such a predicate does not exist or is not unique
 */
Predicate* EnumeratedPredGenerator::resolve(const vector<Sort*>& sorts) {
	Predicate* candidate = 0;
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		Predicate* newcandidate = (*it)->resolve(sorts);
		if (candidate && candidate != newcandidate) {
			return 0;
		} else {
			candidate = newcandidate;
		}
	}
	return candidate;
}

/**
 * \brief Returns the unique predicate that is contained in the generator and which sorts resolve with the given sorts.
 * \brief Returns a null-pointer if such a predicate does not exist or is not unique
 */
Predicate* EnumeratedPredGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	Predicate* candidate = NULL;
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		Predicate* newcandidate = (*it)->disambiguate(sorts, vocabulary);
		if (candidate && candidate != newcandidate) {
			return NULL;
		} else {
			candidate = newcandidate;
		}
	}
	return candidate;
}

set<Sort*> EnumeratedPredGenerator::allsorts() const {
	set<Sort*> ss;
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		set<Sort*> os = (*it)->allsorts();
		ss.insert(os.cbegin(), os.cend());
	}
	ss.erase(0);
	return ss;
}

void EnumeratedPredGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		(*it)->addVocabulary(vocabulary);
	}
}

void EnumeratedPredGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		(*it)->removeVocabulary(vocabulary);
	}
}

set<Predicate*> EnumeratedPredGenerator::nonbuiltins() const {
	set<Predicate*> sp;
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		set<Predicate*> temp = (*it)->nonbuiltins();
		sp.insert(temp.cbegin(), temp.cend());
	}
	return sp;
}

ComparisonPredGenerator::ComparisonPredGenerator(const string& name, PredInterGeneratorGenerator* inter)
		: PredGenerator(name, 2, true), _interpretation(inter) {
}

ComparisonPredGenerator::~ComparisonPredGenerator() {
	delete (_interpretation);
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		if (not it->second->hasVocabularies()) {
			delete (it->second);
		}
	}
}

/**
 * \brief Returns true iff predicate has the same name as the generator, and both sorts of the predicate are equal
 */
bool ComparisonPredGenerator::contains(const Predicate* predicate) const {
	if (predicate->name() == _name) {
		Assert(predicate->arity() == 2);
		return predicate->sort(0) == predicate->sort(1);
	} else {
		return false;
	}
}

/**
 * \brief Returns the unique predicate that has the name of the generator and the given sorts
 */
Predicate* ComparisonPredGenerator::resolve(const vector<Sort*>& sorts) {
	if (sorts.size() == 2 && sorts[0] == sorts[1]) {
		map<Sort*, Predicate*>::const_iterator it = _overpreds.find(sorts[0]);
		if (it == _overpreds.cend()) {
			return disambiguate(sorts);
		} else {
			return it->second;
		}
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
	Sort* predSort = NULL;
	bool sortsContainsZero = false;
	for (auto it = sorts.cbegin(); it != sorts.cend(); ++it) {
		if ((*it) == NULL) {
			sortsContainsZero = true;
			continue;
		}
		if (predSort != NULL) {
			predSort = SortUtils::resolve(predSort, *it, vocabulary);
			if (predSort == NULL) {
				return NULL;
			}
		} else {
			predSort = *it;
		}
	}

	Predicate* pred = NULL;
	if (predSort && (not sortsContainsZero || not predSort->ancestors(vocabulary).empty())) {
		auto it = _overpreds.find(predSort);
		if (it != _overpreds.cend()) {
			pred = it->second;
		} else {
			vector<Sort*> predSorts(2, predSort);
			pred = new Predicate(_name, predSorts, _interpretation->get(predSorts), true);
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
	for (auto it = _overpreds.cbegin(); it != _overpreds.cend(); ++it) {
		it->second->addVocabulary(vocabulary);
	}
}

void ComparisonPredGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for (auto it = _overpreds.begin(); it != _overpreds.end();) {
		map<Sort*, Predicate*>::iterator jt = it;
		++it;
		if (jt->second->removeVocabulary(vocabulary)) {
			_overpreds.erase(jt);
		}
	}
}

set<Predicate*> ComparisonPredGenerator::nonbuiltins() const {
	set<Predicate*> sp;
	return sp;
}

namespace PredUtils {

Predicate* overload(Predicate* p1, Predicate* p2) {
	Assert(p1->name() == p2->name());
	if (p1 == p2) {
		return p1;
	}
	set<Predicate*> sp;
	sp.insert(p1);
	sp.insert(p2);
	return overload(sp);
}

Predicate* overload(const set<Predicate*>& sp) {
	if (sp.empty()) {
		return NULL;
	} else if (sp.size() == 1) {
		return *(sp.cbegin());
	} else {
		auto epg = new EnumeratedPredGenerator(sp);
		return new Predicate(epg);
	}
}

}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi, unsigned int binding)
		: PFSymbol(name, is, pi), _partial(false), _insorts(is), _outsort(os), _interpretation(NULL), _overfuncgenerator(NULL), _binding(binding) {
	_sorts.push_back(os);
}

Function::Function(const std::vector<Sort*>& is, Sort* os, const ParseInfo& pi, unsigned int binding)
		: PFSymbol("", is, pi), _partial(false), _insorts(is), _outsort(os), _interpretation(NULL), _overfuncgenerator(NULL), _binding(binding) {
	_sorts.push_back(os);
	_name = "_internal_function_" + convertToString(getGlobal()->getNewID()) + "/" + convertToString(is.size()+1);
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, const ParseInfo& pi, unsigned int binding)
		: PFSymbol(name, sorts, pi), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(NULL), _overfuncgenerator(NULL),
			_binding(binding) {
	_insorts.pop_back();
}

Function::Function(const std::string& name, const std::vector<Sort*>& is, Sort* os, unsigned int binding)
		: PFSymbol(name, is), _partial(false), _insorts(is), _outsort(os), _interpretation(NULL), _overfuncgenerator(NULL), _binding(binding) {
	_sorts.push_back(os);
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, unsigned int binding)
		: PFSymbol(name, sorts), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(NULL), _overfuncgenerator(NULL), _binding(binding) {
	_insorts.pop_back();
}

Function::Function(const std::string& name, const std::vector<Sort*>& sorts, FuncInterGenerator* inter, unsigned int binding)
		: PFSymbol(name, sorts, binding != 0), _partial(false), _insorts(sorts), _outsort(sorts.back()), _interpretation(inter), _overfuncgenerator(NULL),
			_binding(binding) {
	_insorts.pop_back();
}

Function::Function(FuncGenerator* generator)
		: PFSymbol(generator->name(), generator->arity() + 1, generator->binding() != 0), _partial(true), _insorts(generator->arity(), NULL), _outsort(NULL),
			_interpretation(NULL), _overfuncgenerator(generator) {
}

Function::~Function() {
	if (_interpretation) {
		delete (_interpretation);
	}
	if (_overfuncgenerator) {
		delete (_overfuncgenerator);
	}
}

void Function::partial(bool b) {
	_partial = b;
}

bool Function::removeVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.erase(vocabulary);
	if (overloaded()) {
		_overfuncgenerator->removeVocabulary(vocabulary);
	}
	if (_vocabularies.empty()) {
		delete (this);
		return true;
	}
	return false;
}

void Function::addVocabulary(const Vocabulary* vocabulary) {
	_vocabularies.insert(vocabulary);
	if (overloaded()) {
		_overfuncgenerator->addVocabulary(vocabulary);
	}
}

set<Sort*> Function::allsorts() const {
	set<Sort*> ss;
	ss.insert(_sorts.cbegin(), _sorts.cend());
	if (_overfuncgenerator != NULL) {
		set<Sort*> os = _overfuncgenerator->allsorts();
		ss.insert(os.cbegin(), os.cend());
	}
	ss.erase(NULL);
	return ss;
}

const vector<Sort*>& Function::insorts() const {
	return _insorts;
}

unsigned int Function::arity() const {
	return _insorts.size();
}

Sort* Function::insort(unsigned int n) const {
	return _insorts[n];
}

Sort* Function::outsort() const {
	return _outsort;
}

bool Function::partial() const {
	return _partial;
}

bool Function::builtin() const {
	return _interpretation != NULL || Vocabulary::std()->contains(this);
}

bool Function::overloaded() const {
	return (_overfuncgenerator != 0);
}

unsigned int Function::binding() const {
	return _binding;
}

/**
 * \brief Returns the interpretation of a built-in function
 *
 * PARAMETERS
 *		- structure: for some functions, e.g. //2 over a type A, the interpretation of A is 
 *		needed to generate the interpretation for //2. The structure contains the interpretation of 
 *		the relevant sorts.
 */
FuncInter* Function::interpretation(const AbstractStructure* structure) const {
	if (_interpretation) {
		return _interpretation->get(structure);
	} else {
		return NULL;
	}
}

/**
 * \brief Returns true iff the function is equal to, or overloads a given function
 *
 * PARAMETERS
 *		- function: the given function
 */
bool Function::contains(const Function* function) const {
	if (this == function) {
		return true;
	} else if (_overfuncgenerator && _overfuncgenerator->contains(function)) {
		return true;
	} else {
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
 *		- Otherwise, the unique function that is overloaded by the function and matches the 
 *		  given sorts.
 */
Function* Function::resolve(const vector<Sort*>& sorts) {
	if (overloaded()) {
		return _overfuncgenerator->resolve(sorts);
	} else if (_sorts == sorts) {
		return this;
	} else {
		return NULL;
	}
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
Function* Function::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	if (overloaded()) {
		return _overfuncgenerator->disambiguate(sorts, vocabulary);
	} else {
		for (size_t n = 0; n < _sorts.size(); ++n) {
			if (sorts[n] != NULL && not SortUtils::resolve(sorts[n], _sorts[n], vocabulary)) {
				return NULL;
			}
		}
		return this;
	}
}

set<Function*> Function::nonbuiltins() {
	if (_overfuncgenerator) {
		return _overfuncgenerator->nonbuiltins();
	} else {
		set<Function*> sf;
		if (not _interpretation) {
			sf.insert(this);
		}
		return sf;
	}
}

ostream& Function::put(ostream& output) const {
	if (getOption(BoolType::LONGNAMES)) {
		for (auto it = _vocabularies.cbegin(); it != _vocabularies.cend(); ++it) {
			if (not (*it)->func(_name)->overloaded()) {
				(*it)->putName(output);
				output << "::";
				break;
			}
		}
	}
	output << _name.substr(0, _name.rfind('/'));
	if (getOption(BoolType::LONGNAMES) && not overloaded()) {
		output << '[';
		if (_insorts.empty()) {
			_insorts[0]->put(output);
			for (size_t n = 1; n < _insorts.size(); ++n) {
				output << ',';
				_insorts[n]->put(output);
			}
		}
		output << " : ";
		_outsort->put(output);
		output << ']';
	}
	return output;
}

ostream& operator<<(ostream& output, const Function& f) {
	return f.put(output);
}

const string& FuncGenerator::name() const {
	return _name;
}

unsigned int FuncGenerator::arity() const {
	return _arity;
}

unsigned int FuncGenerator::binding() const {
	return _binding;
}

EnumeratedFuncGenerator::EnumeratedFuncGenerator(const set<Function*>& overfuncs)
		: FuncGenerator((*(overfuncs.cbegin()))->name(), (*(overfuncs.cbegin()))->arity(), (*(overfuncs.cbegin()))->binding()), _overfuncs(overfuncs) {
}

bool EnumeratedFuncGenerator::contains(const Function* function) const {
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		if ((*it)->contains(function)) {
			return true;
		}
	}
	return false;
}

/**
 * \brief Returns the unique function that is contained in the generator and that has the given sorts.
 * \brief Returns a null-pointer if such a function does not exist or is not unique
 */
Function* EnumeratedFuncGenerator::resolve(const vector<Sort*>& sorts) {
	Function* candidate = NULL;
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		Function* newcandidate = (*it)->resolve(sorts);
		if (candidate && candidate != newcandidate) {
			return NULL;
		} else {
			candidate = newcandidate;
		}
	}
	return candidate;
}

/**
 * \brief Returns the unique function that is contained in the generator and which sorts resolve with the given sorts.
 * \brief Returns a null-pointer if such a function does not exist or is not unique
 */
Function* EnumeratedFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	Function* candidate = NULL;
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		Function* newcandidate = (*it)->disambiguate(sorts, vocabulary);
		if (candidate != NULL && candidate != newcandidate) {
			return NULL;
		} else {
			candidate = newcandidate;
		}
	}
	return candidate;
}

set<Sort*> EnumeratedFuncGenerator::allsorts() const {
	set<Sort*> ss;
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		set<Sort*> os = (*it)->allsorts();
		ss.insert(os.cbegin(), os.cend());
	}
	ss.erase(0);
	return ss;
}

void EnumeratedFuncGenerator::addVocabulary(const Vocabulary* vocabulary) {
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		(*it)->addVocabulary(vocabulary);
	}
}

void EnumeratedFuncGenerator::removeVocabulary(const Vocabulary* vocabulary) {
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		(*it)->removeVocabulary(vocabulary);
	}
}

set<Function*> EnumeratedFuncGenerator::nonbuiltins() const {
	set<Function*> sf;
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		set<Function*> temp = (*it)->nonbuiltins();
		sf.insert(temp.cbegin(), temp.cend());
	}
	return sf;
}

IntFloatFuncGenerator::IntFloatFuncGenerator(Function* intfunc, Function* floatfunc)
		: FuncGenerator(intfunc->name(), intfunc->arity(), intfunc->binding()), _intfunction(intfunc), _floatfunction(floatfunc) {
}

bool IntFloatFuncGenerator::contains(const Function* function) const {
	return (function == _intfunction || function == _floatfunction);
}

/**
 * Returns the integer function if the vector of sorts only contains int, 
 * the float function if it only contains float, and a null-pointer otherwise.
 */
Function* IntFloatFuncGenerator::resolve(const vector<Sort*>& sorts) {
	Assert(sorts.size() == 2 || sorts.size() == 3);
	if (sorts[0] == sorts[1] && (sorts.size() == 2 || sorts[1] == sorts[2])) {
		if (sorts[0] == VocabularyUtils::intsort()) {
			return _intfunction;
		} else if (sorts[0] == VocabularyUtils::floatsort()) {
			return _floatfunction;
		} else {
			return NULL;
		}
	}
	return NULL;
}

/**
 * Returns a null-pointer if more than one of the sorts is a null-pointer, 
 * or one of the sorts that is not a null-pointer, is not a subsort of float.
 * Otherwise, return the integer function if all sorts are a subsort of int, 
 * and the float function if at least one sort is not a subsort of _int.
 */
Function* IntFloatFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary* vocabulary) {
	size_t zerocounter = 0;
	bool isfloatbutnotint = false;
	auto intsort = VocabularyUtils::intsort();
	auto floatsort = VocabularyUtils::floatsort();
	for (auto it = sorts.cbegin(); it != sorts.cend(); ++it) {
		if (*it == NULL) {
			if (++zerocounter > 1) {
				return NULL;
			}
		} else if (not SortUtils::isSubsort(*it, floatsort, vocabulary)) {
			return NULL;
		} else {
			Assert(SortUtils::isSubsort(*it, floatsort, vocabulary));
			if (not SortUtils::isSubsort(*it, intsort, vocabulary)) {
				isfloatbutnotint = true;
			}
		}
	}
	return (isfloatbutnotint ? _floatfunction : _intfunction);
}

/**
 * \brief Returns sorts int and float
 */
set<Sort*> IntFloatFuncGenerator::allsorts() const {
	set<Sort*> ss;
	ss.insert(VocabularyUtils::intsort());
	ss.insert(VocabularyUtils::floatsort());
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

set<Function*> IntFloatFuncGenerator::nonbuiltins() const {
	set<Function*> sf;
	return sf;
}

OrderFuncGenerator::OrderFuncGenerator(const string& name, unsigned int arity, FuncInterGeneratorGenerator* inter)
		: FuncGenerator(name, arity, 0), _interpretation(inter) {
}

OrderFuncGenerator::~OrderFuncGenerator() {
	delete (_interpretation);
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		if (not it->second->hasVocabularies()) {
			delete (it->second);
		}
	}
}

/**
 * \brief Returns true iff function has the same name as the generator, and all sorts of function are equal.
 */
bool OrderFuncGenerator::contains(const Function* function) const {
	if (function->name() == _name) {
		for (size_t n = 0; n < _arity; ++n) {
			if (function->outsort() != function->insort(n)) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Returns the unique function that has the name of the generator and the given sorts
 */
Function* OrderFuncGenerator::resolve(const vector<Sort*>& sorts) {
	for (size_t n = 1; n < sorts.size(); ++n) {
		if (sorts[n] != sorts[n - 1]) {
			return NULL;
		}
	}Assert(not sorts.empty());
	map<Sort*, Function*>::const_iterator it = _overfuncs.find(sorts[0]);
	if (it == _overfuncs.cend()) {
		return disambiguate(sorts);
	} else {
		return it->second;
	}
}

/**
 * \brief Returns the function F[A,...,A:A], where F is the name of the generator and A is the only sort 
 * \brief among the given sorts.
 */
Function* OrderFuncGenerator::disambiguate(const vector<Sort*>& sorts, const Vocabulary*) {
	Sort* funcSort = NULL;
	for (auto it = sorts.cbegin(); it != sorts.cend(); ++it) {
		if (*it != NULL) {
			if (funcSort != NULL) {
				if (funcSort != *it) {
					return NULL;
				}
			} else {
				funcSort = *it;
			}
		}
	}

	Function* func = NULL;
	if (funcSort != NULL) {
		map<Sort*, Function*>::const_iterator it = _overfuncs.find(funcSort);
		if (it != _overfuncs.cend()) {
			func = it->second;
		} else {
			vector<Sort*> funcSorts(_arity + 1, funcSort);
			func = new Function(_name, funcSorts, _interpretation->get(funcSorts), 0);
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
	for (auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
		it->second->addVocabulary(vocabulary);
	}
}

void OrderFuncGenerator::removeVocabulary(const Vocabulary*) {
	// TODO: check this...
	//for(auto it = _overfuncs.cbegin(); it != _overfuncs.cend(); ++it) {
	//	it->second->removeVocabulary(vocabulary);
	//}
}

set<Function*> OrderFuncGenerator::nonbuiltins() const {
	set<Function*> sf;
	return sf;
}

namespace FuncUtils {
Function* overload(Function* f1, Function* f2) {
	Assert(f1->name() == f2->name());
	if (f1 == f2) {
		return f1;
	}
	set<Function*> sf;
	sf.insert(f1);
	sf.insert(f2);
	return overload(sf);
}

Function* overload(const set<Function*>& sf) {
	if (sf.empty()) {
		return NULL;
	} else if (sf.size() == 1) {
		return *(sf.cbegin());
	} else {
		EnumeratedFuncGenerator* efg = new EnumeratedFuncGenerator(sf);
		return new Function(efg);
	}
}

bool isIntFunc(const Function* func, const Vocabulary* voc) {
	return SortUtils::isSubsort(func->outsort(), VocabularyUtils::intsort(), voc);
}

bool isIntSum(const Function* function, const Vocabulary* voc) {
	if (function->name() == "+/2" || function->name() == "-/2") {
		bool allintsorts = isIntFunc(function, voc);
		for (auto it = function->insorts().cbegin(); it != function->insorts().cend(); ++it) {
			allintsorts *= SortUtils::isSubsort(*it, VocabularyUtils::intsort(), voc);
		}
		return allintsorts;
	}
	return false;
}
} /* FuncUtils */

/****************
 *	Vocabulary
 ***************/

Vocabulary::Vocabulary(const string& name)
		: _name(name), _namespace(0) {
	if (_name != "std") {
		add(Vocabulary::std());
	}
}

Vocabulary::Vocabulary(const string& name, const ParseInfo& pi)
		: _name(name), _pi(pi), _namespace(0) {
	if (_name != "std") {
		add(Vocabulary::std());
	}
}

Vocabulary::~Vocabulary() {
	if (this == _std) {
		_std = NULL;
	}
	for (auto it = _name2pred.cbegin(); it != _name2pred.cend(); ++it) {
		it->second->removeVocabulary(this);
	}
	for (auto it = _name2func.cbegin(); it != _name2func.cend(); ++it) {
		it->second->removeVocabulary(this);
	}
	for (auto it = _name2sort.cbegin(); it != _name2sort.cend(); ++it) {
		(*it).second->removeVocabulary(this);
	}
}

void Vocabulary::add(Sort* s) {
	if (contains(s)) {
		return;
	}

	_name2sort[s->name()] = s;
	s->addVocabulary(this);
	add(s->pred());
}

// TODO cleaner?
void Vocabulary::add(PFSymbol* symbol) {
	if (sametypeid<Predicate>(*symbol)) {
		add(dynamic_cast<Predicate*>(symbol));
	} else {
		Assert(sametypeid<Function>(*symbol));
		add(dynamic_cast<Function*>(symbol));
	}
}

void Vocabulary::add(Predicate* p) {
	if (contains(p)) {
		return;
	}
	if (p->type() != ST_NONE) {
		Warning::triedAddingSubtypeToVocabulary(p->name(), p->parent()->name(), this->name());
		add(p->parent());
	}

	if (_name2pred.find(p->name()) == _name2pred.cend()) {
		_name2pred[p->name()] = p;
	} else {
		Predicate* ovp = PredUtils::overload(p, _name2pred[p->name()]);
		_name2pred[p->name()] = ovp;
	}
	set<Sort*> ss = p->allsorts();
	for (auto it = ss.cbegin(); it != ss.cend(); ++it) {
		add(*it);
	}
	p->addVocabulary(this);
}

void Vocabulary::add(Function* f) {
	if (contains(f)) {
		return;
	}

	if (_name2func.find(f->name()) == _name2func.cend()) {
		_name2func[f->name()] = f;
	} else {
		Function* ovf = FuncUtils::overload(f, _name2func[f->name()]);
		_name2func[f->name()] = ovf;
	}
	set<Sort*> ss = f->allsorts();
	for (auto it = ss.cbegin(); it != ss.cend(); ++it) {
		add(*it);
	}
	f->addVocabulary(this);
}

void Vocabulary::add(Vocabulary* v) {
	for (auto it = v->firstSort(); it != v->lastSort(); ++it) {
		add((*it).second);
	}
	for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
		add(it->second);
	}
	for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
		add(it->second);
	}
}

Vocabulary* Vocabulary::_std = 0;

std::string getSymbolName(STDSYMBOL s){
	switch(s){
	case STDSYMBOL::STD:
		return "std";
	case STDSYMBOL::NATSORT:
		return "nat";
	case STDSYMBOL::INTSORT:
		return "int";
	case STDSYMBOL::FLOATSORT:
		return "float";
	case STDSYMBOL::CHARSORT:
		return "char";
	case STDSYMBOL::STRINGSORT:
		return "string";
	case STDSYMBOL::EQ:
		return "=/2";
	case STDSYMBOL::GT:
		return ">/2";
	case STDSYMBOL::LT:
		return "</2";
	case STDSYMBOL::MINUS:
		return "-/1";
	case STDSYMBOL::ADDITION:
		return "+/2";
	case STDSYMBOL::SUBSTRACTION:
		return "-/2";
	case STDSYMBOL::PRODUCT:
		return "*/2";
	case STDSYMBOL::DIVISION:
		return "//2";
	case STDSYMBOL::ABS:
		return "abs/1";
	case STDSYMBOL::MODULO:
		return "%/2";
	case STDSYMBOL::EXPONENTIAL:
		return "^/2";
	case STDSYMBOL::MINELEM:
		return "MIN/0";
	case STDSYMBOL::MAXELEM:
		return "MAX/0";
	case STDSYMBOL::SUCCESSOR:
		return "SUCC/1";
	case STDSYMBOL::PREDECESSOR:
		return "PRED/1";
	}
}

Vocabulary* Vocabulary::std() {
	if (not _std) {
		_std = new Vocabulary("std");

		// Create sort interpretations
		SortTable* allnats = new SortTable(new AllNaturalNumbers());
		SortTable* allints = new SortTable(new AllIntegers());
		SortTable* allfloats = new SortTable(new AllFloats());
		SortTable* allstrings = new SortTable(new AllStrings());
		SortTable* allchars = new SortTable(new AllChars());

		// Create sorts
		Sort* natsort = new Sort(getSymbolName(STDSYMBOL::NATSORT), allnats);
		Sort* intsort = new Sort(getSymbolName(STDSYMBOL::INTSORT), allints);
		Sort* floatsort = new Sort(getSymbolName(STDSYMBOL::FLOATSORT), allfloats);
		Sort* charsort = new Sort(getSymbolName(STDSYMBOL::CHARSORT), allchars);
		Sort* stringsort = new Sort(getSymbolName(STDSYMBOL::STRINGSORT), allstrings);

		// Add the sorts
		_std->add(natsort);
		_std->add(intsort);
		_std->add(floatsort);
		_std->add(charsort);
		_std->add(stringsort);

		// Set sort hierarchy 
		intsort->addParent(floatsort);
		natsort->addParent(intsort);
		charsort->addParent(stringsort);

		// Create predicate interpretations
		auto eqgen = new EqualInterGeneratorGenerator();
		auto ltgen = new StrLessThanInterGeneratorGenerator();
		auto gtgen = new StrGreaterThanInterGeneratorGenerator();

		// Create predicate overloaders
		auto eqpgen = new ComparisonPredGenerator(getSymbolName(STDSYMBOL::EQ), eqgen);
		auto ltpgen = new ComparisonPredGenerator(getSymbolName(STDSYMBOL::LT), ltgen);
		auto gtpgen = new ComparisonPredGenerator(getSymbolName(STDSYMBOL::GT), gtgen);

		// Add predicates
		_std->add(new Predicate(eqpgen));
		_std->add(new Predicate(ltpgen));
		_std->add(new Predicate(gtpgen));

		// Create function interpretations
		Universe twoint(vector<SortTable*>(2, allints));
		Universe twofloat(vector<SortTable*>(2, allfloats));
		Universe threeint(vector<SortTable*>(3, allints));
		Universe threefloat(vector<SortTable*>(3, allfloats));

		auto modgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new ModInternalFuncTable(), threeint)));
		auto expgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new ExpInternalFuncTable(), threefloat)));

		vector<Sort*> twoints(2, intsort);
		vector<Sort*> twofloats(2, floatsort);
		vector<Sort*> threeints(3, intsort);
		vector<Sort*> threefloats(3, floatsort);

		auto intplusgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new PlusInternalFuncTable(true), threeint)));
		auto floatplusgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new PlusInternalFuncTable(false), threefloat)));
		auto intplus = new Function(getSymbolName(STDSYMBOL::ADDITION), threeints, intplusgen, 200);
		auto floatplus = new Function(getSymbolName(STDSYMBOL::ADDITION), threefloats, floatplusgen, 200);

		auto intminusgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new MinusInternalFuncTable(true), threeint)));
		auto floatminusgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new MinusInternalFuncTable(false), threefloat)));
		auto intminus = new Function(getSymbolName(STDSYMBOL::SUBSTRACTION), threeints, intminusgen, 200);
		auto floatminus = new Function(getSymbolName(STDSYMBOL::SUBSTRACTION), threefloats, floatminusgen, 200);

		auto inttimesgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new TimesInternalFuncTable(true), threeint)));
		auto floattimesgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new TimesInternalFuncTable(false), threefloat)));
		auto inttimes = new Function(getSymbolName(STDSYMBOL::PRODUCT), threeints, inttimesgen, 300);
		auto floattimes = new Function(getSymbolName(STDSYMBOL::PRODUCT), threefloats, floattimesgen, 300);

		//SingleFuncInterGenerator* intdivgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new DivInternalFuncTable(true), threeint)));
		auto floatdivgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new DivInternalFuncTable(false), threefloat)));
		//Function* intdiv = new Function("//2", threeints, intdivgen, 300);
		auto floatdiv = new Function(getSymbolName(STDSYMBOL::DIVISION), threefloats, floatdivgen, 300);

		auto intabsgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new AbsInternalFuncTable(true), twoint)));
		auto floatabsgen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new AbsInternalFuncTable(false), twofloat)));
		auto intabs = new Function(getSymbolName(STDSYMBOL::ABS), twoints, intabsgen, 0);
		auto floatabs = new Function(getSymbolName(STDSYMBOL::ABS), twofloats, floatabsgen, 0);

		auto intumingen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new UminInternalFuncTable(true), twoint)));
		auto floatumingen = new SingleFuncInterGenerator(new FuncInter(new FuncTable(new UminInternalFuncTable(false), twofloat)));
		auto intumin = new Function(getSymbolName(STDSYMBOL::MINUS), twoints, intumingen, 500);
		auto floatumin = new Function(getSymbolName(STDSYMBOL::MINUS), twofloats, floatumingen, 500);

		auto minigengen = new MinInterGeneratorGenerator();
		auto maxigengen = new MaxInterGeneratorGenerator();
		auto succigengen = new SuccInterGeneratorGenerator();
		auto predigengen = new InvSuccInterGeneratorGenerator();

		// Create function overloaders
		auto plusgen = new IntFloatFuncGenerator(intplus, floatplus);
		auto minusgen = new IntFloatFuncGenerator(intminus, floatminus);
		auto timesgen = new IntFloatFuncGenerator(inttimes, floattimes);
		//IntFloatFuncGenerator* divgen = new IntFloatFuncGenerator(intdiv, floatdiv);
		auto absgen = new IntFloatFuncGenerator(intabs, floatabs);
		auto umingen = new IntFloatFuncGenerator(intumin, floatumin);
		auto mingen = new OrderFuncGenerator(getSymbolName(STDSYMBOL::MINELEM), 0, minigengen);
		auto maxgen = new OrderFuncGenerator(getSymbolName(STDSYMBOL::MAXELEM), 0, maxigengen);
		auto succgen = new OrderFuncGenerator(getSymbolName(STDSYMBOL::SUCCESSOR), 1, succigengen);
		auto predgen = new OrderFuncGenerator(getSymbolName(STDSYMBOL::PREDECESSOR), 1, predigengen);

		// Add functions
		auto modfunc = new Function(getSymbolName(STDSYMBOL::MODULO), threeints, modgen, 100);
		modfunc->partial(true);
		auto expfunc = new Function(getSymbolName(STDSYMBOL::EXPONENTIAL), threefloats, expgen, 400);
		_std->add(modfunc);
		_std->add(expfunc);
		_std->add(floatdiv);
		_std->add(new Function(plusgen));
		_std->add(new Function(minusgen));
		_std->add(new Function(timesgen));
		//_std->add(new Function(divgen));
		_std->add(new Function(absgen));
		_std->add(new Function(umingen));
		_std->add(new Function(mingen));
		_std->add(new Function(maxgen));
		_std->add(new Function(succgen));
		_std->add(new Function(predgen));
	}
	return _std;
}

const string& Vocabulary::name() const {
	return _name;
}

const ParseInfo& Vocabulary::pi() const {
	return _pi;
}

bool Vocabulary::contains(const Sort* s) const {
	auto it = _name2sort.find(s->name());
	return it != _name2sort.cend();
}

bool Vocabulary::containsOverloaded(const Predicate* p) const {
	auto it = _name2pred.find(p->name());
	if (it != _name2pred.cend()) {
		Assert(it->second!=NULL);
		return true;
	} else {
		return false;
	}
}

bool Vocabulary::containsOverloaded(const Function* f) const {
	auto it = _name2func.find(f->name());
	if (it != _name2func.cend()) {
		Assert(it->second!=NULL);
		return true;
	} else {
		return false;
	}
}

bool Vocabulary::contains(const Predicate* p) const {
	map<string, Predicate*>::const_iterator it = _name2pred.find(p->name());
	if (it != _name2pred.cend()) {
		return it->second->contains(p);
	} else {
		return false;
	}
}

bool Vocabulary::contains(const Function* f) const {
	auto it = _name2func.find(f->name());
	if (it != _name2func.cend()) {
		return it->second->contains(f);
	} else {
		return false;
	}
}

bool Vocabulary::contains(const PFSymbol* s) const {
	if (sametypeid<Predicate>(*s)) {
		return contains(dynamic_cast<const Predicate*>(s));
	} else {
		Assert(sametypeid<Function>(*s));
		return contains(dynamic_cast<const Function*>(s));
	}
}

Sort* Vocabulary::sort(const string& name) const {
	auto it = _name2sort.find(name);
	if (it != _name2sort.cend()) {
		return it->second;
	} else {
		return NULL;
	}
}

Predicate* Vocabulary::pred(const string& name) const {
	auto it = _name2pred.find(name);
	if (it != _name2pred.cend()) {
		return it->second;
	} else {
		return 0;
	}
}

Function* Vocabulary::func(const string& name) const {
	auto it = _name2func.find(name);
	if (it != _name2func.cend()) {
		return it->second;
	} else {
		return 0;
	}
}

set<Predicate*> Vocabulary::pred_no_arity(const string& name) const {
	set<Predicate*> vp;
	for (auto it = _name2pred.cbegin(); it != _name2pred.cend(); ++it) {
		string nm = it->second->name();
		if (nm.substr(0, nm.rfind('/')) == name) {
			vp.insert(it->second);
		}
	}
	return vp;
}

set<Function*> Vocabulary::func_no_arity(const string& name) const {
	set<Function*> vf;
	for (auto it = _name2func.cbegin(); it != _name2func.cend(); ++it) {
		string nm = it->second->name();
		if (nm.substr(0, nm.rfind('/')) == name) {
			vf.insert(it->second);
		}
	}
	return vf;
}

ostream& Vocabulary::putName(ostream& output) const {
	if (_namespace && not _namespace->isGlobal()) {
		_namespace->putName(output);
		output << "::";
	}
	output << _name;
	return output;
}

ostream& Vocabulary::put(ostream& output) const {
	pushtab();
	output << "Vocabulary " << _name << ":" <<nt();
	output << "Sorts:";
	pushtab();
	for (auto it = _name2sort.cbegin(); it != _name2sort.cend(); ++it) {
		output <<nt();
		(*it).second->put(output);
	}
	poptab();
	output <<nt();
	output << "Predicates:";
	pushtab();
	for (auto it = _name2pred.cbegin(); it != _name2pred.cend(); ++it) {
		output <<nt();
		it->second->put(output);
	}
	poptab();
	output <<nt();
	output << "Functions:";
	pushtab();
	for (auto it = _name2func.cbegin(); it != _name2func.cend(); ++it) {
		output <<nt();
		it->second->put(output);
	}
	poptab();
	poptab();
	return output;
}

ostream& operator<<(ostream& output, const Vocabulary& voc) {
	return voc.put(output);
}

namespace VocabularyUtils {
Sort* natsort() {
	return Vocabulary::std()->sort("nat");
}
Sort* intsort() {
	return Vocabulary::std()->sort("int");
}

Sort* intRangeSort(int min, int max) {
	stringstream ss;
	ss << "_sort_" << min << '_' << max;
	auto sort = new Sort(ss.str(), new SortTable(new IntRangeInternalSortTable(min, max)));
	sort->addParent(VocabularyUtils::intsort());
	return sort;
}
Sort* floatsort() {
	return Vocabulary::std()->sort("float");
}
Sort* stringsort() {
	return Vocabulary::std()->sort("string");
}
Sort* charsort() {
	return Vocabulary::std()->sort("char");
}

Predicate* equal(Sort* s) {
	vector<Sort*> sorts(2, s);
	return Vocabulary::std()->pred("=/2")->resolve(sorts);
}

Predicate* lessThan(Sort* s) {
	vector<Sort*> sorts(2, s);
	return Vocabulary::std()->pred("</2")->resolve(sorts);
}

Predicate* greaterThan(Sort* s) {
	vector<Sort*> sorts(2, s);
	return Vocabulary::std()->pred(">/2")->resolve(sorts);
}

bool isComparisonPredicate(const PFSymbol* symbol) {
	string name = symbol->name();
	return (sametypeid<Predicate>(*symbol)) && (name == "=/2" || name == "</2" || name == ">/2");
}

bool isIntComparisonPredicate(const PFSymbol* symbol, const Vocabulary* voc) {
	string name = symbol->name();
	if ((sametypeid<Predicate>(*symbol)) && (name == "=/2" || name == "</2" || name == ">/2")) {
		for (auto it = symbol->sorts().cbegin(); it != symbol->sorts().cend(); ++it) {
			if (not SortUtils::isSubsort(*it, VocabularyUtils::intsort(), voc)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool isNumeric(Sort* s) {
	return SortUtils::isSubsort(s, floatsort());
}

} /* VocabularyUtils */
