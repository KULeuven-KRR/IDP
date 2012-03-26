/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GroundTheory.hpp"

#include "IncludeComponents.hpp"
#include "AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTermTranslator.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "visitors/VisitorFriends.hpp"

#include "utils/ListUtils.hpp"

#include "SolverPolicy.hpp"
#include "PrintGroundPolicy.hpp"
#include "GroundPolicy.hpp"

using namespace std;

template<class Policy>
GroundTheory<Policy>::GroundTheory(AbstractStructure* str)
		: AbstractGroundTheory(str) {
	Policy::polStartTheory(translator());
}

template<class Policy>
GroundTheory<Policy>::GroundTheory(Vocabulary* voc, AbstractStructure* str)
		: AbstractGroundTheory(voc, str) {
	Policy::polStartTheory(translator());
}

template<class Policy>
void GroundTheory<Policy>::notifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders){
	Policy::polNotifyUnknBound(context, boundlit, args, grounders);
}

template<class Policy>
void GroundTheory<Policy>::notifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder){
	Policy::polNotifyLazyResidual(inst, type, grounder);
}

template<class Policy>
void GroundTheory<Policy>::recursiveDelete() {
	//deleteList(_foldedterms);
	Policy::polRecursiveDelete();
	delete (this);
}

template<class Policy>
void GroundTheory<Policy>::closeTheory() {
	if(getOption(IntType::GROUNDVERBOSITY)>0){
		clog <<"Closing theory, adding functional constraints and symbols defined false.\n";
	}
	// TODO arbitrary values?
	// FIXME problem if a function does not occur in the theory/grounding! It might be arbitrary, but should still be a function?
	addFalseDefineds();
	if(not getOption(BoolType::GROUNDLAZILY)){
		addFuncConstraints();
		Policy::polEndTheory();
	}
}

// TODO important: before each add, do transformforadd, after each do addfuncconstraints!

template<class Policy>
void GroundTheory<Policy>::add(const GroundClause& cl, bool skipfirst) {
	transformForAdd(cl, VIT_DISJ, getIDForUndefined(), skipfirst);
	Policy::polAdd(cl);
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::add(const GroundDefinition& def) {
	for (auto i = def.begin(); i != def.end(); ++i) {
		if (sametypeid<PCGroundRule>(*(*i).second)) {
			auto rule = dynamic_cast<PCGroundRule*>((*i).second);
			add(def.id(), rule);
		} else {
			Assert(sametypeid<AggGroundRule>(*(*i).second));
			auto rule = dynamic_cast<AggGroundRule*>((*i).second);
			add(rule->setnr(), def.id(), (rule->aggtype() != AggFunction::CARD));
			Policy::polAdd(def.id(), rule);
			notifyDefined(rule->head());
		}
	}
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::add(DefId defid, PCGroundRule* rule) {
	transformForAdd(rule->body(), (rule->type() == RuleType::CONJ ? VIT_CONJ : VIT_DISJ), defid);
	Policy::polAdd(defid, rule);
	notifyDefined(rule->head());
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::notifyDefined(Atom inputatom) {
	if (not translator()->isInputAtom(inputatom)) {
		return;
	}
	PFSymbol* symbol = translator()->getSymbol(inputatom);
	auto it = _defined.find(symbol);
	if (it == _defined.end()) {
		it = _defined.insert(std::pair<PFSymbol*, std::set<Atom>> { symbol, std::set<Atom>() }).first;
	}
	(*it).second.insert(inputatom);
}

template<class Policy>
void GroundTheory<Policy>::add(GroundFixpDef*) {
	Assert(false);
	//TODO
}

template<class Policy>
void GroundTheory<Policy>::add(Lit tseitin, CPTsBody* body) {
	//TODO also add variables (in a separate container?)

	auto foldedbody = new CPTsBody(body->type(), foldCPTerm(body->left()), body->comp(), body->right());
	//FIXME possible leaks!!

	//TODO refactor the following...
	if (body->right()._isvarid && termtranslator()->function(body->right()._varid) == NULL) {
		if (_printedvarids.find(body->right()._varid) == _printedvarids.end()) {
			_printedvarids.insert(body->right()._varid);
			auto cprelation = termtranslator()->cprelation(body->right()._varid);
			auto tseitin2 = translator()->translate(cprelation->left(),cprelation->comp(),cprelation->right(),TsType::EQ);
			addUnitClause(tseitin2);
		}
	}

	Policy::polAdd(tseitin, foldedbody);
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::add(SetId setnr, DefId defnr, bool weighted) {
	if (_printedsets.find(setnr) != _printedsets.end()) {
		return;
	}
	_printedsets.insert(setnr);
	auto tsset = translator()->groundset(setnr);
	transformForAdd(tsset.literals(), VIT_SET, defnr);
	//weightlist weights;
	//if (weighted) {
	//	weights = tsset.weights();
	//}
	Policy::polAdd(tsset, setnr, weighted);
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::add(Lit head, AggTsBody* body) {
	add(body->setnr(), getIDForUndefined(), (body->aggtype() != AggFunction::CARD));
	Policy::polAdd(head, body);
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::add(const Lit& head, TsType type, const litlist& body, bool conj, int defnr) {
	if (type == TsType::IMPL || type == TsType::EQ) {
		if (conj) {
			for (auto i = body.cbegin(); i < body.cend(); ++i) {
				add( { -head, *i }, true);
			}
		} else {
			litlist cl(body.size() + 1, -head);
			for (size_t i = 0; i < body.size(); ++i) {
				cl[i + 1] = body[i];
			}
			add(cl, true);
		}
	}
	if (type == TsType::RIMPL || type == TsType::EQ) {
		if (conj) {
			litlist cl(body.size() + 1, head);
			for (size_t i = 0; i < body.size(); ++i) {
				cl[i + 1] = -body[i];
			}
			add(cl, true);
		} else {
			for (auto i = body.cbegin(); i < body.cend(); ++i) {
				add( { head, -*i }, true);
			}
		}
	}
	if (type == TsType::RULE) {
		// FIXME when doing this lazily, the rule should not be here until the tseitin has a value!
		Assert(defnr != getIDForUndefined());
		add(defnr, new PCGroundRule(head, conj ? RuleType::CONJ : RuleType::DISJ, body, true)); //TODO true (recursive) might not always be the case?
	}
	addFuncConstraints();
}

template<class Policy>
void GroundTheory<Policy>::addOptimization(AggFunction function, int setid){
	add(setid, getIDForUndefined(), function!=AggFunction::CARD);
	Policy::polAddOptimization(function, setid);
}

template<class Policy>
std::ostream& GroundTheory<Policy>::put(std::ostream& s) const {
	return Policy::polPut(s, translator(), termtranslator());
}

template<class Policy>
void GroundTheory<Policy>::transformForAdd(const std::vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst) {
	size_t n = 0;
	if (skipfirst) {
		++n;
	}
	for (; n < vi.size(); ++n) {
		int atom = abs(vi[n]);
		// NOTE: checks whether the tseitin has already been added to the grounding
		if (not translator()->isTseitinWithSubformula(atom) || _printedtseitins.find(atom) != _printedtseitins.end()) {
			//clog <<"Tseitin" <<atom <<" already grounded" <<nt();
			continue;
		}
		//clog <<"Adding tseitin" <<atom <<" to grounding" <<nt();
		_printedtseitins.insert(atom);
		auto tsbody = translator()->getTsBody(atom);
		if (sametypeid<PCTsBody>(*tsbody)) {
			auto body = dynamic_cast<PCTsBody*>(tsbody);
			add(atom, body->type(), body->body(), body->conj(), defnr);
		} else if (sametypeid<AggTsBody>(*tsbody)) {
			AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
			if (body->type() == TsType::RULE) {
				Assert(defnr != getIDForUndefined());
				add(body->setnr(), defnr, (body->aggtype() != AggFunction::CARD));
				Policy::polAdd(defnr, new AggGroundRule(atom, body, true)); //TODO true (recursive) might not always be the case?
			} else {
				add(atom, body);
			}
		} else if (sametypeid<CPTsBody>(*tsbody)) {
			CPTsBody* body = dynamic_cast<CPTsBody*>(tsbody);
			if (body->type() == TsType::RULE) {
				notyetimplemented("Definition rules in CP constraints.");
				//TODO Does this ever happen?
			} else {
				add(atom, body);
			}
		} else {
			Assert(sametypeid<LazyTsBody>(*tsbody));
			auto body = dynamic_cast<LazyTsBody*>(tsbody);
			body->notifyTheoryOccurence();
		}
	}
}

template<class Policy>
CPTerm* GroundTheory<Policy>::foldCPTerm(CPTerm* cpterm) {
	if (_foldedterms.find(cpterm) != _foldedterms.end()) {
		return cpterm;
	}
	_foldedterms.insert(cpterm);

	if (sametypeid<CPVarTerm>(*cpterm)) {
		auto varterm = dynamic_cast<CPVarTerm*>(cpterm);
		if (termtranslator()->function(varterm->varid()) == NULL) {
			CPTsBody* cprelation = termtranslator()->cprelation(varterm->varid());
			CPTerm* left = foldCPTerm(cprelation->left());
			if ((sametypeid<CPSumTerm>(*left) || sametypeid<CPWSumTerm>(*left)) && cprelation->comp() == CompType::EQ) {
				Assert(cprelation->right()._isvarid && cprelation->right()._varid == varterm->varid());
				return left;
			}
		}
	} else if (sametypeid<CPSumTerm>(*cpterm)) {
		auto sumterm = dynamic_cast<CPSumTerm*>(cpterm);
		std::vector<VarId> newvarids;
		for (auto it = sumterm->varids().begin(); it != sumterm->varids().end(); ++it) {
			if (termtranslator()->function(*it) == NULL) {
				CPTsBody* cprelation = termtranslator()->cprelation(*it);
				CPTerm* left = foldCPTerm(cprelation->left());
				if (sametypeid<CPSumTerm>(*left) && cprelation->comp() == CompType::EQ) {
					CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
					Assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
					newvarids.insert(newvarids.end(), subterm->varids().begin(), subterm->varids().end());
				} else { //TODO Need to do something special in other cases?
					newvarids.push_back(*it);
				}
			} else {
				newvarids.push_back(*it);
			}
		}
		sumterm->varids(newvarids);
	} else if (sametypeid<CPWSumTerm>(*cpterm)) {
		//CPWSumTerm* wsumterm = static_cast<CPWSumTerm*>(cpterm);
		//TODO Folding for weighted sumterms
	}
	return cpterm;
}

/**
 *	Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
 *	This method should be called before running the SAT solver and after grounding.
 */
template<class Policy>
void GroundTheory<Policy>::addFuncConstraints() {
	while(translator()->hasNewSymbols()){
		CHECKTERMINATION
		auto n = translator()->getNextNewSymbol();
		auto pfs = translator()->getManagedSymbol(n);
		if (not sametypeid<Function>(*pfs)) {
			continue;
		}
		auto f = dynamic_cast<Function*>(pfs);

		FirstNElementsEqual equalDomain(f->arity());
		StrictWeakNTupleOrdering tuplesFirstNSmaller(f->arity());

		auto ct = structure()->inter(f)->graphInter()->ct();
		auto pt = structure()->inter(f)->graphInter()->pt();
		auto outSortTable = structure()->inter(f->outsort());
		if (not pt->finite()) {
			throw notyetimplemented("Cannot ground functions with infinite sorts.");
			//FIXME make this work for functions with infintie domains and/or infinite out-sorts.  Take a look at lower code...
		}

		ElementTuple domainElement, certainly;
		TableIterator ctIterator = ct->begin();
		bool begin = true, newdomain = true, testcertainly = false;
		litlist tupleset; // Set of tuples with same domain but different range, for which the cardinality should be 1
		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION
			ElementTuple current((*ptIterator));

			if (begin) {
				domainElement = current;
				begin = false;
			}

			if (not equalDomain(current, domainElement)) {
				newdomain = true;
				if (not testcertainly) {
					addRangeConstraint(f, tupleset, outSortTable);
				}
				tupleset.clear();
			}

			if (newdomain) {
				domainElement = current;
				newdomain = false;

				// CERTAINLY TRUE OPTIMIZATION / PROPAGATION: if some in the domain is certainly true, assert all other ones false
				while (not ctIterator.isAtEnd() && tuplesFirstNSmaller(*ctIterator, current)) {
					CHECKTERMINATION
					++ctIterator;
				}
				if (not ctIterator.isAtEnd() && equalDomain(*ctIterator, current)) {
					certainly = *ctIterator;
					testcertainly = true;
				} else {
					testcertainly = false;
				}
			}

			if (testcertainly && equalDomain(certainly, current)) {
				if (current != certainly) { // Assert current false
					Lit translation = translator()->translate(n, current);
					addUnitClause(-translation);
				}
				continue;
			} else {
				Lit translation = translator()->translate(n, current);
				tupleset.push_back(translation);
			}
		}
		if (not testcertainly) {
			addRangeConstraint(f, tupleset, outSortTable);
		}

		//OLD CODE that might work for infinite domains... First we should find out what exactly is the meaning of the grounding in case of infinite domains...
		//FIXME implement for infinite domains
		/*std::vector<bool> weak;
		 for (auto it = tuples.begin(); it != tuples.end();) {
		 //NOTE: DE checks whether or not a tuple (x1,x2,x3,x4,y) starts with (x1,x2,x3,x4)
		 //IMPORTANT: tableiterator respects lexicographic ordering!!!

		 if (tuplesFirstNEqual(it->first, domainElement) && !sets.empty()) {
		 sets.back().push_back(it->second);
		 while (*outSortIterator != it->first.back()) {
		 ElementTuple temp = domainElement;
		 temp.push_back(*outSortIterator);
		 if (pt->contains(temp)) {
		 weak.back() = true;
		 break;
		 }
		 ++outSortIterator;
		 }
		 ++it;
		 if (not outSortIterator.isAtEnd()) {
		 ++outSortIterator;
		 }
		 } else {
		 if (not sets.empty() && not outSortIterator.isAtEnd()) {
		 weak.back() = true;
		 }
		 if (not ctIterator.isAtEnd()) {
		 const ElementTuple& tuple = *ctIterator;
		 if (tuplesFirstNEqual(tuple, it->first)) {
		 do {
		 if (it->first != tuple) {
		 addUnitClause(-(it->second));
		 std::clog << "add unit clause " << -(it->second) << "\n";
		 }
		 ++it;
		 } while (it != tuples.end() && tuplesFirstNEqual(tuple, it->first));
		 continue;
		 } else if (tuplesFirstNSmaller(tuple, it->first)) {
		 do {
		 ++ctIterator;
		 } while (not ctIterator.isAtEnd() && tuplesFirstNSmaller(*ctIterator, it->first));
		 continue;
		 }
		 }
		 sets.push_back(std::vector<int>(0));
		 weak.push_back(false);
		 domainElement = it->first;
		 domainElement.pop_back();
		 outSortIterator = outSortTable->sortBegin();
		 }
		 }*/
	}
}

template<class Policy>
void GroundTheory<Policy>::addFalseDefineds() {
	/*
	 * FIXME FIXME HACKED!
	 * There is an issue that, if all instantiations of a symbol are false, none is ever added to the translator
	 * In that case, it is not managing that symbol, so will not write falsedefineds for it.
	 * This was solved by a workaround in DefinitionGrounder which adds all its head symbols to the translator. This is not maintainable or clear!
	 * It also works lazily because when delaying, the symbol is also added to the translator
	 * So should probably redefine the notion of managedsymbol as any symbol occurring in one of the grounders?
	 */
	for (auto sit=getNeedFalseDefinedSymbols().cbegin(); sit!=getNeedFalseDefinedSymbols().cend(); ++sit) {
		CHECKTERMINATION
		auto pt = structure()->inter(*sit)->pt();
		auto it = _defined.find(*sit);
		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION
			auto translation = translator()->translate(*sit, (*ptIterator));
			if (it==_defined.cend() || it->second.find(translation) == it->second.cend()) {
				addUnitClause(-translation);
				// TODO if not in translator, should make the structure more precise (do not add it to the grounding, that is useless)
			}
		}
	}
}

template<class Policy>
void GroundTheory<Policy>::addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable) {
	CHECKTERMINATION
	weightlist lw(set.size(), 1);
	SetId setnr = translator()->translateSet(set, lw, { }, { });
	Lit tseitin;
	if (f->partial() || (not outSortTable->finite())) {
		tseitin = translator()->translate(1, CompType::GEQ, AggFunction::CARD, setnr, TsType::IMPL);
	} else {
		tseitin = translator()->translate(1, CompType::EQ, AggFunction::CARD, setnr, TsType::IMPL);
	}
	addUnitClause(tseitin);
}

template<class Policy>
void GroundTheory<Policy>::accept(TheoryVisitor* v) const {
	v->visit(this);
}

template<class Policy>
AbstractTheory* GroundTheory<Policy>::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

#include "external/FlatZincRewriter.hpp"
#include "external/ExternalInterface.hpp"

// Explicit instantiations
template class GroundTheory<GroundPolicy> ;
template class GroundTheory<SolverPolicy<MinisatID::WrappedPCSolver> > ;
template class GroundTheory<SolverPolicy<MinisatID::FlatZincRewriter> > ;
template class GroundTheory<PrintGroundPolicy> ;
