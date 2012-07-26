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

#include "inferences/grounding/GroundTranslator.hpp"

#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "visitors/VisitorFriends.hpp"

#include "utils/ListUtils.hpp"

#include "SolverPolicy.hpp"
#include "PrintGroundPolicy.hpp"
#include "GroundPolicy.hpp"

#include "inferences/SolverInclude.hpp"

using namespace std;

template<class Policy>
GroundTheory<Policy>::GroundTheory(AbstractStructure const * const str)
		: AbstractGroundTheory(str) {
	Policy::polStartTheory(translator());
}

template<class Policy>
GroundTheory<Policy>::GroundTheory(Vocabulary* voc, AbstractStructure const * const str)
		: AbstractGroundTheory(voc, str) {
	Policy::polStartTheory(translator());
}

template<class Policy>
void GroundTheory<Policy>::notifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders){
	Policy::polNotifyUnknBound(context, boundlit, args, grounders);
}

template<class Policy>
void GroundTheory<Policy>::notifyLazyAddition(const litlist& glist, int ID){
	addTseitinInterpretations(glist, getIDForUndefined());
	Policy::polAddLazyAddition(glist, ID);
}

template<class Policy>
void GroundTheory<Policy>::notifyLazyResidual(Lit tseitin, LazyStoredInstantiation* inst, TsType type, bool conjunction){
	Policy::polNotifyLazyResidual(tseitin, inst, type, conjunction);
}

template<class Policy>
void GroundTheory<Policy>::recursiveDelete() {
	//deleteList(_foldedterms); TODO
	Policy::polRecursiveDelete();
	delete (this);
}

template<class Policy>
void GroundTheory<Policy>::closeTheory() {
	if (getOption(IntType::GROUNDVERBOSITY) > 0) {
		clog << "Closing theory, adding functional constraints and symbols defined false.\n";
	}
	addFalseDefineds();
	if (not getOption(BoolType::GROUNDLAZILY)) {
		Policy::polEndTheory();
	}
}

// TODO important: before each add, do transformforadd

template<class Policy>
void GroundTheory<Policy>::add(const GroundClause& cl, bool skipfirst) {
	addTseitinInterpretations(cl, getIDForUndefined(), skipfirst);
	Policy::polAdd(cl);
}

template<class Policy>
void GroundTheory<Policy>::add(const GroundDefinition& def) {
	for (auto i = def.begin(); i != def.end(); ++i) {
		if (isa<PCGroundRule>(*(*i).second)) {
			auto rule = dynamic_cast<PCGroundRule*>((*i).second);
			add(def.id(), rule);
		} else {
			Assert(isa<AggGroundRule>(*(*i).second));
			auto rule = dynamic_cast<AggGroundRule*>((*i).second);
			add(rule->setnr(), def.id(), (rule->aggtype() != AggFunction::CARD));
			Policy::polAdd(def.id(), rule);
			notifyDefined(rule->head());
		}
	}
}

template<class Policy>
void GroundTheory<Policy>::add(DefId defid, PCGroundRule* rule) {
	addTseitinInterpretations(rule->body(), defid);
	Policy::polAdd(defid, rule);
	notifyDefined(rule->head());
}

template<class Policy>
void GroundTheory<Policy>::notifyDefined(Atom inputatom) {
	if (not translator()->isInputAtom(inputatom)) {
		return;
	}
	auto symbol = translator()->getSymbol(inputatom);
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

	body->left(foldCPTerm(body->left()));

	//Add constraint for right hand side if necessary.
	if (body->right()._isvarid && translator()->getFunction(body->right()._varid) == NULL) {
		if (_printedvarids.find(body->right()._varid) == _printedvarids.end()) {
			_printedvarids.insert(body->right()._varid);
			auto cprelation = translator()->cprelation(body->right()._varid);
			auto tseitin2 = translator()->translate(cprelation->left(),cprelation->comp(),cprelation->right(),cprelation->type());
			addUnitClause(tseitin2);
		}
	}
	Policy::polAdd(tseitin, body);
}

template<class Policy>
void GroundTheory<Policy>::add(SetId setnr, DefId defnr, bool weighted) {
	if (_printedsets.find(setnr) != _printedsets.end()) {
		return;
	}
	_printedsets.insert(setnr);
	auto tsset = translator()->groundset(setnr);
	addTseitinInterpretations(tsset.literals(), defnr);
	Policy::polAdd(tsset, setnr, weighted);
}

template<class Policy>
void GroundTheory<Policy>::add(Lit head, AggTsBody* body) {
	add(body->setnr(), getIDForUndefined(), (body->aggtype() != AggFunction::CARD));
	Policy::polAdd(head, body);
}

template<class Policy>
void GroundTheory<Policy>::add(const Lit& head, TsType type, const litlist& body, bool conj, DefId defnr) {
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
}

template<class Policy>
void GroundTheory<Policy>::addOptimization(AggFunction function, SetId setid) {
	add(setid, getIDForUndefined(), function!=AggFunction::CARD);
	Policy::polAddOptimization(function, setid);
}

template<class Policy>
void GroundTheory<Policy>::addOptimization(VarId varid) {
	//Add reified constraint necessary. TODO refactor
	if (translator()->getFunction(varid) == NULL) {
		if (_printedvarids.find(varid) == _printedvarids.end()) {
			_printedvarids.insert(varid);
			auto cprelation = translator()->cprelation(varid);
			auto tseitin = translator()->translate(cprelation->left(), cprelation->comp(), cprelation->right(), cprelation->type());
			addUnitClause(tseitin);
		}
	}
	Policy::polAddOptimization(varid);
}

template<class Policy>
void GroundTheory<Policy>::addSymmetries(const std::vector<std::map<Lit, Lit> >& symmetry){
	Policy::polAdd(symmetry);
}

template<class Policy>
std::ostream& GroundTheory<Policy>::put(std::ostream& s) const {
	return Policy::polPut(s, translator());
}

template<class Policy>
void GroundTheory<Policy>::addTseitinInterpretations(const std::vector<int>& vi, DefId defnr, bool skipfirst) {
	size_t n = 0;
	if (skipfirst) {
		++n;
	}
	for (; n < vi.size(); ++n) {
		int tseitin = abs(vi[n]);
		// NOTE: checks whether the tseitin has already been added to the grounding
		if (not translator()->isTseitinWithSubformula(tseitin) || _printedtseitins.find(tseitin) != _printedtseitins.end()) {
			//clog <<"Tseitin" <<atom <<" already grounded" <<nt();
			continue;
		}
		//clog <<"Adding tseitin" <<atom <<" to grounding" <<nt();
		_printedtseitins.insert(tseitin);
		auto tsbody = translator()->getTsBody(tseitin);
		if (isa<PCTsBody>(*tsbody)) {
			auto body = dynamic_cast<PCTsBody*>(tsbody);
			add(tseitin, body->type(), body->body(), body->conj(), defnr);
		} else if (isa<AggTsBody>(*tsbody)) {
			AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
			if (body->type() == TsType::RULE) {
				Assert(defnr != getIDForUndefined());
				add(body->setnr(), defnr, (body->aggtype() != AggFunction::CARD));
				Policy::polAdd(defnr, new AggGroundRule(tseitin, body, true)); //TODO true (recursive) might not always be the case?
			} else {
				add(tseitin, body);
			}
		} else if (isa<CPTsBody>(*tsbody)) {
			auto body = dynamic_cast<CPTsBody*>(tsbody);
			Assert(body->type() != TsType::RULE);
			add(tseitin, body);
		} else {
			Assert(isa<LazyTsBody>(*tsbody));
			auto body = dynamic_cast<LazyTsBody*>(tsbody);
			body->notifyTheoryOccurence(tseitin);
		}
	}
}

template<class Policy>
CPTerm* GroundTheory<Policy>::foldCPTerm(CPTerm* cpterm) {
	if (_foldedterms.find(cpterm) != _foldedterms.end()) {
		return cpterm;
	}
	_foldedterms.insert(cpterm);

	if (isa<CPVarTerm>(*cpterm)) {
		auto varterm = dynamic_cast<CPVarTerm*>(cpterm);
		if (translator()->getFunction(varterm->varid()) == NULL) {
			CPTsBody* cprelation = translator()->cprelation(varterm->varid());
			CPTerm* left = foldCPTerm(cprelation->left());
			if ((isa<CPSumTerm>(*left) or isa<CPWSumTerm>(*left)) and cprelation->comp() == CompType::EQ) {
				Assert(cprelation->right()._isvarid and cprelation->right()._varid == varterm->varid());
				return left;
			}
		}
	} else if (isa<CPWSumTerm>(*cpterm)) {
		auto sumterm = dynamic_cast<CPWSumTerm*>(cpterm);
		varidlist newvarids;
		intweightlist newweights;
		//for (auto vit = sumterm->varids().begin(), auto wit = sumterm->weights().begin(); vit != sumterm->varids().end(); ++vit, ++wit) {
		auto vit = sumterm->varids().begin();
		auto wit = sumterm->weights().begin();
		for (; vit != sumterm->varids().end(); ++vit, ++wit) {
			if (translator()->getFunction(*vit) == NULL) {
				CPTsBody* cprelation = translator()->cprelation(*vit);
				CPTerm* left = foldCPTerm(cprelation->left());
				if (isa<CPWSumTerm>(*left) && cprelation->comp() == CompType::EQ) {
					CPWSumTerm* subterm = static_cast<CPWSumTerm*>(left);
					Assert(cprelation->right()._isvarid && cprelation->right()._varid == *vit);
					newvarids.insert(newvarids.end(), subterm->varids().begin(), subterm->varids().end());
					for (auto it = subterm->weights().begin(); it != subterm->weights().end(); ++it) {
						newweights.push_back((*it) * (*wit));
					}
				} else if (isa<CPSumTerm>(*left) && cprelation->comp() == CompType::EQ) {
//					CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
//					Assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
					Assert(false); //FIXME Remove CPSumTerm from code entirely => always use CPWSumTerm!
				} else { //TODO Need to do something special in other cases?
					newvarids.push_back(*vit);
					newweights.push_back(*wit);
				}
			} else {
				newvarids.push_back(*vit);
				newweights.push_back(*wit);
			}
		}
		sumterm->varids(newvarids);
		sumterm->weights(newweights);
		return sumterm;
	} else if (isa<CPSumTerm>(*cpterm)) {
		Assert(false); //FIXME Remove CPSumTerm from code entirely => always use CPWSumTerm!
	}
	return cpterm;
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
	if(verbosity()>1){
		clog <<"Closing definition by asserting literals false which have no rule making them true.\n";
	}
	for (auto sit=getNeedFalseDefinedSymbols().cbegin(); sit!=getNeedFalseDefinedSymbols().cend(); ++sit) {
		CHECKTERMINATION
		auto pt = structure()->inter(*sit)->pt();
		auto it = _defined.find(*sit);
	//	cerr <<"Already grounded for " <<toString(*sit) <<"\n";
	//	for(auto i=_defined.cbegin(); i!=_defined.cend(); ++i){
	//		cerr <<toString(i->second) <<"\n";
	//	}
		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION
			auto translation = translator()->translate(*sit, (*ptIterator));
			if (it==_defined.cend() || it->second.find(translation) == it->second.cend()) {
				addUnitClause(-translation);
				// TODO better solution would be to make the structure more precise
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

// Explicit instantiations
template class GroundTheory<GroundPolicy> ;
template class GroundTheory<SolverPolicy<PCSolver> > ;
template class GroundTheory<PrintGroundPolicy> ;
