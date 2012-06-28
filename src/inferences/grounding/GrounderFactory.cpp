/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GrounderFactory.hpp"
#include "IncludeComponents.hpp"

#include <limits>
#include <cmath>
#include <cstdlib>
#include <utility> // for relational operators (namespace rel_ops)
#include "options.hpp"
#include "generators/GeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "grounders/FormulaGrounders.hpp"
#include "grounders/TermGrounders.hpp"
#include "grounders/SetGrounders.hpp"
#include "grounders/DefinitionGrounders.hpp"
#include "grounders/LazyFormulaGrounders.hpp"
#include "grounders/LazyRuleGrounder.hpp"
#include "inferences/grounding/grounders/OptimizationTermGrounders.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "inferences/SolverConnection.hpp"

#include "generators/BasicCheckersAndGenerators.hpp"
#include "generators/TableCheckerAndGenerators.hpp"

#include "theory/TheoryUtils.hpp"

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFactory.hpp"

#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/PrintGroundPolicy.hpp"
#include "groundtheories/SolverTheory.hpp"

#include "structure/StructureComponents.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"

#include "utils/CPUtils.hpp"

using namespace std;
using namespace rel_ops;

template<class T>
void deleteDeep(T& object) {
	object->recursiveDelete();
	object = NULL;
}

GenType operator not(GenType orig) {
	GenType result = GenType::CANMAKEFALSE;
	switch (orig) {
	case GenType::CANMAKEFALSE:
		result = GenType::CANMAKETRUE;
		break;
	case GenType::CANMAKETRUE:
		result = GenType::CANMAKEFALSE;
		break;
	}
	return result;
}

DefId getIDForUndefined() {
	return DefId(-1);
}

template<typename Grounding>
GrounderFactory::GrounderFactory(const GroundInfo& data, Grounding* grounding, bool nbModelsEquivalent)
		: 	_theory(data.theory),
			_minimizeterm(data.minimizeterm),
			_vocabulary(data.partialstructure->vocabulary()),
			_structure(data.partialstructure),
			_symstructure(data.symbolicstructure),
			_grounding(grounding),
			_nbmodelsequivalent(nbModelsEquivalent) {
	Assert(_symstructure != NULL);

	// Create a symbolic structure if no such structure is given
	if (getOption(IntType::GROUNDVERBOSITY) > 2) {
		clog << tabs() << "Using the following symbolic structure to ground:" << "\n";
		clog << tabs() << toString(_symstructure) << "\n";
	}
}

GrounderFactory::~GrounderFactory() {
}

/**
 * 	Finds out whether a formula contains recursively defined symbols.
 */
bool GrounderFactory::recursive(const Formula* f) {
	for (auto it = _context._defined.cbegin(); it != _context._defined.cend(); ++it) {
		if (f->contains(*it)) {
			return true;
		}
	}
	return false;
}

/**
 *	Initializes the context of the GrounderFactory before visiting a sentence.
 */
void GrounderFactory::InitContext() {
	_context.gentype = GenType::CANMAKEFALSE;
	_context._funccontext = Context::POSITIVE;
	_context._monotone = Context::POSITIVE;
	_context._component = CompContext::SENTENCE;
	_context._tseitin = _nbmodelsequivalent ? TsType::EQ : TsType::IMPL;
	_context.currentDefID = getIDForUndefined();
	_context._defined.clear();
	_context._conjunctivePathFromRoot = true; // NOTE: default true: needs to be set to false in each visit in grounderfactory in which it is no longer the case
	_context._conjPathUntilNode = true;
	_context._allowDelaySearch = true;

	_context._mappedvars.clear();
	_varmapping.clear();
}

void GrounderFactory::AggContext() {
	_context.gentype = GenType::CANMAKEFALSE;
	_context._funccontext = Context::POSITIVE;
	_context._tseitin = (_context._tseitin == TsType::RULE) ? TsType::RULE : TsType::EQ;
	_context._component = CompContext::FORMULA;
}

void GrounderFactory::SaveContext() {
	_contextstack.push(_context);
	_context._mappedvars.clear();
}

/**
 *	Restores the context to the top of the stack and pops the stack.
 */
void GrounderFactory::RestoreContext() {
	for (auto it = _context._mappedvars.begin(); it != _context._mappedvars.end(); ++it) {
		auto found = _varmapping.find(*it);
		if (found != _varmapping.end()) {
			_varmapping.erase(found);
		}
	}
	_context._mappedvars.clear();

	_context = _contextstack.top();
	_contextstack.pop();
}

/**
 *	Adapts the context to go one level deeper, and inverting some values if sign is negative
 */
void GrounderFactory::DeeperContext(SIGN sign) {
	// One level deeper
	if (_context._component == CompContext::SENTENCE) {
		_context._component = CompContext::FORMULA;
	}

	// If the parent was no longer conjunctive, the new node also won't be
	if (not _context._conjPathUntilNode) {
		_context._conjunctivePathFromRoot = false;
	}

	// Swap positive, truegen and tseitin according to sign
	if (isNeg(sign)) {
		_context.gentype = not _context.gentype;
		_context._funccontext = not _context._funccontext;
		_context._monotone = not _context._monotone;
		_context._tseitin = invertImplication(_context._tseitin);
	}
}

template<class GroundTheory>
Grounder* GrounderFactory::createGrounder(const GroundInfo& data, GroundTheory groundtheory) {
	Assert(VocabularyUtils::isContainedIn(data.minimizeterm, data.partialstructure->vocabulary()));
	Assert(VocabularyUtils::isSubVocabulary(data.theory->vocabulary(), data.partialstructure->vocabulary()));
	GrounderFactory g(data, groundtheory, data.nbModelsEquivalent);
	return g.ground();
}

/**
 * Creates a grounder for the given theory. The grounding produced by that grounder
 * will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 * The produced grounding is not passed to a solver, but stored internally as a EcnfTheory.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 */
Grounder* GrounderFactory::create(const GroundInfo& data) {
	auto groundtheory = new GroundTheory<GroundPolicy>(data.theory->vocabulary(), data.partialstructure);
	return createGrounder(data, groundtheory);
}
Grounder* GrounderFactory::create(const GroundInfo& data, InteractivePrintMonitor* monitor) {
	auto groundtheory = new GroundTheory<PrintGroundPolicy>(data.partialstructure);
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator());
	return createGrounder(data, groundtheory);
}

/**
 *	Creates a grounder for the given theory. The grounding produced by that grounder
 *	will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *	The produced grounding is directly passed to the given solver.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created.
 *		solver	- the solver to which the grounding will be passed.
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 *		One or more models of the ground theory can be obtained by calling solve() on
 *		the solver.
 */
Grounder* GrounderFactory::create(const GroundInfo& data, PCSolver* solver) {
	auto groundtheory = new SolverTheory(data.theory->vocabulary(), data.partialstructure);
	groundtheory->initialize(solver, getOption(IntType::GROUNDVERBOSITY), groundtheory->translator());
	auto grounder = createGrounder(data, groundtheory);
	SolverConnection::setTranslator(solver, grounder->getTranslator());
	return grounder;
}
/*
Grounder* GrounderFactory::create(const GroundInfo& data, FZRewriter* printer) {
	auto groundtheory = new GroundTheory<SolverPolicy<FZRewriter> >(data.theory->vocabulary(), data.partialstructure->clone());
	groundtheory->initialize(printer, getOption(IntType::GROUNDVERBOSITY), groundtheory->translator());
	GrounderFactory g( { data.partialstructure, data.symbolicstructure }, groundtheory);
	data.theory->accept(&g);
	return g.getTopGrounder();
}*/

Grounder* GrounderFactory::ground() {
	std::vector<Grounder*> grounders;

	allowskolemize = true;

	// NOTE: important that we only add funcconstraints for the theory at hand! e.g. for calculate definitions, we should not find values for the functions not occurring in it!
	FormulaUtils::addFuncConstraints(_theory, _vocabulary, funcconstraints, not getOption(BoolType::CPSUPPORT));
	if (_minimizeterm != NULL) {
		FormulaUtils::addFuncConstraints(_minimizeterm, _vocabulary, funcconstraints, not getOption(BoolType::CPSUPPORT));
	}

	InitContext();
	descend(_theory);
	grounders.push_back(getTopGrounder());

	if (_minimizeterm != NULL) {
		OptimizationGrounder* optimgrounder;
		if (getOption(BoolType::CPSUPPORT) and CPSupport::eligibleForCP(_minimizeterm, _structure)) {
			InitContext();
			descend(_minimizeterm);
			optimgrounder = new VariableOptimizationGrounder(getGrounding(), getTermGrounder(), getContext());
			optimgrounder->setOrig(_minimizeterm);
			grounders.push_back(optimgrounder);
		} else {
			switch (_minimizeterm->type()) {
			case TermType::AGG: {
				auto term = dynamic_cast<const AggTerm*>(_minimizeterm);
				if (term == NULL) {
					throw notyetimplemented("Optimization over non-aggregate terms");
				}
				if (term->function() == AggFunction::PROD) {
					for (auto i = term->set()->getSubSets().cbegin(); i != term->set()->getSubSets().cend(); ++i) {
						auto sort = (*i)->getTerm()->sort();
						if (not SortUtils::isSubsort(sort, get(STDSORT::NATSORT)) || _structure->inter(sort)->contains(createDomElem(0))) {
							throw notyetimplemented("Minimization over a product aggregate with negative or zero weights");
						}
					}
				}
				InitContext();
				descend(term->set());
				auto optimgrounder = new AggregateOptimizationGrounder(getGrounding(), term->function(), getSetGrounder(), getContext());
				optimgrounder->setOrig(_minimizeterm);
				grounders.push_back(optimgrounder);
				break;
			}
			case TermType::FUNC:
			case TermType::VAR:
			case TermType::DOM:
				throw notyetimplemented("Optimization over non-aggregate terms without CP support.");
			}
		}
	}

	allowskolemize = false;
	for (auto i = funcconstraints.cbegin(); i != funcconstraints.cend(); ++i) {
		InitContext();
		descend(i->second);
		grounders.push_back(getTopGrounder());
	}
	allowskolemize = true;

	if (grounders.size() == 1) {
		return grounders.front();
	}
	InitContext();
	return new BoolGrounder(getGrounding(), grounders, SIGN::POS, true, getContext());
}

/**
 *	Visits a child and ensures the context is saved before the visit and restored afterwards.
 */
template<typename T>
void GrounderFactory::descend(T child) {
	Assert(child!=NULL);
	SaveContext();

	if (getContext()._component == CompContext::SENTENCE) {
		if (getOption(IntType::GROUNDVERBOSITY) > 0) {
			clog << tabs() << "Creating a grounder for " << toString(child) << "\n";
			if (getOption(IntType::GROUNDVERBOSITY) > 3) {
				pushtab();
			}
		}
	} else {
		if (getOption(IntType::GROUNDVERBOSITY) > 3) {
			clog << tabs() << "Creating a subgrounder for " << toString(child) << "\n";
			pushtab();
		}
	}

	_formgrounder = NULL;
	_termgrounder = NULL;
	_setgrounder = NULL;
	_headgrounder = NULL;
	_rulegrounder = NULL;
	_topgrounder = NULL;

	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false; // NOTE: overwrite at start of visit if necessary!
	child->accept(this);

	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}

	RestoreContext();
}

void GrounderFactory::visit(const Theory* theory) {
	_context._conjPathUntilNode = true;

	// experiment with:
	//tmptheory = FormulaUtils::removeFunctionSymbolsFromDefs(tmptheory, _structure);

	const auto components = theory->components();
		// NOTE: primitive reorder present: definitions first => important for good lazy grounding at the moment
	// TODO Order the components to optimize the grounding process

	// Create grounders for all components
	auto newtheory = theory;

	/*	SKOLEM
	 auto newtheory = new Theory("", _vocabulary, theory->pi());
	 for (auto i = components.cbegin(); i < components.cend(); ++i) {
	 auto component = *i;


	 auto formula = dynamic_cast<Formula*>(*i);
	 // Add definitions etc!
	 // Can we handle subformula  directly if we store the parent quantifiers?
	 if (formula!=NULL && allowskolemize && not _nbmodelsequivalent) { // NOTE: skolemization is not nb-model-equivalent out of the box (might help this in future by changing solver)
	 formula = formula->clone();
	 component = FormulaUtils::skolemize(formula, _vocabulary);
	 FormulaUtils::addFuncConstraints(component, _vocabulary, funcconstraints, getOption(BoolType::CPSUPPORT));
	 }

	 newtheory->add(component);
	 }

	 // bugged:
	 newtheory = FormulaUtils::replaceWithNestedTseitins(newtheory);
	 SKOLEM END
	 */

	std::vector<Grounder*> children;
	const auto components2 = newtheory->components(); // NOTE: primitive reorder present: definitions first
	for (auto i = components2.cbegin(); i < components2.cend(); ++i) {
		InitContext();
		descend(*i);
		children.push_back(getTopGrounder());
	}

	// deleteDeep(newtheory); SKOLEM

	_topgrounder = new BoolGrounder(getGrounding(), children, SIGN::POS, true, getContext());
}

template<class T>
CompType getCompType(T symbol) {
	if (is(symbol, STDPRED::EQ)) {
		return CompType::EQ;
	} else if (is(symbol, STDPRED::LT)) {
		return CompType::LT;
	} else {
		Assert(is(symbol,STDPRED::GT));
		return CompType::GT;
	}
}

std::vector<SortTable*> getArgTables(Function* function, AbstractStructure* structure){
	vector<SortTable*> tables;
	for(auto i=function->sorts().cbegin(); i<function->sorts().cend()-1; ++i) {
		tables.push_back(structure->inter(*i));
	}
	return tables;
}

void GrounderFactory::visit(const PredForm* pf) {
	auto temppf = pf->clone();
	auto transpf = FormulaUtils::unnestThreeValuedTerms(temppf, _structure, _context._funccontext, getOption(BoolType::CPSUPPORT) && not recursive(pf));
			// TODO recursive could be more fine-grained (unnest any not rec defined symbol)
	transpf = FormulaUtils::graphFuncsAndAggs(transpf, _structure, getOption(BoolType::CPSUPPORT) && not recursive(pf), _context._funccontext);

	if (transpf != temppf) { // NOTE: the rewriting changed the atom
		Assert(_context._component != CompContext::HEAD);
		descend(transpf);
		deleteDeep(transpf);
		return;
	}

	auto newpf = dynamic_cast<PredForm*>(transpf);

	// Create grounders for the subterms
	vector<TermGrounder*> subtermgrounders;
	vector<SortTable*> argsorttables;
	SaveContext();// FIXME why shouldnt savecontext always be accompanied by checking the tseitin type for defined symbols?
	   	   	   	   // FIXME and why only in some cases check the sign of the formula for inverting the type?
	for (size_t n = 0; n < newpf->subterms().size(); ++n) {
		descend(newpf->subterms()[n]);
		subtermgrounders.push_back(getTermGrounder());
		argsorttables.push_back(_structure->inter(newpf->symbol()->sorts()[n]));
	}
	RestoreContext();

	// Create checkers and grounder
	if (getOption(BoolType::CPSUPPORT) and not recursive(newpf)) {
		Assert(not recursive(newpf) and _context._component != CompContext::HEAD); // Note: CP does not work in the defined case
		TermGrounder* lefttermgrounder;
		TermGrounder* righttermgrounder;
		CompType comp;
		bool useComparisonGrounder = false;
		if (VocabularyUtils::isIntComparisonPredicate(newpf->symbol(), _structure->vocabulary())) {
			useComparisonGrounder = true;
			Assert(subtermgrounders.size() == 2);
			comp = getCompType(newpf->symbol());
			if (isNeg(newpf->sign())) {
				comp = negateComp(comp);
			}
			lefttermgrounder = subtermgrounders[0];
			righttermgrounder = subtermgrounders[1];
		} else if (isa<Function>(*(newpf->symbol()))) {
			auto function = dynamic_cast<Function*>(newpf->symbol());
			if (CPSupport::eligibleForCP(function,_structure->vocabulary())) {
				useComparisonGrounder = true;
				comp = CompType::EQ;
				if (isNeg(newpf->sign())) {
					comp = negateComp(comp);
				}
				righttermgrounder = subtermgrounders.back();
				subtermgrounders.pop_back();
				auto ftable = _structure->inter(function)->funcTable();
				auto domain = _structure->inter(function->outsort());
				lefttermgrounder = new FuncTermGrounder(getGrounding()->translator(), function, ftable, domain, getArgTables(function, _structure), subtermgrounders);
				//ftgrounder->setOrig(...) TODO
			}
		}
		if (useComparisonGrounder) {
			SaveContext(); // FIXME why shouldnt savecontext always be accompanied by checking the tseitin type for defined symbols?
						   // FIXME and why only in some cases check the sign of the formula for inverting the type?
			if (recursive(newpf)) {
				_context._tseitin = TsType::RULE;
			}
			_formgrounder = new ComparisonGrounder(getGrounding(), lefttermgrounder, comp, righttermgrounder, _context);
			_formgrounder->setOrig(newpf, varmapping());
			RestoreContext();

			// FIXME what if CompContext is HEAD?

			if (_context._component == CompContext::SENTENCE) {
				_topgrounder = getFormGrounder();
			}
			deleteDeep(newpf);
			return;
		}
		
	}

	if (_context._component == CompContext::HEAD) {
		auto inter = _structure->inter(newpf->symbol());
		_headgrounder = new HeadGrounder(getGrounding(), inter->ct(), inter->cf(), newpf->symbol(), subtermgrounders, argsorttables);
	} else {
		GeneratorData data;
		for (auto it = newpf->subterms().cbegin(); it != newpf->subterms().cend(); ++it) {
			data.containers.push_back(new const DomElemContainer());
			data.tables.push_back(_structure->inter((*it)->sort()));
			data.fovars.push_back(new Variable((*it)->sort()));
		}
		data.pattern = vector<Pattern>(data.containers.size(), Pattern::INPUT);
		auto genpf = newpf;
		if (getOption(BoolType::GROUNDWITHBOUNDS)) {
			auto foterms = TermUtils::makeNewVarTerms(data.fovars);
			genpf = new PredForm(newpf->sign(), newpf->symbol(), foterms, FormulaParseInfo());
		}
		auto possTrueChecker = getChecker(genpf, TruthType::POSS_TRUE, data);
		auto certTrueChecker = getChecker(genpf, TruthType::CERTAIN_TRUE, data);
		if (genpf != newpf) {
			deleteDeep(genpf);
		}

		deleteList(data.fovars);

		_formgrounder = new AtomGrounder(getGrounding(), newpf->sign(), newpf->symbol(), subtermgrounders, data.containers, possTrueChecker, certTrueChecker,
				_structure->inter(newpf->symbol()), argsorttables, _context);
		_formgrounder->setOrig(newpf, varmapping());
	}
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = getFormGrounder();
	}
	deleteDeep(newpf);
}

void GrounderFactory::visit(const BoolForm* bf) {
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && (bf->isConjWithSign() || bf->subformulas().size() == 1);

	// If a disjunction or conj with one subformula, it's subformula can be treated as if it was the root of this formula
	if (isPos(bf->sign()) && bf->subformulas().size() == 1) {
		descend(bf->subformulas()[0]);
	} else {
		if (_context._conjPathUntilNode) {
			createBoolGrounderConjPath(bf);
		} else {
			createBoolGrounderDisjPath(bf);
		}
	}
}

ClauseGrounder* createB(AbstractGroundTheory* grounding, vector<Grounder*> sub, const set<Variable*>& freevars, SIGN sign, bool conj,
		const GroundingContext& context, bool trydelay, bool recursive) {
	auto mightdolazy = (not conj && context._monotone == Context::POSITIVE) || (conj && context._monotone == Context::NEGATIVE);
	if (context._monotone == Context::BOTH) {
		mightdolazy = true;
	}
	if (not trydelay) {
		mightdolazy = false;
	}
	if (not getOption(TSEITINDELAY) || recursive) {
			// TODO tseitin introduction in inductive definitions is currently not supported (cannot use the incremental clause for that)
		mightdolazy = false;
	}
	if (getOption(BoolType::GROUNDLAZILY) && isa<SolverTheory>(*grounding) && mightdolazy) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		return new LazyBoolGrounder(freevars, solvertheory, sub, SIGN::POS, conj, context);
	} else {
		return new BoolGrounder(grounding, sub, sign, conj, context);
	}
}

// Handle a top-level conjunction without creating tseitin atoms
void GrounderFactory::createBoolGrounderConjPath(const BoolForm* bf) {
	Assert(_context._component == CompContext::SENTENCE);
	// NOTE: to reduce the number of created tseitins, if bf is a negated disjunction, push the negation one level deeper.
	// Take a clone to avoid changing bf;
	auto newbf = bf->clone();
	if (not newbf->conj()) {
		newbf->conj(true);
		newbf->negate();
		for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
			(*it)->negate();
		}
	}

	// Visit the subformulas
	vector<Grounder*> sub;
	for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
		descend(*it);
		sub.push_back(getTopGrounder());
	}

	bool somequant = false;
	for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
		if (dynamic_cast<QuantForm*>(*i) != NULL) {
			somequant = true;
		}
	}

	auto boolgrounder = createB(getGrounding(), sub, newbf->freeVars(), newbf->sign(), true, _context, somequant, recursive(bf));
	boolgrounder->setOrig(bf, varmapping());
	_topgrounder = boolgrounder;
	deleteDeep(newbf);
}

// Formula bf is not a top-level conjunction
void GrounderFactory::createBoolGrounderDisjPath(const BoolForm* bf) {
	// Create grounders for subformulas
	SaveContext();
	DeeperContext(bf->sign());
	vector<Grounder*> sub;
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		descend(*it);
		sub.push_back(getFormGrounder());
	}
	RestoreContext();

	// Create grounder
	SaveContext();
	if (recursive(bf)) {
		_context._tseitin = TsType::RULE;
	}

	bool somequant = false;
	for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
		if (dynamic_cast<QuantForm*>(*i) != NULL) {
			somequant = true;
		}
	}

	_formgrounder = createB(getGrounding(), sub, bf->freeVars(), bf->sign(), bf->conj(), _context, somequant, recursive(bf));
	RestoreContext();
	_formgrounder->setOrig(bf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = getFormGrounder();
	}
}

void GrounderFactory::visit(const QuantForm* qf) {
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && qf->isUnivWithSign();

	// Create instance generator
	Formula* newsubformula = qf->subformula()->clone();
	// !x phi(x) => generate all x possibly false
	// !x phi(x) => check for x certainly false
	auto gc = createVarsAndGenerators(newsubformula, qf, qf->isUnivWithSign() ? TruthType::POSS_FALSE : TruthType::POSS_TRUE,
			qf->isUnivWithSign() ? TruthType::CERTAIN_FALSE : TruthType::CERTAIN_TRUE);

	// Handle a top-level conjunction without creating tseitin atoms
	_context.gentype = qf->isUnivWithSign() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
	if (_context._conjunctivePathFromRoot) {
		createTopQuantGrounder(qf, newsubformula, gc);
	} else {
		createNonTopQuantGrounder(qf, newsubformula, gc);
	}
	deleteDeep(newsubformula);
}

ClauseGrounder* createQ(AbstractGroundTheory* grounding, FormulaGrounder* subgrounder, QuantForm const * const qf, const GenAndChecker& gc,
		const GroundingContext& context, bool recursive) {
	bool conj = qf->quant() == QUANT::UNIV;
	bool mightdolazy = (not conj && context._monotone == Context::POSITIVE) || (conj && context._monotone == Context::NEGATIVE);
	if (context._monotone == Context::BOTH) {
		mightdolazy = true;
	}
	if (not getOption(TSEITINDELAY) || recursive) {
			// FIXME tseitin introduction in inductive definition is currently not supported (cannot use incremental clause for that)
		mightdolazy = false;
	}
	ClauseGrounder* grounder = NULL;
	if (getOption(BoolType::GROUNDLAZILY) && isa<SolverTheory>(*grounding) && mightdolazy) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		grounder = new LazyQuantGrounder(qf->freeVars(), solvertheory, subgrounder, qf->sign(), qf->quant(), gc._generator, gc._checker, context);
	} else {
		grounder = new QuantGrounder(grounding, subgrounder, qf->sign(), qf->quant(), gc._generator, gc._checker, context);
	}
	return grounder;
}

void GrounderFactory::createTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc) {
	// NOTE: to reduce the number of tseitins created, negations are pushed deeper whenever relevant:
	// If qf is a negated exist, push the negation one level deeper. Take a clone to avoid changing qf;
	QuantForm* tempqf = NULL;
	if (not qf->isUniv() && qf->sign() == SIGN::NEG) {
		tempqf = qf->clone();
		tempqf->quant(QUANT::UNIV);
		tempqf->negate();
		subformula->negate();
	}
	auto newqf = tempqf == NULL ? qf : tempqf;

	// Search here to check whether to prevent lower searches, but repeat the search later on on the ground-ready formula
	const PredForm* delayablepf = NULL;
	const PredForm* twindelayablepf = NULL;
	if (not getOption(SATISFIABILITYDELAY)) {
		_context._allowDelaySearch = false;
	}
	if (getOption(BoolType::GROUNDLAZILY) && getOption(SATISFIABILITYDELAY) && getContext()._allowDelaySearch) {
		auto lazycontext = Context::BOTH;
		auto tuple = FormulaUtils::findDoubleDelayLiteral(newqf, _structure, getGrounding()->translator(), lazycontext);
		if (tuple.size() != 2) {
			delayablepf = FormulaUtils::findUnknownBoundLiteral(newqf, _structure, getGrounding()->translator(), lazycontext);
		} else {
			delayablepf = tuple[0];
			twindelayablepf = tuple[1];
		}
	}
	if (delayablepf != NULL) {
		_context._allowDelaySearch = false;
	}

	// Visit subformula
	SaveContext();
	descend(subformula);
	RestoreContext();

	auto subgrounder = dynamic_cast<FormulaGrounder*>(_topgrounder);
	Assert(subgrounder!=NULL);

	FormulaGrounder* grounder = NULL;
	if (delayablepf != NULL) {
		_context._allowDelaySearch = true;
	}
	if (getOption(BoolType::GROUNDLAZILY) && getOption(SATISFIABILITYDELAY) && getContext()._allowDelaySearch) {
		// TODO issue: subformula might get new variables, but there are not reflected in newq, so the varmapping will not contain them (even if the varmapping is not clean when going back up (which is still done))!
		//  one example is when functions are unnested

		auto latestqf = QuantForm(newqf->sign(), newqf->quant(), newqf->quantVars(), subformula, newqf->pi());

		// Research to get up-to-date predforms!
		Context lazycontext = Context::BOTH;

		auto tuple = FormulaUtils::findDoubleDelayLiteral(&latestqf, _structure, getGrounding()->translator(), lazycontext);
		if (tuple.size() != 2) {
			delayablepf = FormulaUtils::findUnknownBoundLiteral(&latestqf, _structure, getGrounding()->translator(), lazycontext);
		} else {
			delayablepf = tuple[0];
			twindelayablepf = tuple[1];
		}

		if (delayablepf != NULL) {
			if (twindelayablepf != NULL) {
				auto terms = delayablepf->args();
				terms.insert(terms.end(), twindelayablepf->args().cbegin(), twindelayablepf->args().cend());
				grounder = new LazyTwinDelayUnivGrounder(delayablepf->symbol(), terms, lazycontext, varmapping(), getGrounding(), subgrounder, getContext());
			} else {
				grounder = new LazyUnknUnivGrounder(delayablepf, lazycontext, varmapping(), getGrounding(), subgrounder, getContext());
			}
		}
	}
	if (grounder == NULL) {
		grounder = createQ(getGrounding(), subgrounder, newqf, gc, getContext(), recursive(newqf));
	}
	Assert(grounder!=NULL);

	grounder->setMaxGroundSize(gc._universe.size() * subgrounder->getMaxGroundSize());
	grounder->setOrig(qf, varmapping());

	_topgrounder = grounder;

	if (tempqf != NULL) {
		deleteDeep(tempqf);
	}
}

void GrounderFactory::createNonTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc) {
	// Create grounder for subformula
	SaveContext();
	DeeperContext(qf->sign());
	descend(subformula);
	RestoreContext();

	// Create the grounder
	SaveContext();
	if (recursive(qf)) {
		_context._tseitin = TsType::RULE;
	}

	auto subsize = _formgrounder->getMaxGroundSize();
	_formgrounder = createQ(getGrounding(), _formgrounder, qf, gc, getContext(), recursive(qf));
	_formgrounder->setMaxGroundSize(gc._universe.size() * subsize);

	RestoreContext();

	_formgrounder->setOrig(qf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
}

const FOBDD* GrounderFactory::improve(bool approxastrue, const FOBDD* bdd, const vector<Variable*>& fovars) {
	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		clog << tabs() << "improving the following " << (approxastrue ? "maketrue" : "makefalse") << " BDD:" << "\n";
		pushtab();
		clog << tabs() << toString(bdd) << "\n";
	}
	auto manager = _symstructure->manager();

	// Optimize the query
	FOBDDManager optimizemanager;
	auto copybdd = optimizemanager.getBDD(bdd, manager);

	set<const FOBDDVariable*, CompareBDDVars> copyvars;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable(*it));
	}
	optimizemanager.optimizeQuery(copybdd, copyvars, { }, _structure);

	// Remove certain leaves
	const FOBDD* pruned = NULL;
	auto mcpa = 1; // TODO experiment with variations?
	if (approxastrue) {
		pruned = optimizemanager.makeMoreTrue(copybdd, copyvars, { }, _structure, mcpa);
	} else {
		pruned = optimizemanager.makeMoreFalse(copybdd, copyvars, { }, _structure, mcpa);
	}

	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		poptab();
		clog << tabs() << "Resulted in:" << "\n";
		pushtab();
		clog << tabs() << toString(pruned) << "\n";
		poptab();
	}

	return manager->getBDD(pruned, &optimizemanager);
}

void GrounderFactory::visit(const EquivForm* ef) {
	// Create grounders for the subformulas
	SaveContext();
	DeeperContext(ef->sign());
	_context._funccontext = Context::BOTH;
	_context._monotone = Context::BOTH;
	_context._tseitin = TsType::EQ;

	descend(ef->left());
	auto leftgrounder = getFormGrounder();
	descend(ef->right());
	auto rightgrounder = getFormGrounder();
	RestoreContext();

	// Create the grounder
	SaveContext();
	if (recursive(ef)) {
		_context._tseitin = TsType::RULE;
	}
	_formgrounder = new EquivGrounder(getGrounding(), leftgrounder, rightgrounder, ef->sign(), _context);
	RestoreContext();
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = getFormGrounder();
	}
}

Formula* trueFormula() {
	return new BoolForm(SIGN::POS, true, { }, FormulaParseInfo());
}

Formula* falseFormula() {
	return new BoolForm(SIGN::POS, false, { }, FormulaParseInfo());
}

void GrounderFactory::visit(const AggForm* af) {
	auto clonedaf = af->clone();

	// Rewrite card op func, card op var, sum op func, sum op var into sum op 0
	if (clonedaf->getBound()->type() == TermType::FUNC || clonedaf->getBound()->type() == TermType::VAR) {
		if (clonedaf->getAggTerm()->function() == AggFunction::CARD) {
			deleteDeep(clonedaf);
			clonedaf = new AggForm(af->sign(), af->getBound()->clone(), af->comp(),
					new AggTerm(af->getAggTerm()->set()->clone(), AggFunction::SUM, af->getAggTerm()->pi()), af->pi());
			for (auto i = clonedaf->getAggTerm()->set()->getSets().cbegin(); i < clonedaf->getAggTerm()->set()->getSets().cend(); ++i) {
				Assert((*i)->getTerm()->type()==TermType::DOM);
				Assert(dynamic_cast<DomainTerm*>((*i)->getTerm())->value()->type()==DomainElementType::DET_INT);
				Assert(dynamic_cast<DomainTerm*>((*i)->getTerm())->value()->value()._int==1);
			}
		}
		if (clonedaf->getAggTerm()->function() == AggFunction::SUM) {
			auto minus = get(STDFUNC::UNARYMINUS, { get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, _structure->vocabulary());
			auto newset = clonedaf->getAggTerm()->set()->clone();
			newset->addSubSet(new QuantSetExpr( { }, trueFormula(), new FuncTerm(minus, { clonedaf->getBound()->clone() }, TermParseInfo()), SetParseInfo()));
			auto temp = new AggForm(clonedaf->sign(), new DomainTerm(get(STDSORT::NATSORT), createDomElem(0), TermParseInfo()), clonedaf->comp(),
					new AggTerm(newset, clonedaf->getAggTerm()->function(), clonedaf->getAggTerm()->pi()), clonedaf->pi());
			deleteDeep(clonedaf);
			clonedaf = temp;
		}
	}

	Formula* transaf = FormulaUtils::unnestThreeValuedTerms(clonedaf->clone(), _structure, _context._funccontext,
			getOption(CPSUPPORT) && not recursive(clonedaf)); // TODO recursive could be more fine-grained (unnest any not rec defined symbol)
	transaf = FormulaUtils::graphFuncsAndAggs(transaf, _structure, getOption(CPSUPPORT) && not recursive(clonedaf), _context._funccontext);
	if (recursive(transaf)) {
		transaf = FormulaUtils::splitIntoMonotoneAgg(transaf);
	}

	if (not isa<AggForm>(*transaf)) { // The rewriting changed the atom
		descend(transaf);
		deleteDeep(transaf);
		return;
	}

	auto newaf = dynamic_cast<AggForm*>(transaf);

	// Create grounder for the bound
	descend(newaf->getBound());
	auto boundgrounder = getTermGrounder();

	// Create grounder for the set
	SaveContext();
	if (recursive(newaf)) {
		Assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf));
	}
	DeeperContext((not FormulaUtils::isAntimonotone(newaf)) ? SIGN::POS : SIGN::NEG);
	descend(newaf->getAggTerm()->set());
	auto setgrounder = getSetGrounder();
	RestoreContext();

	// Create aggregate grounder
	SaveContext();
	if (recursive(newaf)) {
		_context._tseitin = TsType::RULE;
	}
	if (isNeg(newaf->sign())) {
		_context._tseitin = invertImplication(_context._tseitin);
	}
	_formgrounder = new AggGrounder(getGrounding(), _context, newaf->getAggTerm()->function(), setgrounder, boundgrounder, newaf->comp(), newaf->sign());
	RestoreContext();
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = getFormGrounder();
	}
	deleteDeep(transaf);
	deleteDeep(clonedaf);
}

void GrounderFactory::visit(const EqChainForm* ef) {
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot;
	Formula* f = ef->clone();
	f = FormulaUtils::splitComparisonChains(f, getGrounding()->vocabulary());
	descend(f);
	deleteDeep(f);
}

void GrounderFactory::visit(const VarTerm* t) {
	Assert(varmapping().find(t->var()) != varmapping().cend());
	_termgrounder = new VarTermGrounder(varmapping().find(t->var())->second);
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> subtermgrounders;
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		descend(*it);
		subtermgrounders.push_back(getTermGrounder());
	}

	// Create term grounder
	auto function = t->function();
	auto ftable = _structure->inter(function)->funcTable();
	auto domain = _structure->inter(function->outsort());
	if (getOption(BoolType::CPSUPPORT) and FuncUtils::isIntSum(function, _structure->vocabulary())) {
		if (is(function, STDFUNC::SUBSTRACTION)) {
			_termgrounder = new SumTermGrounder(getGrounding()->translator(), ftable, domain, subtermgrounders[0], subtermgrounders[1], ST_MINUS);
		} else {
			_termgrounder = new SumTermGrounder(getGrounding()->translator(), ftable, domain, subtermgrounders[0], subtermgrounders[1]);
		}
	} else if (getOption(BoolType::CPSUPPORT) and TermUtils::isTermWithIntFactor(t, _structure)) {
		if (TermUtils::isFactor(t->subterms()[0], _structure)) {
			_termgrounder = new TermWithFactorGrounder(getGrounding()->translator(), ftable, domain, subtermgrounders[0], subtermgrounders[1]);
		} else {
			_termgrounder = new TermWithFactorGrounder(getGrounding()->translator(), ftable, domain, subtermgrounders[1], subtermgrounders[0]);
		}
	} else {
		_termgrounder = new FuncTermGrounder(getGrounding()->translator(), function, ftable, domain, getArgTables(function, _structure), subtermgrounders);
	}
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const AggTerm* t) {
	Assert(SetUtils::approxTwoValued(t->set(),_structure) or getOption(BoolType::CPSUPPORT));

	// Create set grounder
	descend(t->set());

	// Compute domain
	SortTable* domain = NULL;
	if (getOption(BoolType::CPSUPPORT) and CPSupport::eligibleForCP(t,_structure)) {
		domain = TermUtils::deriveSmallerSort(t, _structure)->interpretation();
	}

	// Create term grounder
	_termgrounder = new AggTermGrounder(getGrounding()->translator(), t->function(), domain, getSetGrounder());
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<QuantSetGrounder*> subgrounders;
	SaveContext();
	AggContext();
	for (auto i = s->getSets().cbegin(); i < s->getSets().cend(); ++i) {
		descend(*i);
		Assert(_quantsetgrounder!=NULL);
		subgrounders.push_back(_quantsetgrounder);
	}
	RestoreContext();

	_setgrounder = new EnumSetGrounder(getGrounding()->translator(), subgrounders);
}

void GrounderFactory::visit(const QuantSetExpr* origqs) {
	// Move three-valued terms in the set expression: from term to condition
	auto transqs = SetUtils::unnestThreeValuedTerms(origqs->clone(), _structure, _context._funccontext, getOption(CPSUPPORT));
	if (not isa<QuantSetExpr>(*transqs)) {
		descend(transqs);
		return;
	}

	auto newqs = dynamic_cast<QuantSetExpr*>(transqs);
	auto newsubformula = newqs->getCondition()->clone();

	// NOTE: generator generates possibly true instances, checker checks the certainly true ones
	auto gc = createVarsAndGenerators(newsubformula, newqs, TruthType::POSS_TRUE, TruthType::CERTAIN_TRUE);

	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(newqs->getCondition());
	auto subgr = getFormGrounder();
	RestoreContext();

	// Create grounder for weight
	descend(newqs->getTerm());
	auto wgr = getTermGrounder();

	_quantsetgrounder = new QuantSetGrounder(getGrounding()->translator(), subgr, gc._generator, gc._checker, wgr);
	_setgrounder = _quantsetgrounder;
	delete newqs;
}

void GrounderFactory::visit(const Definition* def) {
	// Store defined predicates
	for (auto it = def->defsymbols().cbegin(); it != def->defsymbols().cend(); ++it) {
		_context._defined.insert(*it);
	}

	_context.currentDefID = def->getID();

	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for (auto it = def->rules().cbegin(); it != def->rules().cend(); ++it) {
		descend(*it);
		subgrounders.push_back(getRuleGrounder());
	}

	_topgrounder = new DefinitionGrounder(getGrounding(), subgrounders, _context);
}

void GrounderFactory::visit(const Rule* rule) {
	auto newrule = rule->clone();
	//FIXME: if negations are already pushed, this is too much work. But on the other hand, checking if they are pushed is as expensive as pushing them
	//However, pushing negations here is important to avoid errors such as {p <- ~~p} turning into {p <- ~q; q<- ~p}
	newrule->body(FormulaUtils::pushNegations(newrule->body()));
	newrule = DefinitionUtils::unnestThreeValuedTerms(newrule, _structure, _context._funccontext, getOption(CPSUPPORT));

	auto groundlazily = false;
	if (getOption(SATISFIABILITYDELAY)) {
		groundlazily = getGrounding()->translator()->canBeDelayedOn(newrule->head()->symbol(), Context::BOTH, _context.getCurrentDefID());
	}
	if (groundlazily) { // NOTE: lazy grounding cannot handle head terms containing nested variables
		newrule = DefinitionUtils::unnestHeadTermsContainingVars(newrule, _structure, _context._funccontext);
	}

	// Split the quantified variables in two categories:
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	varlist bodyvars, headvars;
	for (auto it = newrule->quantVars().cbegin(); it != newrule->quantVars().cend(); ++it) {
		if (groundlazily) {
			if (not newrule->head()->contains(*it)) {
				// NOTE: for lazygroundrules, we need a generator for all variables NOT occurring in the head!
				bodyvars.push_back(*it);
			} else {
				headvars.push_back(*it);
				createVarMapping(*it);
			}
		} else {
			if (newrule->body()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				headvars.push_back(*it);
			}
		}
	}
	InstGenerator *headgen = NULL, *bodygen = NULL;
	if (not groundlazily) {
		headgen = createVarMapAndGenerator(newrule->head(), headvars);
	}
	bodygen = createVarMapAndGenerator(newrule->body(), bodyvars);

	// Create head grounder
	SaveContext();
	_context._component = CompContext::HEAD;
	descend(newrule->head());
	auto headgrounder = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._funccontext = Context::NEGATIVE; // minimize truth value of rule bodies
	_context._monotone = Context::POSITIVE;
	_context.gentype = GenType::CANMAKETRUE; // body instance generator corresponds to an existential quantifier
	_context._component = CompContext::FORMULA;
	_context._tseitin = TsType::EQ; // NOTE: this is allowed, as for any formula, it is checked whether it contains defined symbols and in that case, it grounds as if TsType::RULE
	descend(newrule->body());
	auto bodygrounder = getFormGrounder();
	RestoreContext();

	// Create rule grounder
	if (recursive(newrule->body())) {
		_context._tseitin = TsType::RULE;
	}
	if (groundlazily) {
		_rulegrounder = new LazyRuleGrounder(rule, newrule->head()->args(), headgrounder, bodygrounder, bodygen, _context);
	} else {
		_rulegrounder = new FullRuleGrounder(rule, headgrounder, bodygrounder, headgen, bodygen, _context);
	}

	deleteDeep(newrule);
}

template<typename Object>
void checkGeneratorInfinite(InstChecker* gen, Object* original) {
	if (gen->isInfiniteGenerator()) {
		Warning::possiblyInfiniteGrounding(toString(original));
		throw IdpException("Infinite grounding");
	}
}

template<class VarList>
InstGenerator* GrounderFactory::createVarMapAndGenerator(const Formula* original, const VarList& vars) {
	vector<SortTable*> varsorts;
	vector<const DomElemContainer*> varcontainers;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		varcontainers.push_back(createVarMapping(*it));
		varsorts.push_back(structure()->inter((*it)->sort()));
	}
	auto gen = GeneratorFactory::create(varcontainers, varsorts, original);

	if (not getOption(BoolType::GROUNDWITHBOUNDS) && not getOption(BoolType::GROUNDLAZILY)) {
		checkGeneratorInfinite(gen, original);
	}
	return gen;
}

template<typename OrigConstruct>
GenAndChecker GrounderFactory::createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype) {
	vector<Variable*> fovars, quantfovars;
	vector<Pattern> pattern;
	for (auto it = orig->quantVars().cbegin(); it != orig->quantVars().cend(); ++it) {
		quantfovars.push_back(*it);
	}
	for (auto it = orig->freeVars().cbegin(); it != orig->freeVars().cend(); ++it) {
		fovars.push_back(*it);
	}

	auto data = getPatternAndContainers(quantfovars, fovars);
	auto generator = getGenerator(subformula, generatortype, data);
	auto checker = getChecker(subformula, checkertype, data);

	vector<SortTable*> directquanttables;
	for (auto it = orig->quantVars().cbegin(); it != orig->quantVars().cend(); ++it) {
		directquanttables.push_back(_structure->inter((*it)->sort()));
	}

	if (not getOption(BoolType::GROUNDWITHBOUNDS) && not getOption(BoolType::GROUNDLAZILY)) {
		checkGeneratorInfinite(generator, orig);
		checkGeneratorInfinite(checker, orig);
	}

	return GenAndChecker(data.containers, generator, checker, Universe(directquanttables));
}

GeneratorData GrounderFactory::getPatternAndContainers(std::vector<Variable*> quantfovars, std::vector<Variable*> remvars) {
	GeneratorData data;
	data.quantfovars = quantfovars;
	data.fovars = data.quantfovars;
	data.fovars.insert(data.fovars.end(), remvars.cbegin(), remvars.cend());
	for (auto i = data.fovars.cbegin(); i < data.fovars.cend(); ++i) {
		auto st = _structure->inter((*i)->sort());
		data.tables.push_back(st);
	}

	for (auto it = quantfovars.cbegin(); it != quantfovars.cend(); ++it) {
		auto d = createVarMapping(*it);
		data.containers.push_back(d);
		data.pattern.push_back(Pattern::OUTPUT);
	}
	for (auto it = remvars.cbegin(); it != remvars.cend(); ++it) {
		Assert(varmapping().find(*it) != varmapping().cend());
		// Should already have a varmapping
		data.containers.push_back(varmapping().at(*it));
		data.pattern.push_back(Pattern::INPUT);
	}
	return data;
}

InstGenerator* createGen(const std::string& name, TruthType type, const GeneratorData& data, PredTable* table, Formula* subformula, const std::vector<Pattern>& pattern){
	auto checker = GeneratorFactory::create(table, pattern, data.containers, Universe(data.tables), subformula);
	//In either case, the newly created tables are now useless: the bddtable is turned into a treeinstgenerator, the other are also useless
	delete (table);

	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << name <<" for " << toString(type) << ": \n" << tabs() << toString(checker) << "\n";
	}

	return checker;
}

PredTable* GrounderFactory::createTable(Formula* subformula, TruthType type, const std::vector<Variable*>& quantfovars, bool approxvalue, const GeneratorData& data){
	auto tempsubformula = subformula->clone();
	tempsubformula = FormulaUtils::unnestTerms(tempsubformula, getContext()._funccontext, _structure);
	tempsubformula = FormulaUtils::splitComparisonChains(tempsubformula);
	tempsubformula = FormulaUtils::graphFuncsAndAggs(tempsubformula, _structure, false, getContext()._funccontext);
	auto bdd = _symstructure->evaluate(tempsubformula, type); // !x phi(x) => generate all x possibly false
	bdd = improve(approxvalue, bdd, quantfovars);
	auto table = new PredTable(new BDDInternalPredTable(bdd, _symstructure->manager(), data.fovars, _structure), Universe(data.tables));
	deleteDeep(tempsubformula);
	return table;
}

InstGenerator* GrounderFactory::getGenerator(Formula* subformula, TruthType generatortype, const GeneratorData& data) {
	PredTable* gentable = NULL;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		gentable = createTable(subformula, generatortype, data.quantfovars, true, data);
	} else {
		gentable = TableUtils::createFullPredTable(Universe(data.tables));
	}
	return createGen("Generator", generatortype, data, gentable, subformula, data.pattern);
}

InstChecker* GrounderFactory::getChecker(Formula* subformula, TruthType checkertype, const GeneratorData& data) {
	PredTable* checktable = NULL;
	bool approxastrue = checkertype == TruthType::POSS_TRUE || checkertype == TruthType::POSS_FALSE;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		checktable = createTable(subformula, checkertype, {}, approxastrue, data);
	} else {
		if (approxastrue) {
			checktable = TableUtils::createFullPredTable(Universe(data.tables));
		} else {
			checktable = TableUtils::createPredTable(Universe(data.tables));
		}
	}
	return createGen("Checker", checkertype, data, checktable, subformula, std::vector<Pattern>(data.pattern.size(), Pattern::INPUT));
}

DomElemContainer* GrounderFactory::createVarMapping(Variable* const var) {
	Assert(varmapping().find(var)==varmapping().cend());
	_context._mappedvars.insert(var);
	auto d = new DomElemContainer();
	_varmapping[var] = d;
	return d;
}
