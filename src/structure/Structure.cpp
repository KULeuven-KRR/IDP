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

#include "Structure.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "utils/ListUtils.hpp"
#include "insert.hpp"
#include "structure/StructureComponents.hpp"
#include "printers/idpprinter.hpp"

using namespace std;

Structure::Structure(const std::string& name, const ParseInfo& pi)
		: _name(name),
			_pi(pi),
			_vocabulary(NULL) {
}
Structure::Structure(const std::string& name, Vocabulary* v, const ParseInfo& pi)
		: _name(name),
			_pi(pi),
			_vocabulary(NULL) {
	changeVocabulary(v);
}

Structure::~Structure() {
	for (auto it = _sortinter.cbegin(); it != _sortinter.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		delete (it->second);
	}
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		delete (it->second);
	}
	deleteList(_intersToDelete);
	changeVocabulary(NULL);
}

void Structure::put(std::ostream& s) const {
	auto p = IDPPrinter<std::ostream>(s);
	p.startTheory();
	p.visit(this);
	p.endTheory();
}

Structure* Structure::clone() const {
	auto s = new Structure("", ParseInfo());
	s->changeVocabulary(_vocabulary);
	for (auto it = _sortinter.begin(); it != _sortinter.end(); ++it) {
		s->inter(it->first)->internTable(it->second->internTable());
	}
	for (auto it = _predinter.begin(); it != _predinter.end(); ++it) {
		s->changeInter(it->first, it->second->clone(s->inter(it->first)->universe()));
	}
	for (auto it = _funcinter.begin(); it != _funcinter.end(); ++it) {
		s->changeInter(it->first, it->second->clone(s->inter(it->first)->universe()));
	}
	return s;
}

void Structure::notifyAddedToVoc(Sort* sort) {
	if (sort->builtin()) {
		return;
	}
	if (_sortinter.find(sort) == _sortinter.cend()) {
		auto st = TableUtils::createSortTable();
		_sortinter[sort] = st;
		vector<SortTable*> univ(1, st);
		auto pt = new PredTable(new FullInternalPredTable(), Universe(univ));
		_predinter[sort->pred()] = new PredInter(pt, true);
	}
}
void Structure::notifyAddedToVoc(PFSymbol* symbol) {
	if (symbol->isFunction()) {
		auto func = dynamic_cast<Function*>(symbol);
		auto sf = func->nonbuiltins();
		for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
			if (_funcinter.find(*jt) == _funcinter.cend()) {
				vector<SortTable*> univ;
				for (auto kt = (*jt)->sorts().cbegin(); kt != (*jt)->sorts().cend(); ++kt) {
					univ.push_back(inter(*kt));
				}
				_funcinter[(*jt)] = TableUtils::leastFuncInter(Universe(univ));
			}
		}
	} else {
		auto pred = dynamic_cast<Predicate*>(symbol);
		auto sp = pred->nonbuiltins();
		for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
			if (_predinter.find(*jt) == _predinter.cend()) {
				vector<SortTable*> univ;
				for (auto kt = (*jt)->sorts().cbegin(); kt != (*jt)->sorts().cend(); ++kt) {
					univ.push_back(inter(*kt));
				}
				_predinter[*jt] = TableUtils::leastPredInter(Universe(univ));
			}
		}
	}
}

/**
 * This method changes the vocabulary of the structure
 * All tables of symbols that do not occur in the new vocabulary are deleted.
 * Empty tables are created for symbols that occur in the new vocabulary, but did not occur in the old one.
 * Except for constructed type symbols, which are set to their interpretation in this structure.
 */
void Structure::changeVocabulary(Vocabulary* v) {
    if (v != _vocabulary) {
		if (_vocabulary != NULL) {
				_vocabulary->removeStructure(this);
		}
		_vocabulary = v;
		if (_vocabulary != NULL) {
				_vocabulary->addStructure(this);
		}
    }else{
    	return;
    }

    if(_vocabulary==NULL){
    	_sortinter.clear();
    	_predinter.clear();
    	_funcinter.clear();
    	return;
    }

	// Delete tables for symbols that do not occur anymore
	for (auto it = _sortinter.begin(); it != _sortinter.end();) {
		if (not v->contains(it->first)) {
			delete (it->second);
			_sortinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		} else {
			++it;
		}
	}
	for (auto it = _predinter.begin(); it != _predinter.end();) {
		if (not _vocabulary->contains(it->first)) {
			delete (it->second);
			_predinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		} else {
			++it;
		}
	}
	for (auto it = _funcinter.begin(); it != _funcinter.end();) {
		if (not _vocabulary->contains(it->first)) {
			delete (it->second);
			_funcinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		} else {
			++it;
		}
	}
	// Create empty tables for new sorts
	for (auto it = _vocabulary->firstSort(); it != _vocabulary->lastSort(); ++it) {
		auto sort = it->second;
		if (not sort->builtin()) {
			if (_sortinter.find(sort) == _sortinter.cend()) {
				SortTable* st;
				if (sort->isConstructed()) {
					st = getConstructedInterpretation(sort, this);
				} else {
					st = TableUtils::createSortTable();
				}
				_sortinter[sort] = st;
				auto pt = new PredTable(new FullInternalPredTable(), Universe({st}));
				_predinter[sort->pred()] = new PredInter(pt, true);
			}
		}
	}

	//... and for new predicates and functions
	createPredAndFuncTables(false);

}

void Structure::reset() {
	createPredAndFuncTables(true);
}
void Structure::createPredAndFuncTables(bool forced) {
	for (auto nameAndPred = _vocabulary->firstPred(); nameAndPred != _vocabulary->lastPred(); ++nameAndPred) {
		auto pred = nameAndPred->second;
		for (auto nonOverloadedPred : pred->nonbuiltins()) {
			auto sorts = nonOverloadedPred->sorts();
			if (sorts.size() == 1 && sorts.at(0)->pred() == nameAndPred->second) {
				continue;
			}
			if (forced || _predinter.find(nonOverloadedPred) == _predinter.cend()) {
				vector<SortTable*> univ;
				for (auto kt : nonOverloadedPred->sorts()) {
					univ.push_back(inter(kt));
				}
				_predinter[nonOverloadedPred] = TableUtils::leastPredInter(Universe(univ));
			}
		}
	}
	for (auto it = _vocabulary->firstFunc(); it != _vocabulary->lastFunc(); ++it) {
		auto sf = it->second->nonbuiltins();
		for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
			if (forced || _funcinter.find(*jt) == _funcinter.cend()) {
				vector<SortTable*> univ;
				for (auto kt = (*jt)->sorts().cbegin(); kt != (*jt)->sorts().cend(); ++kt) {
					univ.push_back(inter(*kt));
				}
				_funcinter[(*jt)] = TableUtils::leastFuncInter(Universe(univ));
			}
		}
	}
}

void Structure::changeInter(Sort* f, SortTable* i) {
	Assert(_sortinter[f]!=NULL);
	_sortinter[f]->internTable(i->internTable());
}

void Structure::changeInter(Predicate* p, PredInter* i) {
	Assert(_predinter[p]!=NULL);
	delete (_predinter[p]);
	_predinter[p] = i;
}

void Structure::changeInter(Function* f, FuncInter* i) {
	Assert(_funcinter[f]!=NULL);
	delete (_funcinter[f]);
	_funcinter[f] = i;
}

bool Structure::approxTwoValued() const {
	for (auto funcInterIterator = _funcinter.cbegin(); funcInterIterator != _funcinter.cend(); ++funcInterIterator) {
		FuncInter* fi = (*funcInterIterator).second;
		if (not fi->approxTwoValued()) {
			return false;
		}
	}
	for (auto predInterIterator = _predinter.cbegin(); predInterIterator != _predinter.cend(); ++predInterIterator) {
		PredInter* pi = (*predInterIterator).second;
		if (not pi->approxTwoValued()) {
			return false;
		}
	}
	return true;
}

bool Structure::isConsistent() const {
	for (auto name2func : _funcinter) {
		auto inter = name2func.second;
		if (not inter->isConsistent()) {
// TODO warning was here as it is the only way to get information on inconsistent structures to the user, but this was not the correct location or way to do it.
//			stringstream ss;
//			ss << "Inconsistent interpretation for " << print(name2func.first) << ", namely the atoms " << print(inter->getInconsistentAtoms()) << "\n";
//			Warning::warning(ss.str());
			return false;
		}
	}
	for (auto name2pred : _predinter) {
		auto inter = name2pred.second;
		if (not inter->isConsistent()) {
//			stringstream ss;
//			ss << "Inconsistent interpretation for " << print(name2pred.first) << ", namely the atoms " << print(inter->getInconsistentAtoms()) << "\n";
//			Warning::warning(ss.str());
			return false;
		}
	}
	return true;
}

void makeUnknownsFalse(PredInter* inter) {
	Assert(inter!=NULL);

	if (inter->approxTwoValued()) {
		return;
	}
	inter->pt(new PredTable(inter->ct()->internTable(), inter->ct()->universe()));
	auto cfpf = new PredTable(InverseInternalPredTable::getInverseTable(inter->pt()->internTable()), inter->pt()->universe());
	inter->cfpf(cfpf);
	delete(cfpf);
}

void makeUnknownsFalse(Structure* structure){
	for (auto pred : structure->vocabulary()->getPreds()) {
		for (auto predToSet : pred.second->nonbuiltins()) {
			makeUnknownsFalse(structure->inter(predToSet));
		}
	}
	for (auto func : structure->vocabulary()->getFuncs()) {
		for (auto funcToSet : func.second->nonbuiltins()) {
			makeUnknownsFalse(structure->inter(funcToSet)->graphInter());
		}
	}
}

void makeTwoValued(Function* function, FuncInter* inter){
	if (not inter->isConsistent()) {
		throw IdpException("Error, trying to make an inconsistent structure two-valued.");
	}
	if (inter->approxTwoValued()) {
		return;
	}
	// create a generator for the interpretation
	auto universe = inter->graphInter()->universe();
	const auto& sorts = universe.tables();

	vector<SortIterator> domainIterators;
	bool allempty = true;
	for (auto sort : sorts) {
		const auto& temp = SortIterator(sort->internTable()->sortBegin());
		domainIterators.push_back(temp);
		if (not temp.isAtEnd()) {
			allempty = false;
		}
	}
	domainIterators.pop_back();
	if(domainIterators.size()==0) {
		allempty = false;
	}

	auto ct = inter->graphInter()->ct();
	auto cf = inter->graphInter()->cf();

	//Now, choose an image for this domainelement
	auto internaliterator = new CartesianInternalTableIterator(domainIterators, domainIterators, not allempty);
	TableIterator domainIterator(internaliterator);

	auto ctIterator = ct->begin();
	FirstNElementsEqual eq(function->arity());
	StrictWeakNTupleOrdering so(function->arity());

	for (; not domainIterator.isAtEnd(); ++domainIterator) {
		CHECKTERMINATION
		// get unassigned domain element
		auto domainElementWithoutValue = *domainIterator;
		while (not ctIterator.isAtEnd() && so(*ctIterator, domainElementWithoutValue)) {
			++ctIterator;
		}
		if (not ctIterator.isAtEnd() && eq(domainElementWithoutValue, *ctIterator)) {
			continue;
		}

		auto imageIterator = SortIterator(sorts.back()->internTable()->sortBegin());
		for (; not imageIterator.isAtEnd(); ++imageIterator) {
			CHECKTERMINATION
			ElementTuple tuple(domainElementWithoutValue);
			tuple.push_back(*imageIterator);
			if (cf->contains(tuple)) {
				continue;
			}
			inter->graphInter()->makeTrueExactly(tuple);
			break;
		}
	}
	clean(function, inter);
}

void makeTwoValued(Predicate* p, PredInter* inter){
	if (not inter->isConsistent()) {
		throw IdpException("Error, trying to make an inconsistent structure two-valued.");
	}
	inter->pt(new PredTable(inter->ct()->internTable(), inter->ct()->universe()));
	clean(p,inter);
}
void Structure::makeTwoValued() {
	if (not isConsistent()) {
		throw IdpException("Error, trying to make an inconsistent structure two-valued.");
	}
	if (approxTwoValued()) {
		return;
	}
	for (auto f2inter : _funcinter) {
		CHECKTERMINATION;
		::makeTwoValued(f2inter.first, f2inter.second);
	}
	for (auto i = _predinter.begin(); i != _predinter.end(); i++) {
		CHECKTERMINATION;
		::makeTwoValued((*i).first, (*i).second);
	}
	clean();
}

void computescore(Sort* s, map<Sort*, unsigned int>& scores) {
	if (contains(scores, s)) {
		return;
	}
	unsigned int sc = 0;
	for (auto it = s->parents().cbegin(); it != s->parents().cend(); ++it) {
		computescore(*it, scores);
		if (scores[*it] >= sc) {
			sc = scores[*it] + 1;
		}
	}
	scores[s] = sc;
}

bool addToInterpretation(Structure* structure, Sort* sort, const DomainElement* e) {
    if (e->type() == DET_COMPOUND) {
        const Compound* c = e->value()._compound;
        for (uint i = 0; i < c->args().size(); i++) {
            if ( not addToInterpretation(structure, c->function()->sort(i), c->args().at(i))){
                return false;
            }
        }
    }
    // NOTE: we do not autocomplete sorts for which the interpretation is provided by the user, since this is a bug more often than not.
    if (not sort->hasFixedInterpretation()
            && not getGlobal()->getInserter().interpretationSpecifiedByUser(structure, sort)
            && getOption(AUTOCOMPLETE)) {
        structure->inter(sort)->add(e);
    } else if (!structure->inter(sort)->contains(e) && not getOption(ASSUMECONSISTENTINPUT)) {
        return false;
    }
    return true;
}

template<class Table>
void checkAndCompleteSortTable(const Table* pt, const Universe& univ, PFSymbol* symbol, Structure* structure) {
    if (not pt->approxFinite() || (getOption(ASSUMECONSISTENTINPUT) && not getOption(AUTOCOMPLETE))) {
        return;
    }
    for (auto jt = pt->begin(); not jt.isAtEnd(); ++jt) {
        const ElementTuple& tuple = *jt;
        for (unsigned int col = 0; col < tuple.size(); ++col) {
            auto sort = symbol->sorts()[col];
            auto e = tuple[col];
            if (not addToInterpretation(structure, sort, e)) {
                if (typeid (*symbol) == typeid (Predicate)) {
                    Error::predelnotinsort(toString(e), symbol->name(), sort->name(), structure->name());
                } else {
                    Error::funcelnotinsort(toString(e), symbol->name(), sort->name(), structure->name());
                }
            }
        }
    }
}

void addUNAPattern(Function*) {
	throw notyetimplemented("una pattern type");
}

void Structure::autocompleteFromSymbol(PFSymbol* symbol, PredInter* inter) {
	auto pt1 = inter->ct();
	if (isa<InverseInternalPredTable>(*(pt1->internTable()))) {
		pt1 = inter->pf();
	}
	checkAndCompleteSortTable(pt1, pt1->universe(), symbol, this);
	if (not inter->approxTwoValued()) {
		auto pt2 = inter->cf();
		if (isa<InverseInternalPredTable>(*(pt2->internTable()))) {
			pt2 = inter->pt();
		}
		checkAndCompleteSortTable(pt2, pt2->universe(), symbol, this);
	}
}

void Structure::checkAndAutocomplete() {
	if (getOption(SHOWWARNINGS)) {
		stringstream ss;
		ss <<"Verifying and/or autocompleting structure " <<name();
		Warning::warning(ss.str());
	}
	// Adding elements from predicate interpretations to sorts
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		auto pred = it->first;
		if (pred->arity() == 1 && pred->sorts()[0]->pred() == pred) {
			continue; // It was a sort itself
			/* NOTE: using sort interpretations for autocompleting types is only useful to
			* - extend a supertype
			* - interpret the base types of a constructed type
			* Both are done later in this method
			*/
		}
		autocompleteFromSymbol(pred, it->second);
	}
	// Adding elements from function interpretations to sorts
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		auto func = it->first;
		auto inter = it->second;

		if (inter->funcTable() && isa<UNAInternalFuncTable>(*(inter->funcTable()->internTable()))) {
			addUNAPattern(func);
			continue;
		}

		autocompleteFromSymbol(func, inter->graphInter());
	}

	if (getOption(AUTOCOMPLETE)) {
		// Adding elements from subsorts to supersorts
		map<Sort*, unsigned int> levels; // Maps a sort to a level such that all its parents are on lower levels and no 2 sorts in the same hierarchy have the same level.
		for (auto it = _vocabulary->firstSort(); it != _vocabulary->lastSort(); ++it) {
			computescore(it->second, levels);
		}
		map<unsigned int, vector<Sort*> > levels2sorts; // Map a level to all the sorts in it
		for (auto sort2level : levels) {
			if (_vocabulary->contains(sort2level.first)) {
				levels2sorts[sort2level.second].push_back(sort2level.first);
			}
		}
		// Go over the level from lowest to highest (so starting at the lowest sorts in the hierarchy) and at each level adding its elements to the parent sorts.
		for (auto level2sorts = levels2sorts.rbegin(); level2sorts != levels2sorts.rend(); level2sorts++) {
			for (auto sort : (*level2sorts).second) {
				set<Sort*> notextend = { sort };
				vector<Sort*> toextend, tocheck;
				while (not notextend.empty()) {
					auto e = *(notextend.cbegin());
					for (auto sp : e->parents()) {
						if (_vocabulary->contains(sp)) {
							if (sp->builtin() || getGlobal()->getInserter().interpretationSpecifiedByUser(this, sp)) {
								tocheck.push_back(sp);
							} else {
								toextend.push_back(sp);
							}
						} else {
							notextend.insert(sp);
						}
					}
					notextend.erase(e);
				}
				auto st = inter(sort);
				for (auto kt : toextend) {
					if (not st->approxFinite()) {
						throw notyetimplemented("Completing non approx-finite tables");
					}
					for (auto lt = st->sortBegin(); not lt.isAtEnd(); ++lt) {
                                            addToInterpretation(this,kt,*lt);
					}
				}
				if (sort->builtin()) {
					continue;
				}
				for (auto kt : tocheck) {
					auto kst = inter(kt);
					// TODO speedup for common cases (expensive if both tables are large) => should be in some general visitor which checks this!
					if (dynamic_cast<AllIntegers*>(kst->internTable()) != NULL && dynamic_cast<IntRangeInternalSortTable*>(st->internTable()) != NULL) {
						continue;
					}
					if (dynamic_cast<AllNaturalNumbers*>(kst->internTable()) != NULL && dynamic_cast<IntRangeInternalSortTable*>(st->internTable()) != NULL
							&& dynamic_cast<IntRangeInternalSortTable*>(st->internTable())->first()->value()._int > -1) {
						continue;
					}
					if (not st->approxFinite()) {
						Warning::warning("There is no auto-completion of infinite symbol interpretations");
						continue;
					}
					for (auto lt = st->sortBegin(); not lt.isAtEnd(); ++lt) {
						if (not kst->contains(*lt)) {
							Error::sortelnotinsort(toString(*lt), sort->name(), kt->name(), _name);
						}
					}
				}
			}
		}
	}
}

void Structure::addStructure(Structure*) {
	throw notyetimplemented("Add a structure to another one.");
}

void Structure::sortCheck() const {
	for (auto sort2inter : _sortinter) {
		auto sort = sort2inter.first;
		if (not sort->isConstructed() && sort2inter.second->empty()) {
			Warning::emptySort(sort->name(), name());
		}
	}
}

bool Structure::satisfiesFunctionConstraints(bool throwerrors) const {
	for (auto func2inter : _funcinter) {
		if (not functionCheck(func2inter.first, throwerrors)) {
			return false;
		}
	}
	return true;
}

bool Structure::satisfiesFunctionConstraints(const Function* f, bool throwerrors) const {
	return functionCheck(f, throwerrors);
}

bool Structure::functionCheck(const Function* f, bool throwErrors) const {
	auto fi = inter(f);
	if(f->builtin()){ // builtins are always valid function interpretations
		return true;
	}
	if (not fi->universe().approxFinite()) {
		Warning::warning("Consistency cannot be checked for functions over an infinite domain.");
		return true;
	}

	// When parsing (throwErrors = true), don't make this cheap check, because
	// the parser will have created a two-valued function table
	if(not throwErrors and fi->approxTwoValued()){
		return true;
	}

	auto pt = fi->graphInter();

	// Check whether each input tuple maps to less than two output tuples
	//This check should not happen to an interpretation with a functable (as this can be definition only map to at most one tuple!
	if (fi->funcTable() == NULL) {
		auto ct = pt->ct();
		// Check if the interpretation is indeed a function
		FirstNElementsEqual eq(f->arity());
		auto ctit = ct->begin();
		if (not ctit.isAtEnd()) {
			auto jt = ct->begin();
			++jt;
			for (; not jt.isAtEnd(); ++ctit, ++jt) {
				if (eq(*ctit, *jt)) { // Found a tuple that violates the constraint
					if (throwErrors) {
						const auto& tuple = *ctit;
						vector<string> vstr;
						for (size_t c = 0; c < f->arity(); ++c) {
							vstr.push_back(toString(tuple[c]));
						}
						Error::notfunction(f->name(), name(), vstr);
					}
					return false;
				}
			}
		}
	}

	// For partial functions the totality check need not be done
	if ( f->partial()) {
		return true;
	}

	auto domainUnivTables = fi->universe().tables();
	domainUnivTables.pop_back(); //Everything but the output table
	auto domainUnivSize = Universe(domainUnivTables).size();

	// Check if the interpretation is total
	// We distinguish two cases
	//CASE ONE: Function is represented by functable.
	//In this case, all we need to is iterate over the domain and check that every domain element has an image.
	//Or, even simpler: check that the number of elements in "functable" equals the number of elements in the universe
	if (fi->funcTable() != NULL) {
		auto ft = fi->funcTable();
		auto ftsize = ft->size();
		auto domainUnivTables = fi->universe().tables();
		domainUnivTables.pop_back(); //Everything but the output table
		auto domainUnivSize = Universe(domainUnivTables).size();

		//Infinite case is already handled above.
		if (domainUnivSize == ftsize) {
			//as much tuples in the map as input elements
			return true;
		}
		if (throwErrors) {
			Error::nottotal(f->name(), name());
		}
		return false;
	}

	//CASE TWO: REPRSENTED AS GRAPHINTER:
	//Go over all CF images; count number of impossibles.
	// TODO: could be further optimised by running over pt instead of cf in case pt is explicitely represented.
	//However, this will not often be the case
	auto cf = pt->cf();
	auto maxnbimages = inter(f->outsort())->size();
	if (maxnbimages == 0 && domainUnivSize != tablesize(TableSizeType::TST_EXACT, 0)) {
		if (throwErrors) {
			Error::nottotal(f->name(), name());
		}
		return false;
	}
	map<ElementTuple, int> domain2numberofimages;
	for(auto cfit = cf->begin(); not cfit.isAtEnd(); ++cfit) {
		auto domain = *cfit;
		domain.pop_back();
		auto domit = domain2numberofimages.find(domain);
		int count = 0;
		if(domit==domain2numberofimages.cend()){
			domain2numberofimages[domain]=1;
			count = 1;
		}else{
			domit->second++;
			count = domit->second;
		}
		if (count==maxnbimages) {
			if (throwErrors) {
				Error::nottotal(f->name(), name());
			}
			return false;
		}
	}
	return true;
}

bool Structure::hasInter(const Sort* s) const {
	return s != NULL && (s->hasFixedInterpretation() || _sortinter.find(const_cast<Sort*>(s)) != _sortinter.cend());
}

SortTable* Structure::inter(const Sort* s) const {
	if (s == NULL) {
		throw IdpException("Sort was NULL");
	}

	if (s->builtin()) {
		return s->interpretation();
	}

	auto sortit = _sortinter.find(const_cast<Sort*>(s));
	Assert(sortit != _sortinter.cend());
	return sortit->second;
}

SortTable* Structure::storableInter(const Sort* s) const {
	auto unstorableSortTable = inter(s);
	auto tmp = new SortTable(unstorableSortTable->internTable());
	return tmp;
}

PredInter* Structure::inter(const Predicate* p) const {
	if (p == NULL) {
		throw IdpException("Predicate was NULL");
	}
	if (p->builtin()) {
		return p->interpretation(this);
	}

	if (p->type() == ST_NONE) {
		auto it = _predinter.find(const_cast<Predicate*>(p));
		if (it == _predinter.cend()) {
			stringstream ss;
			ss << "The structure does not contain the predicate " << p->name();
			throw IdpException(ss.str());
		}
		return it->second;
	}

	PredInter* pinter = inter(p->parent());
	PredInter* newinter = NULL;
	Assert(p->type() != ST_NONE);
	switch (p->type()) {
	case ST_CT:
		newinter = new PredInter(new PredTable(pinter->ct()->internTable(), pinter->universe()), true);
		break;
	case ST_CF:
		newinter = new PredInter(new PredTable(pinter->cf()->internTable(), pinter->universe()), true);
		break;
	case ST_PT:
		newinter = new PredInter(new PredTable(pinter->pt()->internTable(), pinter->universe()), true);
		break;
	case ST_PF:
		newinter = new PredInter(new PredTable(pinter->pf()->internTable(), pinter->universe()), true);
		break;
	default:
		break;
	}
	_intersToDelete.push_back(newinter);
	return newinter;
}

FuncInter* Structure::inter(const Function* f) const {
	if(f==NULL){
		throw IdpException("Function was NULL");
	}
	if(f->overloaded()){
		throw IdpException("Cannot get the interpretation of a non-disambiguated function.");
	}
	if (f->builtin()) {
		auto it = _fixedfuncinter.find(f);
		if(it==_fixedfuncinter.cend()){
			auto inter = f->interpretation(this);
			_fixedfuncinter[f]=inter;
			return inter;
		}
		return it->second;
	} else {
		auto it = _funcinter.find(const_cast<Function*>(f));
		Assert(it != _funcinter.cend());
		return it->second;
	}
}

PredInter* Structure::inter(const PFSymbol* s) const {
	if (isa<const Predicate>(*s)) {
		return inter(dynamic_cast<const Predicate*>(s));
	} else {
		Assert(isa<const Function>(*s));
		return inter(dynamic_cast<const Function*>(s))->graphInter();
	}
}

Universe Structure::universe(const PFSymbol* s) const {
	vector<SortTable*> vst;
	for (auto it = s->sorts().cbegin(); it != s->sorts().cend(); ++it) {
		vst.push_back(inter(*it));
	}
	return Universe(vst);
}

// TODO new name and document
void Structure::materialize() {
	for (auto it = _sortinter.cbegin(); it != _sortinter.cend(); ++it) {
		SortTable* st = it->second->materialize();
		if (st != NULL) {
			_sortinter[it->first] = st;
		}
	}
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		it->second->materialize();
	}
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		it->second->materialize();
	}
}

void clean(Predicate*, PredInter* inter){
	if (inter->approxTwoValued()) {
		return;
	}
	if(not inter->isConsistent()){
		return;
	}
	if (not TableUtils::isInverse(inter->ct(), inter->cf())) {
		return;
	}
	auto npt = new PredTable(inter->ct()->internTable(), inter->ct()->universe());
	inter->pt(npt);
}

void clean(Function* function, FuncInter* inter){
	if (inter->approxTwoValued()) {
		return;
	}
	if(not inter->isConsistent()){
		return;
	}
	if (function->partial()) {
		auto lastsorttable = inter->universe().tables().back();
		for (auto ctit = inter->graphInter()->ct()->begin(); not ctit.isAtEnd(); ++ctit) {
			auto tuple = *ctit;
			auto ctvalue = tuple.back();
			for (auto sortit = lastsorttable->sortBegin(); not sortit.isAtEnd(); ++sortit) {
				auto cfvalue = *sortit;
				if (*cfvalue != *ctvalue) {
					tuple.pop_back();
					tuple.push_back(*sortit);
					inter->graphInter()->makeFalseExactly(tuple);
				}
			}
		}
	}

	if ((function->partial() && TableUtils::isInverse(inter->graphInter()->ct(), inter->graphInter()->cf())) ||
	 (TableUtils::approxTotalityCheck(inter) && inter->isConsistent())) {
		auto eift = new EnumeratedInternalFuncTable();
		for (auto jt = inter->graphInter()->ct()->begin(); not jt.isAtEnd(); ++jt) {
			eift->add(*jt);
		}
		if (eift->size(inter->graphInter()->ct()->universe()) == inter->graphInter()->ct()->size()) {
			//TODO: too expensive. We should be able to directly transform ct-table to functable!
			inter->funcTable(new FuncTable(eift, inter->graphInter()->ct()->universe()));
		}
	}
}

//TODO Shouldn't this be approxClean?
void Structure::clean() {
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		::clean(it->first, it->second);
	}
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		::clean(it->first, it->second);
	}
}
