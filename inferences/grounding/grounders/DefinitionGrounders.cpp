#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

#include "inferences/grounding/grounders/TermGrounders.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "inferences/grounding/grounders/GroundUtils.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "generators/InstGenerator.hpp"
#include "checker.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>
#include <exception>

using namespace std;

unsigned int DefinitionGrounder::_currentdefnb = 1;

// INVAR: definition is always toplevel, so certainly conjunctive path to the root
DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr, const GroundingContext& context)
		: Grounder(gt,context), _defnb(_currentdefnb++), _subgrounders(subgr), _grounddefinition(new GroundDefinition(_defnb, gt->translator())) {
}

void DefinitionGrounder::run(ConjOrDisj& formula) const {
	for(auto grounder = _subgrounders.cbegin(); grounder<_subgrounders.cend(); ++grounder){
		(*grounder)->run(id(), _grounddefinition);
	}
	grounding()->add(_grounddefinition); // FIXME check how it is handled in the lazy part

	formula.type = Conn::CONJ; // Empty conjunction, so always true
}

RuleGrounder::RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
			: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) {
}

void RuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {
	for(bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator ++()){
		ConjOrDisj body;
		_bodygrounder->run(body);
		bool conj = body.type==Conn::CONJ;
		bool falsebody = (body.literals.empty() && !conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if(falsebody){
			continue;
		}

		for(_headgenerator->begin(); not _headgenerator->isAtEnd(); _headgenerator->operator ++()){
			Lit head = _headgrounder->run();
			assert(head != _true);
			if(head != _false) {
				if(truebody){
					body.literals.clear();
				}
				grounddefinition->addPCRule(head, body.literals, conj, context()._tseitin == TsType::RULE);
			}
		};
	}
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt,
							InstanceChecker* pc,
							InstanceChecker* cc,
							PFSymbol* s,
							const vector<TermGrounder*>& sg,
							const vector<SortTable*>& vst)
		: _grounding(gt), _subtermgrounders(sg),
		  _truechecker(pc), _falsechecker(cc),
		  _symbol(gt->translator()->addSymbol(s)),
		  _tables(vst),
		  _pfsymbol(s){
}

int HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n].isVariable) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	// TODO guarantee that all subterm grounders return domain elements
	assert(alldomelts);

	// Checking partial functions
	for(unsigned int n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...! Also produce a warning!
		if(not args[n]) return _false;
		if(not _tables[n]->contains(args[n])) return _false;
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,args);
	if(_truechecker->isInInterpretation(args)) {
		_grounding->addUnitClause(atom);
	}else if(_falsechecker->isInInterpretation(args)) {
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

dominstlist LazyRuleGrounder::createInst(const ElementTuple& headargs){
	dominstlist domlist;

	// set the variable instantiations
	for(uint i=0; i<headargs.size(); ++i){
		// FIXME what if it is not a VarTermGrounder! (e.g. if it is a constant => we should check whether it can unify with it)
		if(not sametypeid<VarTermGrounder>(*headgrounder()->subtermgrounders()[i])){
			notyetimplemented("Lazygrounding with functions.\n");
			throw std::exception();
		}
		auto var = (dynamic_cast<VarTermGrounder*>(headgrounder()->subtermgrounders()[i]))->getElement();
		domlist.push_back(dominst(var, headargs[i]));
	}
	return domlist;
}

void LazyRuleGrounder::notify(const Lit& lit, const ElementTuple& headargs, const std::vector<LazyRuleGrounder*>& grounders){
	// FIXME do this for all grounders with the same grounding (which should be all, is other TODO?)?
	grounding()->polNotifyDefined(lit, headargs, grounders);
}

void LazyRuleGrounder::ground(const Lit& head, const ElementTuple& headargs){
	assert(head!=_true && head!=_false);

	dominstlist headvarinstlist = createInst(headargs);

	vector<const DomainElement*> originstantiation;
	overwriteVars(originstantiation, headvarinstlist);

	for(bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator ++()){
		ConjOrDisj body;
		bodygrounder()->run(body);
		bool conj = body.type==Conn::CONJ;
		bool falsebody = (body.literals.empty() && !conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		bool truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if(falsebody){
			grounding()->add(GroundClause{-head});
			continue;
		}else if(truebody){
			grounding()->add(GroundClause{head});
			continue;
		}else{
			// FIXME correct defID!
			grounding()->polAdd(1, new PCGroundRule(head, (conj ? RT_CONJ : RT_DISJ), body.literals, context()._tseitin == TsType::RULE));
		}
	}

	restoreOrigVars(originstantiation, headvarinstlist);
}

void LazyRuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {

}
