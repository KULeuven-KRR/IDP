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

#include "DefinitionGrounders.hpp"

#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "generators/InstGenerator.hpp"
#include "IncludeComponents.hpp"

#include "utils/ListUtils.hpp"
#include "errorhandling/error.hpp"

using namespace std;

// INVAR: definition is always toplevel, so certainly conjunctive path to the root
DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr, const GroundingContext& context)
		: 	Grounder(gt, context),
			_subgrounders(subgr) {
	Assert(context.getCurrentDefID()!=getIDForUndefined());
	auto t = tablesize(TableSizeType::TST_EXACT, 0);
	for (auto grounder : subgr) {
		t = t + grounder->getMaxGroundSize();
	}
	setMaxGroundSize(t);
}

DefinitionGrounder::~DefinitionGrounder() {
	deleteList(_subgrounders);
}

void DefinitionGrounder::put(std::ostream& stream) const {
	stream <<"definition " <<getDefinitionID().id <<" { \n";
	for(auto rg: getSubGrounders()){
		stream <<"\t" <<toString(rg) <<"\n";
	}

	stream <<"}\n";
}

void DefinitionGrounder::internalRun(ConjOrDisj& formula, LazyGroundingRequest& request) {
	if(verbosity()>2){
		clog <<"Grounding definition " <<print(this) <<"\n";
	}
	auto grounddefinition = new GroundDefinition(getDefinitionID(), translator());

	std::vector<PFSymbol*> headsymbols;
	for (auto grounder : _subgrounders) {
		CHECKTERMINATION;
		grounder->run(getDefinitionID(), grounddefinition, request);
		headsymbols.push_back(grounder->getHead()->symbol());
	}
	getGrounding()->add(*grounddefinition);

	for (auto symbol : headsymbols) {
		CHECKTERMINATION;
		auto pt = getGrounding()->structure()->inter(symbol)->pt();
		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			CHECKTERMINATION;
			auto translatedvar = getGrounding()->translator()->translateReduced(symbol, (*ptIterator), recursive(*symbol, getContext()));
			Assert(translatedvar>0);
			if (not grounddefinition->hasRule(translatedvar)) {
				getGrounding()->addUnitClause(-translatedvar);
				if(getGrounding()->structure()->inter(symbol)->ct()->contains(*ptIterator)) {
					formula.setType(Conn::DISJ); // Empty disjunction, always false
					return;// NOTE: abort early because inconsistent anyway
				}
				// TODO better solution would be to make the structure more precise
			}
		}
	}

	delete (grounddefinition);

	formula.setType(Conn::CONJ); // Empty conjunction, so always true
}

RuleGrounder::RuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
		: 	_origrule(rule->clone()),
			_headgrounder(hgr),
			_bodygrounder(bgr),
			_nonheadgenerator(big),
			_context(ct) {
	Assert(_headgrounder!=NULL);
	Assert(_bodygrounder!=NULL);
	Assert(_nonheadgenerator!=NULL);
}

void RuleGrounder::put(std::ostream& stream) const {
	stream << print(_origrule);
}

tablesize RuleGrounder::getMaxGroundSize() const {
	return headgrounder()->getUniverse().size() * bodygrounder()->getMaxGroundSize();
}

int RuleGrounder::verbosity() const {
	return getOption(IntType::VERBOSE_GROUNDING);
}

RuleGrounder::~RuleGrounder() {
	delete (_headgrounder);
	delete (_bodygrounder);
	delete (_nonheadgenerator);
	_origrule->recursiveDelete();
}

GroundTranslator* RuleGrounder::translator() const{
	return _bodygrounder->translator();
}

const std::vector<const DomElemContainer*>& RuleGrounder::getHeadVarContainers() const {
	return headgrounder()->getHeadVarContainers();
}

FullRuleGrounder::FullRuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
		: 	RuleGrounder(rule, hgr, bgr, big, ct),
			_headgenerator(hig),
			done(false) {
	Assert(hig!=NULL);

	map<Variable*, vector<uint> > samevars;
	for (uint i = 0; i < rule->head()->args().size(); ++i) {
		auto arg = rule->head()->args()[i];
		if (isa<VarTerm>(*arg)) {
			auto varterm = dynamic_cast<VarTerm*>(arg);
			samevars[varterm->var()].push_back(i);
		}
	}
	for (auto var2list : samevars) {
		samevalues.push_back(var2list.second);
	}
}

FullRuleGrounder::~FullRuleGrounder() {
	delete (_headgenerator);
}

void FullRuleGrounder::run(DefId defid, GroundDefinition* grounddefinition, LazyGroundingRequest& request) const {
	notifyRun();
	Assert(defid == grounddefinition->id());

	auto previousgroundsize = Grounder::groundedAtoms();
	if (verbosity() > 2) {
		clog <<"Grounding rule " <<print(this) <<"\n";
	}

	Assert(bodygenerator()!=NULL);
	for (bodygenerator()->begin(); not bodygenerator()->isAtEnd(); bodygenerator()->operator++()) {
		CHECKTERMINATION;
		if (verbosity() > 2) {
			clog <<"Generating rule body\n";
		}
		ConjOrDisj body;
		bodygrounder()->run(body, request);
		auto conj = body.getType() == Conn::CONJ;
		auto falsebody = (body.literals.empty() && not conj) || (body.literals.size() == 1 && body.literals[0] == _false);
		auto truebody = (body.literals.empty() && conj) || (body.literals.size() == 1 && body.literals[0] == _true);
		if (falsebody) {
			continue;
		}

		for (headgenerator()->begin(); not headgenerator()->isAtEnd(); headgenerator()->operator++()) {
			CHECKTERMINATION;
			Lit head = headgrounder()->run();
			Assert(head != _true);
			if (head != _false) {
				if (verbosity() > 2) {
					auto trans = headgrounder()->grounding()->translator();
					clog <<"Generated head " <<print(trans->getSymbol(head)) <<print(trans->getArgs(head)) <<"\n";
				}
				if (truebody) {
					body.literals.clear();
					conj = true;
				}
				grounddefinition->addPCRule(head, body.literals, conj, context()._tseitin == TsType::RULE);
			}
		}
	}

	if (verbosity() > 0) {
		clog <<"Rule " <<print(this) <<" was grounded to " <<Grounder::groundedAtoms()-previousgroundsize <<" atoms.\n";
	}
}

void FullRuleGrounder::groundForSetHeadInstance(GroundDefinition& def, const Lit& head, const ElementTuple& headinst) {
	for (auto list : samevalues) {
		auto first = headinst[list.front()];
		for (auto value : list) {
			if (headinst[value] != first) {
				return;
			}
		}
	}
	for (uint i = 0; i < headinst.size(); ++i) {
		auto arg = getHead()->args()[i];
		switch (arg->type()) {
		case TermType::DOM:
			if (headinst[i] != dynamic_cast<DomainTerm*>(arg)->value()) { // Not matching this head
				return;
			}
			break;
		case TermType::VAR:
			break;
		case TermType::FUNC:
		case TermType::AGG:
			cerr << toString(getHead()) << "\n";
			throw notyetimplemented("Lazy head grounding with non-unnested functions or aggregates");
		}
	}
	//	FIXME Replace groundForSetHeadInstance with lazygroundingrequest
	ConjOrDisj body;
	auto lgr = LazyGroundingRequest( { });
	bodygrounder()->run(body, lgr);

	auto conj = body.getType() == Conn::CONJ;
	litlist lits;
	if (body.literals.size() > 0) {
		// TODO why do we only need code here to erase true and false literals?
		bool allfalse = true;
		for (auto lit : body.literals) {
			if (lit == _true) {
				allfalse = false;
				continue;
			}
			if (lit == _false) {
				continue;
			} else {
				allfalse = false;
			}

			lits.push_back(lit);
		}

		if (lits.size() == 0) {
			conj = not allfalse;
		}
	}

	def.addPCRule(head, lits, conj, true);
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, PFSymbol* s, const vector<TermGrounder*>& sg, const vector<SortTable*>& vst, GroundingContext& context)
		: 	_grounding(gt),
			_subtermgrounders(sg),
			_symbol(gt->translator()->addSymbol(s)),
			_tables(vst),
			_pfsymbol(s),
			_context(context) {
	for (auto tg : _subtermgrounders) {
		Assert(not getOption(SATISFIABILITYDELAY) || isa<VarTermGrounder>(*tg) || isa<DomTermGrounder>(*tg));
		if (isa<VarTermGrounder>(*tg)) {
			auto vg = dynamic_cast<VarTermGrounder*>(tg);
			_headvarcontainers.push_back(vg->getElement());
		} else if (isa<DomTermGrounder>(*tg)) {
			auto vc = new DomElemContainer();
			*vc = dynamic_cast<DomTermGrounder*>(tg)->getDomainElement();
			_headvarcontainers.push_back(vc);
		}
	}
}

HeadGrounder::~HeadGrounder() {
	deleteList(_subtermgrounders);
}

GroundTranslator* HeadGrounder::translator() const {
	return _grounding->translator();
}

const std::vector<const DomElemContainer*>& HeadGrounder::getHeadVarContainers() const {
	return _headvarcontainers;
}

Lit HeadGrounder::run() const {
	// Run subterm grounders
	ElementTuple args;
	vector<GroundTerm> groundsubterms;
	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		CHECKTERMINATION;
		auto term = _subtermgrounders[n]->run();
		Assert (not term.isVariable);
		args.push_back(term._domelement);
		groundsubterms.push_back(term);
	}

	// Checking partial functions
	for (size_t n = 0; n < args.size(); ++n) {
		if (args[n] == NULL || not _tables[n]->contains(args[n])) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				Warning::warning("Out of bounds on headgrounder");
			}
			return _false;
		}
	}

	return _grounding->translator()->translateReduced(_symbol, args, recursive(*_pfsymbol, _context));
}
