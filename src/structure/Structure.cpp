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

void Structure::notifyAddedToVoc(Sort* sort){
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
void Structure::notifyAddedToVoc(PFSymbol* symbol){
	if(symbol->isFunction()){
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
	}else{
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
	// Create empty tables for new symbols
	for (auto it = _vocabulary->firstSort(); it != _vocabulary->lastSort(); ++it) {
		auto sort = it->second;
		if (not sort->builtin()) {
			if (_sortinter.find(sort) == _sortinter.cend()) {
				auto st = TableUtils::createSortTable();
				_sortinter[sort] = st;
				vector<SortTable*> univ(1, st);
				auto pt = new PredTable(new FullInternalPredTable(), Universe(univ));
				_predinter[sort->pred()] = new PredInter(pt, true);
			}
		}
	}
	for (auto it = _vocabulary->firstPred(); it != _vocabulary->lastPred(); ++it) {
		auto sp = it->second->nonbuiltins();
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
	for (auto it = _vocabulary->firstFunc(); it != _vocabulary->lastFunc(); ++it) {
		auto sf = it->second->nonbuiltins();
		for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
			if (_funcinter.find(*jt) == _funcinter.cend()) {
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
	delete (_sortinter[f]);
	_sortinter[f] = i;
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
			stringstream ss;
			ss <<"Inconsistent interpretation for " <<print(name2func.first) <<", namely the atoms " <<print(inter->getInconsistentAtoms()) <<"\n";
			Warning::warning(ss.str());
			return false;
		}
	}
	for (auto name2pred :  _predinter) {
		auto inter = name2pred.second;
		if (not inter->isConsistent()) {
			stringstream ss;
			ss <<"Inconsistent interpretation for " <<print(name2pred.first) <<", namely the atoms " <<print(inter->getInconsistentAtoms()) <<"\n";
			Warning::warning(ss.str());
			return false;
		}
	}
	return true;
}

void makeUnknownsFalse(PredInter* inter){
	Assert(inter!=NULL);

	if (inter->approxTwoValued()) {
		return;
	}
	inter->pt(new PredTable(inter->ct()->internTable(), inter->ct()->universe()));
	inter->cfpf(new PredTable(InverseInternalPredTable::getInverseTable(inter->pt()->internTable()), inter->pt()->universe()));
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
		auto inter = f2inter.second;
		if (inter->approxTwoValued()) {
			continue;
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
		FirstNElementsEqual eq(f2inter.first->arity());
		StrictWeakNTupleOrdering so(f2inter.first->arity());

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
				inter->graphInter()->makeTrue(tuple);
				break;
			}
		}
	}
	for (auto i = _predinter.begin(); i != _predinter.end(); i++) {
		CHECKTERMINATION;
		auto inter = (*i).second;
		Assert(inter!=NULL);
		inter->pt(new PredTable(inter->ct()->internTable(), inter->ct()->universe()));
	}
	clean();
	Assert(approxTwoValued());
}

void computescore(Sort* s, map<Sort*, unsigned int>& scores) {
	if(contains(scores, s)){
		return;
	}
	unsigned int sc = 0;
	for (auto it = s->parents().cbegin(); it != s->parents().cend(); ++it) {
		computescore(*it, scores);
		if (scores[*it] >= sc){
			sc = scores[*it] + 1;
		}
	}
	scores[s] = sc;
}

void checkAndCompleteSortTable(const PredTable* pt, PFSymbol* symbol, Structure* structure) {
	if (not pt->approxFinite()) {
		return;
	}
	for (auto jt = pt->begin(); not jt.isAtEnd(); ++jt) {
		const ElementTuple& tuple = *jt;
		for (unsigned int col = 0; col < tuple.size(); ++col) {
			auto sort = symbol->sorts()[col];
			// NOTE: we do not use predicate/function interpretations to autocomplete user provided sorts, this is a bug more often than not
			if (not sort->builtin() && not getGlobal()->getInserter().interpretationSpecifiedByUser(structure, sort) && getOption(AUTOCOMPLETE)) {
				pt->universe().tables()[col]->add(tuple[col]);
			} else if (!pt->universe().tables()[col]->contains(tuple[col])) {
				if (typeid(*symbol) == typeid(Predicate)) {
					Error::predelnotinsort(toString(tuple[col]), symbol->name(), sort->name(), structure->name());
				} else {
					Error::funcelnotinsort(toString(tuple[col]), symbol->name(), sort->name(), structure->name());
				}
			}
		}
	}
}

void addUNAPattern(Function*) {
	throw notyetimplemented("una pattern type");
}

void Structure::autocompleteFromSymbol(PFSymbol* symbol, PredInter* inter){
	auto pt1 = inter->ct();
	if (isa<InverseInternalPredTable>(*(pt1->internTable()))) {
		pt1 = inter->pf();
	}
	checkAndCompleteSortTable(pt1, symbol, this);
	if (not inter->approxTwoValued()) {
		auto pt2 = inter->cf();
		if (isa<InverseInternalPredTable>(*(pt2->internTable()))) {
			pt2 = inter->pt();
		}
		checkAndCompleteSortTable(pt2, symbol, this);
	}
}

void Structure::checkAndAutocomplete() {
	if(getOption(SHOWWARNINGS)){
		Warning::warning("Autocompleting structure");
	}
	// Adding elements from predicate interpretations to sorts
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		auto pred = it->first;
		if (pred->arity() == 1 && pred->sorts()[0]->pred() == pred) {
			continue; // It was a sort itself
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

	if(getOption(AUTOCOMPLETE)){
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
		for (auto level2sorts : levels2sorts) {
			for (auto sort : level2sorts.second) {
				set<Sort*> notextend = { sort };
				vector<Sort*> toextend, tocheck;
				while (not notextend.empty()) {
					auto e = *(notextend.cbegin());
					for (auto sp : e->parents()) {
						if (_vocabulary->contains(sp)) {
							if (sp->builtin()) {
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
					auto kst = inter(kt);
					if(not st->approxFinite()){
						throw notyetimplemented("Completing non approx-finite tables");
					}
					for (auto lt = st->sortBegin(); not lt.isAtEnd(); ++lt) {
						kst->add(*lt);
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
						if (not kst->contains(*lt)){
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
	for (auto i = _sortinter.cbegin(); i != _sortinter.cend(); ++i) {
		if ((*i).second->empty()) {
			Warning::emptySort(i->first->name(), name());
		}
	}
}

void Structure::functionCheck() {
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		Function* f = it->first;
		FuncInter* ft = it->second;
		if (it->second->universe().approxFinite()) {
			PredInter* pt = ft->graphInter();
			const PredTable* ct = pt->ct();
			// Check if the interpretation is indeed a function
			bool isfunc = true;
			FirstNElementsEqual eq(f->arity());
			TableIterator it = ct->begin();
			if (not it.isAtEnd()) {
				TableIterator jt = ct->begin();
				++jt;
				for (; not jt.isAtEnd(); ++it, ++jt) {
					if (eq(*it, *jt)) {
						const ElementTuple& tuple = *it;
						vector<string> vstr;
						for (size_t c = 0; c < f->arity(); ++c) {
							vstr.push_back(toString(tuple[c]));
						}
						Error::notfunction(f->name(), name(), vstr);
						do {
							++it;
							++jt;
						} while (not jt.isAtEnd() && eq(*it, *jt));
						isfunc = false;
					}
				}
			}
			// Check if the interpretation is total
			if (isfunc && !(f->partial()) && ft->approxTwoValued() && ct->approxFinite()) {
				vector<SortTable*> vst;
				vector<bool> linked;
				for (size_t c = 0; c < f->arity(); ++c) {
					vst.push_back(inter(f->insort(c)));
					linked.push_back(true);
				}
				PredTable spt(new FullInternalPredTable(), Universe(vst));
				it = spt.begin();
				TableIterator jt = ct->begin();
				for (; not it.isAtEnd() && not jt.isAtEnd(); ++it, ++jt) {
					if (not eq(*it, *jt)) {
						break;
					}
				}
				if (not it.isAtEnd() || not jt.isAtEnd()) {
					Error::nottotal(f->name(), name());
				}
			}
		}
	}
}

bool Structure::hasInter(const Sort* s) const {
	return s != NULL && (s->builtin() || _sortinter.find(const_cast<Sort*>(s)) != _sortinter.cend());
}

SortTable* Structure::inter(const Sort* s) const {
	if (s == NULL) { // TODO prevent error by introducing UnknownSort object (prevent nullpointers)
		throw IdpException("Sort was NULL");
	}

	if (s->builtin()) {
		return s->interpretation();
	}

	auto sortit = _sortinter.find(const_cast<Sort*>(s));
	Assert(sortit != _sortinter.cend());
	return sortit->second;
}

PredInter* Structure::inter(const Predicate* p) const {
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
	if (f->builtin()) {
		return f->interpretation(this);
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

//TODO Shouldn't this be approxClean?
void Structure::clean() {
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		if (it->second->approxTwoValued()) {
			continue;
		}
		if (not TableUtils::approxIsInverse(it->second->ct(), it->second->cf())) {
			continue;
		}
		auto npt = new PredTable(it->second->ct()->internTable(), it->second->ct()->universe());
		it->second->pt(npt);
	}
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		if (it->second->approxTwoValued()) {
			continue;
		}
		if (it->first->partial()) {
			auto lastsorttable = it->second->universe().tables().back();
			for (auto ctit = it->second->graphInter()->ct()->begin(); not ctit.isAtEnd(); ++ctit) {
				auto tuple = *ctit;
				auto ctvalue = tuple.back();
				for (auto sortit = lastsorttable->sortBegin(); not sortit.isAtEnd(); ++sortit) {
					auto cfvalue = *sortit;
					if (*cfvalue != *ctvalue) {
						tuple.pop_back();
						tuple.push_back(*sortit);
						it->second->graphInter()->makeFalse(tuple);
					}
				}
			}
		}

		// TODO this code should be reviewed!
		if (((not it->first->partial()) && TableUtils::approxTotalityCheck(it->second))
				|| TableUtils::approxIsInverse(it->second->graphInter()->ct(), it->second->graphInter()->cf())) {
			auto eift = new EnumeratedInternalFuncTable();
			for (auto jt = it->second->graphInter()->ct()->begin(); not jt.isAtEnd(); ++jt) {
				eift->add(*jt);
			}
			//TODO: too expensive. We should be able to directly transform ct-table to functable!
			it->second->funcTable(new FuncTable(eift, it->second->graphInter()->ct()->universe()));
		}
	}
}
