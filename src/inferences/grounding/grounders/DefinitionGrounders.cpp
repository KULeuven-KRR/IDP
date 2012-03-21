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
	auto t = tablesize(TableSizeType::TST_EXACT, 0);
	for(auto i=subgr.cbegin(); i<subgr.cend(); ++i){
		t = t + (*i)->getMaxGroundSize();
	}
	setMaxGroundSize(t);
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

RuleGrounder::RuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: origrule(rule->clone()), _headgrounder(hgr), _bodygrounder(bgr), _bodygenerator(big), _context(ct) {
	Assert(_headgrounder!=NULL);
	Assert(_bodygrounder!=NULL);
	Assert(_bodygenerator!=NULL);
}

void RuleGrounder::put(std::stringstream& stream) {
	stream << toString(origrule);
}

tablesize RuleGrounder::getMaxGroundSize() const {
	return headgrounder()->getUniverse().size() * bodygrounder()->getMaxGroundSize();
}

RuleGrounder::~RuleGrounder() {
	delete (_headgrounder);
	delete (_bodygrounder);
	delete (_bodygenerator);
}

FullRuleGrounder::FullRuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
		: RuleGrounder(rule, hgr, bgr, big, ct), _headgenerator(hig), done(false) {
	Assert(hig!=NULL);
	hgr->grounding()->notifyNeedFalseDefineds(hgr->pfsymbol()); // FIXME very ugly hack to get addFalseDefineds correct, see more info there (groundtheory.cpp)
}

FullRuleGrounder::~FullRuleGrounder() {
	delete (_headgenerator);
}

void FullRuleGrounder::run(DefId defid, GroundDefinition* grounddefinition) const {
	notifyRun();
	Assert(defid == grounddefinition->id());
	Assert(bodygenerator()!=NULL);
	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator++()) {
		CHECKTERMINATION
		ConjOrDisj body;
		bodygrounder()->run(body);
		auto conj = body.getType() == Conn::CONJ;
		auto falsebody = (body.literals.empty() && not conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		auto truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if (falsebody) {
			continue;
		}

		for (headgenerator()->begin(); not headgenerator()->isAtEnd(); headgenerator()->operator++()) {
			CHECKTERMINATION
			Lit head = headgrounder()->run();
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
