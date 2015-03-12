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
		  _nbofatoms(0),
		  addingTseitins(false) {
	Policy::polStartTheory(translator());
}

template<class Policy>
GroundTheory<Policy>::GroundTheory(Vocabulary* voc, StructureInfo info, bool nbModelsEquivalent)
		: AbstractGroundTheory(voc, info),
		  _nbModelsEquivalent(nbModelsEquivalent),
		  _nbofatoms(0),
		  addingTseitins(false) {
	Policy::polStartTheory(translator());
}

template<class Policy>
void GroundTheory<Policy>::startLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction){
	Policy::polStartLazyFormula(inst, type, conjunction);
}
template<class Policy>
void GroundTheory<Policy>::notifyLazyResidual(LazyInstantiation* inst, TsType type){
	Policy::polNotifyLazyResidual(inst, type);
}
template<class Policy>
void GroundTheory<Policy>::notifyLazyAddition(const litlist& glist, int ID) {
	addTseitinInterpretations(glist, getIDForUndefined());
	notifyAtomsAdded(glist.size());
	Policy::polAddLazyAddition(glist, ID);
}

template<class Policy>
void GroundTheory<Policy>::notifyLazyWatch(Atom atom, TruthValue watches, LazyGroundingManager* manager){
	Policy::polNotifyLazyWatch(atom, watches, manager);
}

template<class Policy>
void GroundTheory<Policy>::addLazyElement(Lit head, PFSymbol* symbol, const std::vector<GroundTerm>& args, bool recursive) {
	for(auto arg:args){
		if(arg.isVariable){
			addVarIdInterpretation(arg._varid);
			addFoldedVarEquiv(arg._varid);
		}
	}
	notifyAtomsAdded(2);
	Policy::polAddLazyElement(head, symbol, args, this, recursive);
}

template<class Policy>
void GroundTheory<Policy>::recursiveDelete() {
	Policy::polRecursiveDelete();
	deleteList(_foldedterms);
	delete (this);
}

template<class Policy>
void GroundTheory<Policy>::closeTheory() {
	if (not useLazyGrounding()) {
		Policy::polEndTheory();
	}
}

 template<class Policy>
void GroundTheory<Policy>::add(const GroundClause& cl, bool skipfirst) {
	bool propagates = cl.size()==1;
	// If propagates is true, it will have been added to the grounding if necessary by addTseitinInterpretations
	if(not propagates){
		notifyAtomsAdded(cl.size());
		Policy::polAdd(cl);
	}
	addTseitinInterpretations(cl, getIDForUndefined(), skipfirst, propagates);
}

template<class Policy>
void GroundTheory<Policy>::add(const GroundDefinition& def) {
	for (auto head2rule : def.rules()) {
		addTseitinInterpretations({head2rule.first}, def.id());
		auto rule = head2rule.second;
		if (isa<PCGroundRule>(*rule)) {
			add(def.id(), *dynamic_cast<PCGroundRule*>(rule));
		} else {
			Assert(isa<AggGroundRule>(*rule));
			auto aggrule = dynamic_cast<AggGroundRule*>(rule);
			add(aggrule->setnr(), def.id(), (aggrule->aggtype() != AggFunction::CARD));
			notifyAtomsAdded(2);
			Policy::polAdd(def.id(), aggrule);
		}
	}
}

template<class Policy>
void GroundTheory<Policy>::add(DefId defid, const PCGroundRule& rule) {
	Assert(defid!=getIDForUndefined());
	addTseitinInterpretations({rule.head()}, defid);
	addTseitinInterpretations(rule.body(), defid);
	notifyAtomsAdded(rule.body().size()+1);
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
	if (contains(_addedvarinterpretation,id)) {
		return;
	}

	_addedvarinterpretation.insert(id);

	// It might be already partially known
	translator()->addKnown(id);
}

template<class Policy>
void GroundTheory<Policy>::add(Lit tseitin, CPTsBody* body) {
	body->left(foldCPTerm(body->left(), getIDForUndefined()));

	for(auto var: body->left()->getVarIds()){
		addVarIdInterpretation(var);
	}

	if (body->right()._isvarid) {
		auto id = body->right()._varid;
		addVarIdInterpretation(id);
		addFoldedVarEquiv(id);
	}

	notifyAtomsAdded(2);
	Policy::polAdd(tseitin, body);
}

template<class Policy>
void GroundTheory<Policy>::add(Lit tseitin, VarId varid) {
	addVarIdInterpretation(varid);
	notifyAtomsAdded(1);
	Policy::polAdd(tseitin, varid);
}

template<class Policy>
void GroundTheory<Policy>::add(SetId setnr, DefId defnr, bool weighted) {
	if (_addedSets.find(setnr) != _addedSets.end()) {
		return;
	}
	_addedSets.insert(setnr);

	auto tsset = translator()->groundset(setnr);
	addTseitinInterpretations(tsset.literals(), defnr);
	notifyAtomsAdded(tsset.literals().size());
	Policy::polAdd(tsset, setnr, weighted);
}

template<class Policy>
void GroundTheory<Policy>::add(Lit head, AggTsBody* body) {
	add(body->setnr(), getIDForUndefined(), (body->aggtype() != AggFunction::CARD));
	notifyAtomsAdded(2);
	Policy::polAdd(head, body);
}

template<class Policy>
void GroundTheory<Policy>::add(const Lit& head, TsType type, const litlist& body, bool conj, DefId defnr) {
	if (type == TsType::IMPL) {
		if (conj) {
			for (auto lit : body) {
				add( { -head, lit }, true);
			}
		} else {
			litlist cl(body.size() + 1, -head);
			for (size_t i = 0; i < body.size(); ++i) {
				cl[i + 1] = body[i];
			}
			add(cl, true);
		}
	}
	if (type == TsType::RIMPL) {
		if (conj) {
			litlist cl(body.size() + 1, head);
			for (size_t i = 0; i < body.size(); ++i) {
				cl[i + 1] = -body[i];
			}
			add(cl, true);
		} else {
			for (auto lit : body) {
				add( { head, -lit }, true);
			}
		}
	}
	if (type == TsType::RULE) {
		Assert(defnr != getIDForUndefined());
		add(defnr, PCGroundRule(head, conj ? RuleType::CONJ : RuleType::DISJ, body, true)); //TODO true (recursive) might not always be the case?
	}
	if (type ==  TsType::EQ){
		GroundEquivalence geq(head, body, conj);
		Policy::polAdd(geq);
		addTseitinInterpretations(body, getIDForUndefined(), false, false);
		notifyAtomsAdded(body.size()+1);
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

		if (not translator()->isTseitinWithSubformula(tseitin) || contains(_addedTseitins, tseitin)) {
			if(atroot){
				notifyAtomsAdded(1);
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
				notifyAtomsAdded(2);
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
		} else if (isa<DenotingTsBody>(*tsbody)) {
			add(tseitin, dynamic_cast<DenotingTsBody*>(tsbody)->getVarId());
		} else {
			Assert(isa<LazyTsBody>(*tsbody));
			auto body = dynamic_cast<LazyTsBody*>(tsbody);
			body->notifyTheoryOccurence();
		}

		if(atroot && not eliminated){
			notifyAtomsAdded(1);
			Policy::polAdd(GroundClause{elem.lit});
		}

		if(not eliminated){
			_addedTseitins.insert(tseitin);
			translator()->removeTsBody(tseitin);
		}
	}
	addingTseitins = false;
}

template<class Policy>
CPTerm* GroundTheory<Policy>::foldCPTerm(CPTerm* cpterm, DefId defnr) {
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
		auto replacement = foldCPTerm(cprelation->left(), defnr);
		auto replacementvar = dynamic_cast<CPVarTerm*>(replacement);
		if (replacementvar==NULL) {
			if(cprelation->comp() != CompType::EQ || not cprelation->right()._isvarid || cprelation->right()._varid != varterm->varid()){
				throw InternalIdpException("Invalid code path");
			}
			return replacement;
		}
		return cpterm;
	}

	auto term = dynamic_cast<CPSetTerm*>(cpterm);
	addTseitinInterpretations(term->conditions(), defnr);
	Assert(term!=NULL);
	switch(term->type()){
	case AggFunction::SUM:{
		varidlist newvarids;
		litlist newconditions;
		intweightlist newweights;
		for (uint i=0; i != term->varids().size(); ++i) {
			auto varid = term->varids()[i];
			auto oldcondition = term->conditions()[i];
			auto weight1 = term->weights()[i];
			auto cprelation = translator()->cprelation(varid);
			if (cprelation != NULL) {
				auto left = foldCPTerm(cprelation->left(), defnr);
				auto leftassetterm = dynamic_cast<CPSetTerm*>(left);
				if (leftassetterm != NULL && leftassetterm->type() == AggFunction::SUM and cprelation->comp() == CompType::EQ) {
					Assert(cprelation->right()._isvarid and cprelation->right()._varid == varid);
					insertAtEnd(newvarids, leftassetterm->varids());
					//The new condition is the conjunction of the condition in the original set and the condition in the nested set
					for (auto condition2 : leftassetterm->conditions()) {
						auto newcond = translator()->conjunction(oldcondition, condition2, TsType::EQ);
						newconditions.push_back(newcond);
					}
					for (auto weight2 : leftassetterm->weights()) {
						newweights.push_back(weight1 * weight2);
					}
					continue;
				} else if (leftassetterm != NULL and leftassetterm->type() == AggFunction::PROD and cprelation->comp() == CompType::EQ
						and leftassetterm->varids().size()==2){ // of the form varid = prod{(true,int),(true, term)} OR varid = prod{(true,int),(cond,term)} and oldcondition = true
					auto varone = leftassetterm->varids()[0];
					auto vartwo = leftassetterm->varids()[1];
					if(translator()->domain(varone)->size()==tablesize(TableSizeType::TST_EXACT, 1)
							&& leftassetterm->conditions()[0]==_true
							&& (oldcondition==_true || leftassetterm->conditions()[1]==_true)){
						newvarids.push_back(vartwo);
						newconditions.push_back(oldcondition==_true?leftassetterm->conditions()[1]:oldcondition);
						newweights.push_back(weight1 * (*translator()->domain(varone)->begin()).front()->value()._int);
						continue;
					}else if(translator()->domain(vartwo)->size()==tablesize(TableSizeType::TST_EXACT, 1)
							&& leftassetterm->conditions()[1]==_true
							&& (oldcondition==_true || leftassetterm->conditions()[0]==_true)){
						newvarids.push_back(varone);
						newconditions.push_back(oldcondition==_true?leftassetterm->conditions()[0]:oldcondition);
						newweights.push_back(weight1 * (*translator()->domain(vartwo)->begin()).front()->value()._int);
						continue;
					}
				}
			}
			newvarids.push_back(varid);
			addFoldedVarEquiv(varid);
			newweights.push_back(weight1);
			newconditions.push_back(term->conditions()[i]);
		}
		term->varids(newvarids);
		term->weights(newweights);
		term->conditions(newconditions);
		addTseitinInterpretations(newconditions, defnr);
		return term;
	}
	case AggFunction::PROD:{
		varidlist newvarids;
		litlist newconditions;
		int newweight = term->weights().back();
		for (uint i=0; i != term->varids().size(); ++i) {
			auto varid = term->varids()[i];
			auto cprelation = translator()->cprelation(varid);
			if (cprelation != NULL) {
				auto left = foldCPTerm(cprelation->left(), defnr);
				auto leftassetterm = dynamic_cast<CPSetTerm*>(left);
				if (leftassetterm!=NULL && leftassetterm->type()==AggFunction::PROD and cprelation->comp() == CompType::EQ) {
					Assert(cprelation->right()._isvarid and cprelation->right()._varid == varid);
					insertAtEnd(newvarids, leftassetterm->varids());
					insertAtEnd(newconditions, leftassetterm->conditions());
					newweight *= leftassetterm->weights().back();
					continue;
				}
			}
			newvarids.push_back(varid);
			addFoldedVarEquiv(varid);
			newconditions.push_back(term->conditions()[i]);
			//Note: weight doesn't change
		}
		term->varids(newvarids);
		term->weights({newweight});
		term->conditions(newconditions);
		addTseitinInterpretations(newconditions, defnr);
		return term;
	}
	default: // TODO better solution for min and max?
		for(auto var: cpterm->getVarIds()){
			addFoldedVarEquiv(var);
		}

		return cpterm;
	}
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
