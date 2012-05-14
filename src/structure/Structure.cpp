/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "Structure.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "utils/ListUtils.hpp"
#include "insert.hpp"

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

/**
 * This method changes the vocabulary of the structure
 * All tables of symbols that do not occur in the new vocabulary are deleted.
 * Empty tables are created for symbols that occur in the new vocabulary, but did not occur in the old one.
 */
void Structure::changeVocabulary(Vocabulary* v) {
	_vocabulary = v;
	// Delete tables for symbols that do not occur anymore
	for (auto it = _sortinter.begin(); it != _sortinter.end();) {
		if (not v->contains(it->first)) {
			delete (it->second);
			_sortinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		}else{
			++it;
		}
	}
	for (auto it = _predinter.begin(); it != _predinter.end();) {
		if (not v->contains(it->first)) {
			delete (it->second);
			_predinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		}else{
			++it;
		}
	}
	for (auto it = _funcinter.begin(); it != _funcinter.end();) {
		if (not v->contains(it->first)) {
			delete (it->second);
			_funcinter.erase(it++); // NOTE: increment here is important for iterator consistency in map erasure
		}else{
			++it;
		}
	}
	// Create empty tables for new symbols
	for (auto it = _vocabulary->firstSort(); it != _vocabulary->lastSort(); ++it) {
		auto sort = it->second;
		if (not sort->builtin()) {
			if (_sortinter.find(sort) == _sortinter.cend()) {
				auto st = new SortTable(new EnumeratedInternalSortTable());
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
	for (auto funcInterIterator = _funcinter.cbegin(); funcInterIterator != _funcinter.cend(); ++funcInterIterator) {
		FuncInter* fi = (*funcInterIterator).second;
		if (not fi->isConsistent()) {
			return false;
		}
	}
	for (auto predInterIterator = _predinter.cbegin(); predInterIterator != _predinter.cend(); ++predInterIterator) {
		PredInter* pi = (*predInterIterator).second;
		if (not pi->isConsistent()) {
			return false;
		}
	}
	return true;
}

void Structure::makeTwoValued() {
	Assert(isConsistent());
	if (approxTwoValued()) {
		return;
	}
	//clog <<"Before: \n" <<toString(this) <<"\n";
	for (auto i = _funcinter.begin(); i != _funcinter.end(); ++i) {
		CHECKTERMINATION
		auto inter = (*i).second;
		if (inter->approxTwoValued()) {
			continue;
		}
		// create a generator for the interpretation
		auto universe = inter->graphInter()->universe();
		const auto& sorts = universe.tables();

		vector<SortIterator> domainIterators;
		bool allempty = true;
		for (auto sort = sorts.cbegin(); sort != sorts.cend(); ++sort) {
			const auto& temp = SortIterator((*sort)->internTable()->sortBegin());
			domainIterators.push_back(temp);
			if (not temp.isAtEnd()) {
				allempty = false;
			}
		}
		domainIterators.pop_back();

		auto ct = inter->graphInter()->ct();
		auto cf = inter->graphInter()->cf();

		//Now, choose an image for this domainelement
		ElementTuple domainElementWithoutValue;
		if (sorts.size() > 0) {
			auto internaliterator = new CartesianInternalTableIterator(domainIterators, domainIterators, not allempty);
			TableIterator domainIterator(internaliterator);

			auto ctIterator = ct->begin();
			FirstNElementsEqual eq((*i).first->arity());
			StrictWeakNTupleOrdering so((*i).first->arity());

			for (; not allempty && not domainIterator.isAtEnd(); ++domainIterator) {
				CHECKTERMINATION
				// get unassigned domain element
				domainElementWithoutValue = *domainIterator;
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
		} else {
			auto imageIterator = SortIterator(sorts.back()->internTable()->sortBegin());
			for (; not imageIterator.isAtEnd(); ++imageIterator) {
				ElementTuple tuple(domainElementWithoutValue);
				tuple.push_back(*imageIterator);
				if (cf->contains(tuple)) {
					continue;
				}
				inter->graphInter()->makeTrue(tuple);
			}
		}
	}
	for (auto i = _predinter.begin(); i != _predinter.end(); i++) {
		CHECKTERMINATION
		auto inter = (*i).second;
		Assert(inter!=NULL);
		if (inter->approxTwoValued()) {
			continue;
		}

		auto pf = inter->pf();
		for (TableIterator ptIterator = inter->pt()->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION
			if (not pf->contains(*ptIterator)) {
				continue;
			}

			inter->makeFalse(*ptIterator);
		}
	}
	clean();
	//clog <<"After: \n" <<toString(this) <<"\n";
	Assert(approxTwoValued());
}

void computescore(Sort* s, map<Sort*, unsigned int>& scores) {
	if (scores.find(s) == scores.cend()) {
		unsigned int sc = 0;
		for (auto it = s->parents().cbegin(); it != s->parents().cend(); ++it) {
			computescore(*it, scores);
			if (scores[*it] >= sc)
				sc = scores[*it] + 1;
		}
		scores[s] = sc;
	}
}

void completeSortTable(const PredTable* pt, PFSymbol* symbol, const string& structname) {
	if (not pt->approxFinite()) {
		return;
	}
	for (auto jt = pt->begin(); not jt.isAtEnd(); ++jt) {
		const ElementTuple& tuple = *jt;
		for (unsigned int col = 0; col < tuple.size(); ++col) {
			auto sort = symbol->sorts()[col];
			// NOTE: we do not use predicate/function interpretations to autocomplete user provided sorts, this is a bug more often than not
			if (not sort->builtin() && not getGlobal()->getInserter().interpretationSpecifiedByUser(sort)) {
				pt->universe().tables()[col]->add(tuple[col]);
			} else if (!pt->universe().tables()[col]->contains(tuple[col])) {
				if (typeid(*symbol) == typeid(Predicate)) {
					Error::predelnotinsort(toString(tuple[col]), symbol->name(), sort->name(), structname);
				} else {
					Error::funcelnotinsort(toString(tuple[col]), symbol->name(), sort->name(), structname);
				}
			}
		}
	}
}

void addUNAPattern(Function*) {
	throw notyetimplemented("una pattern type");
}

void Structure::autocomplete() {
	// Adding elements from predicate interpretations to sorts
	for (auto it = _predinter.cbegin(); it != _predinter.cend(); ++it) {
		if (it->first->arity() != 1 || it->first->sorts()[0]->pred() != it->first) {
			auto pt1 = it->second->ct();
			if (sametypeid<InverseInternalPredTable>(*(pt1->internTable()))) {
				pt1 = it->second->pf();
			}
			completeSortTable(pt1, it->first, _name);
			if (not it->second->approxTwoValued()) {
				auto pt2 = it->second->cf();
				if (sametypeid<InverseInternalPredTable>(*(pt2->internTable()))) {
					pt2 = it->second->pt();
				}
				completeSortTable(pt2, it->first, _name);
			}
		}
	}
	// Adding elements from function interpretations to sorts
	for (auto it = _funcinter.cbegin(); it != _funcinter.cend(); ++it) {
		if (it->second->funcTable() && sametypeid<UNAInternalFuncTable>(*(it->second->funcTable()->internTable()))) {
			addUNAPattern(it->first);
		} else {
			auto pt1 = it->second->graphInter()->ct();
			if (sametypeid<InverseInternalPredTable>(*(pt1->internTable()))) {
				pt1 = it->second->graphInter()->pf();
			}
			completeSortTable(pt1, it->first, _name);
			if (not it->second->approxTwoValued()) {
				auto pt2 = it->second->graphInter()->cf();
				if (sametypeid<InverseInternalPredTable>(*(pt2->internTable()))) {
					pt2 = it->second->graphInter()->pt();
				}
				completeSortTable(pt2, it->first, _name);
			}
		}
	}

	// Adding elements from subsorts to supersorts
	map<Sort*, unsigned int> scores;
	for (auto it = _vocabulary->firstSort(); it != _vocabulary->lastSort(); ++it) {
		computescore(it->second, scores);
	}
	map<unsigned int, vector<Sort*> > invscores;
	for (auto it = scores.cbegin(); it != scores.cend(); ++it) {
		if (_vocabulary->contains(it->first)) {
			invscores[it->second].push_back(it->first);
		}
	}
	for (auto it = invscores.rbegin(); it != invscores.rend(); ++it) {
		for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
			Sort* s = *jt;
			set<Sort*> notextend = { s };
			vector<Sort*> toextend;
			vector<Sort*> tocheck;
			while (not notextend.empty()) {
				Sort* e = *(notextend.cbegin());
				for (auto kt = e->parents().cbegin(); kt != e->parents().cend(); ++kt) {
					Sort* sp = *kt;
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
			auto st = inter(s);
			for (auto kt = toextend.cbegin(); kt != toextend.cend(); ++kt) {
				auto kst = inter(*kt);
				if (st->approxFinite()) {
					for (auto lt = st->sortBegin(); not lt.isAtEnd(); ++lt) {
						kst->add(*lt);
					}
				} else {
					throw notyetimplemented("Completing non approx-finite tables");
				}
			}
			if (not s->builtin()) {
				for (auto kt = tocheck.cbegin(); kt != tocheck.cend(); ++kt) {
					auto kst = inter(*kt);
					// TODO speedup for common cases (expensive if both tables are large) => should be in some general visitor which checks this!
					if (dynamic_cast<AllIntegers*>(kst->internTable()) != NULL && dynamic_cast<IntRangeInternalSortTable*>(st->internTable()) != NULL) {
						continue;
					}
					if (dynamic_cast<AllNaturalNumbers*>(kst->internTable()) != NULL
							&& dynamic_cast<IntRangeInternalSortTable*>(st->internTable()) != NULL
							&& dynamic_cast<IntRangeInternalSortTable*>(st->internTable())->first()->value()._int > -1) {
						continue;
					}
					if (st->approxFinite()) {
						for (auto lt = st->sortBegin(); not lt.isAtEnd(); ++lt) {
							if (not kst->contains(*lt))
								Error::sortelnotinsort(toString(*lt), s->name(), (*kt)->name(), _name);
						}
					} else {
						throw notyetimplemented("Completing non approx-finite tables");
					}
				}
			}
		}
	}
}

void Structure::addStructure(AbstractStructure*) {
	throw notyetimplemented("Add a structure to another one.");
}

void Structure::sortCheck() const {
	for(auto i=_sortinter.cbegin(); i!=_sortinter.cend(); ++i) {
		if((*i).second->empty()){
			Warning::emptySort(i->first->name());
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

SortTable* Structure::inter(Sort* s) const {
	if (s == NULL) { // TODO prevent error by introducing UnknownSort object (prevent nullpointers)
		throw IdpException("Sort was NULL"); // TODO should become Assert
	}Assert(s != NULL);
	if (s->builtin()) {
		return s->interpretation();
	}

	vector<SortTable*> tables;
	auto list = s->getSortsForTable();
	for (auto i = list.cbegin(); i < list.cend(); ++i) {
		auto it = _sortinter.find(*i);
		Assert(it != _sortinter.cend());
		tables.push_back((*it).second);
	}
	if (tables.size() == 1) {
		return tables.back();
	} else {
		return new SortTable(new UnionInternalSortTable( { }, tables));
	}
}

PredInter* Structure::inter(Predicate* p) const {
	if (p->builtin()) {
		return p->interpretation(this);
	}

	if (p->type() == ST_NONE) {
		auto it = _predinter.find(p);
		if (it == _predinter.cend()) {
			stringstream ss;
			ss <<"The structure does not contain the predicate " <<p->name();
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

FuncInter* Structure::inter(Function* f) const {
	if (f->builtin()) {
		return f->interpretation(this);
	} else {
		auto it = _funcinter.find(f);
		Assert(it != _funcinter.cend());
		return it->second;
	}
}

PredInter* Structure::inter(PFSymbol* s) const {
	if (sametypeid<Predicate>(*s)) {
		return inter(dynamic_cast<Predicate*>(s));
	} else {
		Assert(sametypeid<Function>(*s));
		return inter(dynamic_cast<Function*>(s))->graphInter();
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
