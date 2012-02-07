/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "LazyRuleGrounder.hpp"
#include "TermGrounders.hpp"
#include "generators/InstGenerator.hpp"
#include "IncludeComponents.hpp"

using namespace std;

LazyRuleGrounder::LazyRuleGrounder(const Rule* rule, DefId defid, const vector<Term*>& headterms, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: RuleGrounder(rule, hgr, bgr, big, ct), LazyUnknBoundGrounder(rule->head()->symbol(), defid, hgr->grounding()) {
	std::map<Variable*, int> vartofirstocc;
	int index = 0;
	for(auto i=headterms.cbegin(); i<headterms.cend(); ++i, ++index){
		auto varterm = dynamic_cast<VarTerm*>(*i);
		if(varterm==NULL){
			Assert((*i)->freeVars().size()==0);
			continue;
		}

		auto first = vartofirstocc.find(varterm->var());
		if(first==vartofirstocc.cend()){
			vartofirstocc[varterm->var()] = index;
		}else{
			sameargs.push_back({first->second, index});
		}
	}
}

LazyRuleGrounder::Substitutable LazyRuleGrounder::createInst(const ElementTuple& headargs, dominstlist& domlist) {
	// set the variable instantiations
	for (size_t i = 0; i < headargs.size(); ++i) {
		auto grounder = headgrounder()->subtermgrounders()[i];
		// NOTE: can only be a vartermgrounder or a two-valued termgrounder here
		if (not sametypeid<VarTermGrounder>(*grounder)) {
			auto result = grounder->run(); // TODO running the grounder each time again?
			Assert(not result.isVariable);
			if(headargs[i]!=result._domelement){
				//clog <<"Head of rule " <<toString(this) <<" not unifiable with " <<toString(headargs) <<"\n";
				return Substitutable::NO_UNIFIER;
			}
		}else{
			auto var = (dynamic_cast<VarTermGrounder*>(grounder))->getElement();
			domlist.push_back(dominst { var, headargs[i] });
		}
	}
	//clog <<"Head of rule " <<toString(this) <<" unifies with " <<toString(headargs) <<"\n";
	return Substitutable::UNIFIABLE;
}

void LazyRuleGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	// NOTE: If multiple vars are the same, it is not checked that their instantiation is also the same!
	for(auto i=sameargs.cbegin(); i<sameargs.cend(); ++i){
		if(headargs[i->first]!=headargs[i->second]){
			return;
		}
	}

	dominstlist headvarinstlist;
	auto subst = createInst(headargs, headvarinstlist);
	if(subst==Substitutable::NO_UNIFIER){
		return;
	}

	vector<const DomainElement*> originst;
	overwriteVars(originst, headvarinstlist);

	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator ++()) {
		CHECKTERMINATION

		ConjOrDisj body;
		bodygrounder()->run(body);
		bool conj = body.getType() == Conn::CONJ;
		bool falsebody = (body.literals.empty() && !conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		// IMPORTANT! As multiple rules might exist, should NOT add unit clauses if one body is certainly true or false!
		if (falsebody) {
			conj = false;
			body.literals = {};
		} else if (truebody) {
			conj = true;
			body.literals = {};
		}
		getGrounding()->add(context().getCurrentDefID(), new PCGroundRule(head, (conj ? RuleType::CONJ : RuleType::DISJ), body.literals, context()._tseitin == TsType::RULE));
	}

	restoreOrigVars(originst, headvarinstlist);
}

void LazyRuleGrounder::run(DefId, GroundDefinition*) const {
	// No-op
}
