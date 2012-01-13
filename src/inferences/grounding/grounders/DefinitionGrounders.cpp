/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

#include "inferences/grounding/grounders/TermGrounders.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "inferences/grounding/grounders/GroundUtils.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "generators/InstGenerator.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>
#include <exception>

using namespace std;

unsigned int DefinitionGrounder::_currentdefnb = 1;

// INVAR: definition is always toplevel, so certainly conjunctive path to the root
DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr, const GroundingContext& context)
		: Grounder(gt, context), _defnb(_currentdefnb++), _subgrounders(subgr), _grounddefinition(new GroundDefinition(_defnb, gt->translator())) {
}

void DefinitionGrounder::run(ConjOrDisj& formula) const {
	for (auto grounder = _subgrounders.cbegin(); grounder < _subgrounders.cend(); ++grounder) {
		(*grounder)->run(id(), _grounddefinition);
	}
	getGrounding()->add(_grounddefinition); // FIXME check how it is handled in the lazy part

	formula.setType(Conn::CONJ); // Empty conjunction, so always true
}

RuleGrounder::RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
		: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) {
}

void RuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {
	Assert(defid==grounddefinition->id());
	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator ++()) {
		if (GlobalData::instance()->terminateRequested()) {
			throw IdpException("Terminate requested");
		}
		ConjOrDisj body;
		_bodygrounder->run(body);
		bool conj = body.getType() == Conn::CONJ;
		bool falsebody = (body.literals.empty() && !conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if (falsebody) {
			continue;
		}

		for (_headgenerator->begin(); not _headgenerator->isAtEnd(); _headgenerator->operator ++()) {
			if (GlobalData::instance()->terminateRequested()) {
				throw IdpException("Terminate requested");
			}
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

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, const PredTable* ct, const PredTable* cf, PFSymbol* s, const vector<TermGrounder*>& sg,
		const vector<SortTable*>& vst)
		: _grounding(gt), _subtermgrounders(sg), _ct(ct), _cf(cf), _symbol(gt->translator()->addSymbol(s)), _tables(vst), _pfsymbol(s) {
}

Lit HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for (unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		if (GlobalData::instance()->terminateRequested()) {
			throw IdpException("Terminate requested");
		}
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
	for (unsigned int n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...! Also produce a warning!
		if (not args[n])
			return _false;
		if (not _tables[n]->contains(args[n]))
			return _false;
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

// FIXME require a transformation such that there is only one headgrounder for any defined symbol
// FIXME also handle tseitin defined rules!
LazyRuleGrounder::LazyRuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: RuleGrounder(hgr, bgr, NULL, big, ct), _grounding(dynamic_cast<SolverTheory*>(headgrounder()->grounding())) {
	grounding()->translator()->notifyDefined(headgrounder()->pfsymbol(), this);
}

dominstlist LazyRuleGrounder::createInst(const ElementTuple& headargs) {
	dominstlist domlist;

	// set the variable instantiations
	for (unsigned int i = 0; i < headargs.size(); ++i) {
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
	// FIXME do this for all grounders with the same grounding (which should be all, is other TODO?)?
	grounding()->polNotifyDefined(lit, headargs, grounders);
}

void LazyRuleGrounder::ground(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	dominstlist headvarinstlist = createInst(headargs);

	vector<const DomainElement*> originstantiation;
	overwriteVars(originstantiation, headvarinstlist);

	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator ++()) {
		if (GlobalData::instance()->terminateRequested()) {
			throw IdpException("Terminate requested");
		}

		ConjOrDisj body;
		bodygrounder()->run(body);
		bool conj = body.getType() == Conn::CONJ;
		bool falsebody = (body.literals.empty() && !conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if (falsebody) {
			grounding()->add(GroundClause { -head });
			continue;
		} else if (truebody) {
			grounding()->add(GroundClause { head });
			continue;
		} else {
			// FIXME correct defID!
			grounding()->polAdd(1, new PCGroundRule(head, (conj ? RT_CONJ : RT_DISJ), body.literals, context()._tseitin == TsType::RULE));
		}
	}

	restoreOrigVars(originstantiation, headvarinstlist);
}

void LazyRuleGrounder::run(unsigned int, GroundDefinition*) const {
	// No-op
}
