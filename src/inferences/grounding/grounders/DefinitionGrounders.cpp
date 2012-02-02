/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "DefinitionGrounders.hpp"

#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "GroundUtils.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "generators/InstGenerator.hpp"
#include "IncludeComponents.hpp"

#include "utils/ListUtils.hpp"

using namespace std;

// INVAR: definition is always toplevel, so certainly conjunctive path to the root
DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr, const GroundingContext& context)
		: Grounder(gt, context), _subgrounders(subgr) {
	Assert(context.getCurrentDefID()!=getIDForUndefined());
}

DefinitionGrounder::~DefinitionGrounder() {
	deleteList(_subgrounders);
}

void DefinitionGrounder::run(ConjOrDisj& formula) const {
	auto grounddefinition = new GroundDefinition(id(), getTranslator());
	for (auto grounder = _subgrounders.cbegin(); grounder < _subgrounders.cend(); ++grounder) {
		(*grounder)->run(id(), grounddefinition);
	}
	getGrounding()->add(*grounddefinition); // FIXME check how it is handled in the lazy part
	delete (grounddefinition);
	formula.setType(Conn::CONJ); // Empty conjunction, so always true
}

RuleGrounder::RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
		: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) {
	// TODO Assert(...!=NULL);
}

RuleGrounder::~RuleGrounder() {
	delete (_headgenerator);
	delete (_headgrounder);
	delete (_bodygrounder);
	delete (_bodygenerator);
}

void RuleGrounder::run(DefId defid, GroundDefinition* grounddefinition) const {
	Assert(defid == grounddefinition->id());
	Assert(bodygenerator()!=NULL);
	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator++()) {
		CHECKTERMINATION
		ConjOrDisj body;
		_bodygrounder->run(body);
		bool conj = body.getType() == Conn::CONJ;
		bool falsebody = (body.literals.empty() && not conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if (falsebody) {
			continue;
		}

		Assert(_headgenerator!=NULL);
		for (_headgenerator->begin(); not _headgenerator->isAtEnd(); _headgenerator->operator++()) {
			CHECKTERMINATION
			Lit head = _headgrounder->run();
			Assert(head != _true);
			if (head != _false) {
				if (truebody) {
					body.literals.clear();
					conj = true;
				}
				grounddefinition->addPCRule(head, body.literals, conj, context()._tseitin == TsType::RULE);
			}
		}
	}
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, const PredTable* ct, const PredTable* cf, PFSymbol* s, const vector<TermGrounder*>& sg, const vector<SortTable*>& vst)
		: _grounding(gt), _subtermgrounders(sg), _ct(ct), _cf(cf), _symbol(gt->translator()->addSymbol(s)), _tables(vst), _pfsymbol(s) {
}

HeadGrounder::~HeadGrounder() {
	deleteList(_subtermgrounders);
}

Lit HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		CHECKTERMINATION
		groundsubterms[n] = _subtermgrounders[n]->run();
		if (groundsubterms[n].isVariable) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	// TODO guarantee that all subterm grounders return domain elements
	Assert(alldomelts);

	// Checking partial functions
	for (size_t n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...! Also produce a warning!
		if (not args[n]) {
			return _false;
		}
		if (not _tables[n]->contains(args[n])) {
			return _false;
		}
	}

	// Run instance checkers and return grounding
	Lit atom = _grounding->translator()->translate(_symbol, args);
	if (_ct->contains(args)) {
		_grounding->addUnitClause(atom);
	} else if (_cf->contains(args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

LazyRuleGrounder::LazyRuleGrounder(const vector<Term*>& vars, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: RuleGrounder(hgr, bgr, NULL, big, ct), _grounding(dynamic_cast<SolverTheory*>(headgrounder()->grounding())), isGrounding(false) {
	grounding()->translator()->notifyDefined(headgrounder()->pfsymbol(), this);

	std::map<Variable*, int> vartofirstocc;
	int index = 0;
	for(auto i=vars.cbegin(); i<vars.cend(); ++i, ++index){
		auto varterm = dynamic_cast<VarTerm*>(*i);
		Assert(varterm!=NULL);
		auto first = vartofirstocc.find(varterm->var());
		if(first==vartofirstocc.cend()){
			vartofirstocc[varterm->var()] = index;
		}else{
			sameargs.push_back({first->second, index});
		}
	}
}

dominstlist LazyRuleGrounder::createInst(const ElementTuple& headargs) {
	dominstlist domlist;

	// set the variable instantiations
	for (size_t i = 0; i < headargs.size(); ++i) {
		// FIXME what if it is not a VarTermGrounder! (e.g. if it is a constant => we should check whether it can unify with it)
		if (not sametypeid<VarTermGrounder>(*headgrounder()->subtermgrounders()[i])) {
			throw notyetimplemented("Lazygrounding with functions.\n");
		}
		auto var = (dynamic_cast<VarTermGrounder*>(headgrounder()->subtermgrounders()[i]))->getElement();
		domlist.push_back(dominst { var, headargs[i] });
	}
	return domlist;
}

void LazyRuleGrounder::notify(const Lit& lit, const ElementTuple& headargs, const std::vector<LazyRuleGrounder*>& grounders) {
	grounding()->polNotifyDefined(lit, headargs, grounders);
}

void LazyRuleGrounder::ground(const Lit& head, const ElementTuple& headargs) {
	stilltoground.push( { head, headargs });
	if (not isGrounding) {
		doGrounding();
	}
}

void LazyRuleGrounder::doGrounding() {
	isGrounding = true;
	while (not stilltoground.empty()) {
		auto elem = stilltoground.front();
		stilltoground.pop();
		doGround(elem.first, elem.second);
	}
	isGrounding = false;
}

void LazyRuleGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	// NOTE: If multiple vars are the same, it is not checked that their instantiation is also the same!
	for(auto i=sameargs.cbegin(); i<sameargs.cend(); ++i){
		if(headargs[i->first]!=headargs[i->second]){
			return;
		}
	}

	dominstlist headvarinstlist = createInst(headargs);

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
		grounding()->add(context().getCurrentDefID(), new PCGroundRule(head, (conj ? RuleType::CONJ : RuleType::DISJ), body.literals, context()._tseitin == TsType::RULE));
	}

	restoreOrigVars(originst, headvarinstlist);
}

void LazyRuleGrounder::run(DefId, GroundDefinition*) const {
	// No-op
}
