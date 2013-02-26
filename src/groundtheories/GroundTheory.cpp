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

#include "GroundTheory.hpp"

#include "IncludeComponents.hpp"
#include "AbstractGroundTheory.hpp"
#include "inferences/SolverInclude.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "visitors/VisitorFriends.hpp"
#include "utils/ListUtils.hpp"
#include "SolverPolicy.hpp"
#include "PrintGroundPolicy.hpp"
#include "GroundPolicy.hpp"

// IMPORTANT IMPORTANT: before each add, do addTseitinInterpretations or addFoldedVarEquiv for all relevant literals/variables!

using namespace std;

template<class Policy>
GroundTheory<Policy>::GroundTheory(StructureInfo info, bool nbModelsEquivalent)
		: AbstractGroundTheory(info),
		  _nbModelsEquivalent(nbModelsEquivalent),
		  addingTseitins(false) {
	Policy::polStartTheory(translator());
}

template<class Policy>
GroundTheory<Policy>::GroundTheory(Vocabulary* voc, StructureInfo info, bool nbModelsEquivalent)
		: AbstractGroundTheory(voc, info),
		  _nbModelsEquivalent(nbModelsEquivalent),
		  addingTseitins(false) {
	Policy::polStartTheory(translator());
}

template<class Policy>
void GroundTheory<Policy>::notifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders) {
	Policy::polNotifyUnknBound(context, boundlit, args, grounders);
}

template<class Policy>
void GroundTheory<Policy>::notifyLazyAddition(const litlist& glist, int ID) {
	addTseitinInterpretations(glist, getIDForUndefined());
	Policy::polAddLazyAddition(glist, ID);
}

template<class Policy>
void GroundTheory<Policy>::startLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction) {
	Policy::polStartLazyFormula(inst, type, conjunction);
}
template<class Policy>
void GroundTheory<Policy>::notifyLazyResidual(LazyInstantiation* inst, TsType type) {
	Policy::polNotifyLazyResidual(inst, type);
}

template<class Policy>
void GroundTheory<Policy>::addLazyElement(Lit head, PFSymbol* symbol, const std::vector<VarId>& args) {
	for(auto arg:args){
		addFoldedVarEquiv(arg);
	}
	Policy::polAddLazyElement(head, symbol, args, this);
}

template<class Policy>
void GroundTheory<Policy>::recursiveDelete() {
	Policy::polRecursiveDelete();
	delete (this);
}

template<class Policy>
void GroundTheory<Policy>::closeTheory() {
	if (not getOption(BoolType::GROUNDLAZILY)) {
		Policy::polEndTheory();
	}
}

template<class Policy>
void GroundTheory<Policy>::add(const GroundClause& cl, bool skipfirst) {
#ifdef DEBUG
	for (auto lit : cl) {
		Assert(lit!=_true);
		Assert(lit!=_false);
	}
#endif
	bool propagates = cl.size()==1;
	// If propagates is true, it will have been added to the grounding if necessary by addTseitinInterpretations
	if(not propagates){
		Policy::polAdd(cl);
	}
	addTseitinInterpretations(cl, getIDForUndefined(), skipfirst, propagates);
}

template<class Policy>
void GroundTheory<Policy>::add(const GroundDefinition& def) {
	for (auto head2rule : def) {
		addTseitinInterpretations({head2rule.first}, def.id());
		auto rule = head2rule.second;
		if (isa<PCGroundRule>(*rule)) {
			add(def.id(), dynamic_cast<PCGroundRule*>(rule));
		} else {
			Assert(isa<AggGroundRule>(*rule));
			auto aggrule = dynamic_cast<AggGroundRule*>(rule);
			add(aggrule->setnr(), def.id(), (aggrule->aggtype() != AggFunction::CARD));
			Policy::polAdd(def.id(), aggrule);
		}
	}
}

template<class Policy>
void GroundTheory<Policy>::add(DefId defid, PCGroundRule* rule) {
	Assert(defid!=getIDForUndefined());
	addTseitinInterpretations({rule->head()}, defid);
	addTseitinInterpretations(rule->body(), defid);
	Policy::polAdd(defid, rule);
}

template<class Policy>
void GroundTheory<Policy>::add(GroundFixpDef*) {
	throw notyetimplemented("Adding ground fixpoint definitions to a groundtheory.");
}

template<class Policy>
void GroundTheory<Policy>::addFoldedVarEquiv(VarId id) {
	if (_printedvarids.find(id) != _printedvarids.end()) {
		return;
	}
	_printedvarids.insert(id);

	// It has an internal meaning, add it here:
	if (translator()->cprelation(id) == NULL) {
		return;
	}
	auto cprelation = translator()->cprelation(id);
	auto tseitin2 = translator()->reify(cprelation->left(), cprelation->comp(), cprelation->right(), cprelation->type());
	addUnitClause(tseitin2);
}

template<class Policy>
void GroundTheory<Policy>::addVarIdInterpretation(VarId id){
	if (_addedvarinterpretation.find(id) != _addedvarinterpretation.cend()) {
		return;
	}
	_addedvarinterpretation.insert(id);

	// It is already partially known:
	translator()->addKnown(id);
}

template<class Policy>
void GroundTheory<Policy>::add(Lit tseitin, CPTsBody* body) {
	body->left(foldCPTerm(body->left()));

	for(auto var: body->left()->getVarIds()){
		addVarIdInterpretation(var);
	}

	if (body->right()._isvarid) {
		auto id = body->right()._varid;
		addVarIdInterpretation(id);
		addFoldedVarEquiv(id);
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
	add(setid, getIDForUndefined(), function != AggFunction::CARD);
	Policy::polAddOptimization(function, setid);
}

template<class Policy>
void GroundTheory<Policy>::addOptimization(VarId varid) {
	addVarIdInterpretation(varid);
	addFoldedVarEquiv(varid);
	Policy::polAddOptimization(varid);
}

template<class Policy>
void GroundTheory<Policy>::addSymmetries(const std::vector<std::map<Lit, Lit> >& symmetry) {
	Policy::polAdd(symmetry);
}

template<class Policy>
std::ostream& GroundTheory<Policy>::put(std::ostream& s) const {
	return Policy::polPut(s, translator());
}

template<class Policy>
void GroundTheory<Policy>::addTseitinInterpretations(const std::vector<int>& vi, DefId defnr, bool skipfirst, bool propagated) {
	for(uint i=0; i<vi.size(); ++i){
		if(i==0 && skipfirst){
			continue;
		}
		tseitinqueue.push(TseitinInfo{vi[i], defnr, propagated});
	}
	if(addingTseitins){
		return;
	}
	addingTseitins = true;
	while(not tseitinqueue.empty()){
		auto elem = tseitinqueue.front();
		auto tseitin = abs(elem.lit);
		auto tseitindefnr = elem.defid;
		auto atroot = elem.rootlevel;
		tseitinqueue.pop();

		if (not translator()->isTseitinWithSubformula(tseitin) || _printedtseitins.find(tseitin) != _printedtseitins.end()) {
			if(atroot){
				Policy::polAdd(GroundClause{elem.lit});
			}
			continue;
		}

		auto eliminated = false;
		auto tsbody = translator()->getTsBody(tseitin);
		if (isa<PCTsBody>(*tsbody)) {
			auto body = dynamic_cast<PCTsBody*>(tsbody);

			if(atroot && body->type()!=TsType::RULE){
				/*
				 * if tseitin at root level has to be true: for EQ and IMPL, add body as (set of) sentence(s)
				 * 											for RIMPL, skip
				 * if tseitin at root has to be false: for IMPL: skip
				 * 										for EQ and RIMPL: add negation of body at root
				 */
				if(elem.lit > 0 && body->type()!=TsType::RIMPL){
					eliminated = true;
					if(body->conj()){
						for(auto lit: body->body()){
							add({lit});
						}
					}else{
						add(body->body());
					}
				}
				if(elem.lit < 0 && body->type()!=TsType::IMPL){
					eliminated = true;
					if(body->conj()){
						litlist lits;
						for(auto lit: body->body()){
							lits.push_back(-lit);
						}
						add(lits);
					}else{
						for(auto lit: body->body()){
							add({-lit});
						}
					}
				}
			}else{
				add(tseitin, body->type(), body->body(), body->conj(), tseitindefnr);
				if(body->type()==TsType::RULE && useUFSAndOnlyIfSem() && _nbModelsEquivalent){
					add(tseitin, TsType::RIMPL, body->body(), body->conj(), tseitindefnr);
				}
			}
		} else if (isa<AggTsBody>(*tsbody)) {
			auto body = dynamic_cast<AggTsBody*>(tsbody);
			if (body->type() == TsType::RULE) {
				Assert(tseitindefnr != getIDForUndefined());
				add(body->setnr(), tseitindefnr, (body->aggtype() != AggFunction::CARD));
				Policy::polAdd(tseitindefnr, new AggGroundRule(tseitin, body, true)); //TODO true (recursive) might not always be the case?
				if(useUFSAndOnlyIfSem() && _nbModelsEquivalent){
					body->type(TsType::RIMPL);
					add(tseitin, body);
				}
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
			body->notifyTheoryOccurence();
		}

		if(atroot && not eliminated){
			Policy::polAdd(GroundClause{elem.lit});
		}

		if(not eliminated){
			_printedtseitins.insert(tseitin);
			translator()->removeTsBody(tseitin);
		}
	}
	addingTseitins = false;
}

template<class Policy>
CPTerm* GroundTheory<Policy>::foldCPTerm(CPTerm* cpterm) {
	if (_foldedterms.find(cpterm) != _foldedterms.end()) {
		return cpterm;
	}
	_foldedterms.insert(cpterm);

	if (isa<CPVarTerm>(*cpterm)) {
		auto varterm = dynamic_cast<CPVarTerm*>(cpterm);
		if (translator()->cprelation(varterm->varid()) == NULL) {
			return cpterm;
		}
		auto cprelation = translator()->cprelation(varterm->varid());
		auto left = foldCPTerm(cprelation->left());
		if (isa<CPWSumTerm>(*left) and cprelation->comp() == CompType::EQ) {
			Assert(cprelation->right()._isvarid and cprelation->right()._varid == varterm->varid());
			return left;
		} else if (isa<CPWProdTerm>(*left) and cprelation->comp() == CompType::EQ) {
			Assert(cprelation->right()._isvarid and cprelation->right()._varid == varterm->varid());
			return left;
		}
	} else if (isa<CPWSumTerm>(*cpterm)) {
		auto sumterm = dynamic_cast<CPWSumTerm*>(cpterm);
		varidlist newvarids;
		intweightlist newweights;
		auto vit = sumterm->varids().begin();
		auto wit = sumterm->weights().begin();
		for (; vit != sumterm->varids().end(); ++vit, ++wit) {
			if (translator()->cprelation(*vit) != NULL) {
				CPTsBody* cprelation = translator()->cprelation(*vit);
				CPTerm* left = foldCPTerm(cprelation->left());
				if (isa<CPWSumTerm>(*left) and cprelation->comp() == CompType::EQ) {
					CPWSumTerm* subterm = static_cast<CPWSumTerm*>(left);
					Assert(cprelation->right()._isvarid and cprelation->right()._varid == *vit);
					insertAtEnd(newvarids, subterm->varids());
					for (auto it = subterm->weights().begin(); it != subterm->weights().end(); ++it) {
						newweights.push_back((*it) * (*wit));
					}
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
	} else {
		//TODO
		Assert(isa<CPWProdTerm>(*cpterm));
		auto prodterm = dynamic_cast<CPWProdTerm*>(cpterm);
		varidlist newvarids;
		int newweight = prodterm->weight();
		for (auto vit = prodterm->varids().begin(); vit != prodterm->varids().end(); ++vit) {
			if (translator()->cprelation(*vit) != NULL) {
				auto cprelation = translator()->cprelation(*vit);
				auto left = foldCPTerm(cprelation->left());
				if (isa<CPWProdTerm>(*left) and cprelation->comp() == CompType::EQ) {
					auto subterm = static_cast<CPWProdTerm*>(left);
					Assert(cprelation->right()._isvarid and cprelation->right()._varid == *vit);
					insertAtEnd(newvarids, subterm->varids());
					newweight *= subterm->weight();
				} else { //TODO Need to do something special in other cases?
					newvarids.push_back(*vit);
					//Note: weight doesn't change
				}
			} else {
				newvarids.push_back(*vit);
				//Note: weight doesn't change
			}
		}
		prodterm->varids(newvarids);
		prodterm->weight(newweight);
		return prodterm;
	}
	return cpterm;
}

template<class Policy>
void GroundTheory<Policy>::addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable) {
	CHECKTERMINATION;
	weightlist lw(set.size(), 1);
	SetId setnr = translator()->translateSet(set, lw, {}, {});
	Lit tseitin;
	if (f->partial() || (not outSortTable->finite())) {
		tseitin = translator()->reify(1, CompType::GEQ, AggFunction::CARD, setnr, TsType::IMPL);
	} else {
		tseitin = translator()->reify(1, CompType::EQ, AggFunction::CARD, setnr, TsType::IMPL);
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
