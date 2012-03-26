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
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

LazyRuleGrounder::LazyRuleGrounder(const Rule* rule, const vector<Term*>& headterms, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: RuleGrounder(rule, hgr, bgr, big, ct), DelayGrounder(rule->head()->symbol(), headterms, Context::BOTH, ct.getCurrentDefID(), hgr->grounding()), grounded(0) {
	if(verbosity()>1){
		clog <<"Lazily grounding " <<toString(rule) <<" by unknown-delay of the head.\n";
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

tablesize LazyRuleGrounder::getGroundedSize() const{
	return grounded*bodygrounder()->getGroundedSize();
}

void LazyRuleGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	for(auto i=getSameargs().cbegin(); i<getSameargs().cend(); ++i){
		if(headargs[i->first]!=headargs[i->second]){
			return;
		}
	}

	dominstlist headvarinstlist;
	auto subst = createInst(headargs, headvarinstlist);
	if(subst==Substitutable::NO_UNIFIER){
		return;
	}

	grounded++;

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
