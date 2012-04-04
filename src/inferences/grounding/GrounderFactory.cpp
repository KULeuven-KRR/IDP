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
#include "visitors/TheoryMutatingVisitor.hpp"
#include "inferences/SolverConnection.hpp"

#include "generators/BasicGenerators.hpp"
#include "generators/TableGenerator.hpp"

#include "theory/TheoryUtils.hpp"

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFactory.hpp"

#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/PrintGroundPolicy.hpp"
#include "groundtheories/SolverTheory.hpp"

using namespace std;
using namespace rel_ops;

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

int getIDForUndefined() {
	return -1;
}

double MCPA = 1; // TODO: constant currently used when pruning bdds. Should be made context dependent

template<typename Grounding>
GrounderFactory::GrounderFactory(const GroundStructureInfo& data, Grounding* grounding)
		: _structure(data.partialstructure), _symstructure(data.symbolicstructure), _grounding(grounding) {

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
	_context._tseitin = (getOption(NBMODELS) != 1) ? TsType::EQ : TsType::IMPL;
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

/**
 *	void GrounderFactory::SaveContext()
 *	DESCRIPTION
 *		Pushes the current context on a stack
 */
void GrounderFactory::SaveContext() {
	_contextstack.push(_context);
	_context._mappedvars.clear();
}

/**
 * void GrounderFactory::RestoreContext()
 * DESCRIPTION
 *		Restores the context to the top of the stack and pops the stack.
 */
void GrounderFactory::RestoreContext() {
	for (auto it = _context._mappedvars.begin(); it != _context._mappedvars.end(); ++it) {
		auto found = _varmapping.find(*it);
		if (found != _varmapping.end()) {
			_varmapping.erase(found); // FIXME: this should be disabled currenlty for lazy grounding
		}
	}
	_context._mappedvars.clear();

	_context = _contextstack.top();
	_contextstack.pop();
}

/**
 * void GrounderFactory::DeeperContext(bool sign)
 * DESCRIPTION
 *		Adapts the context to go one level deeper, and inverting some values if sign is negative
 * PARAMETERS
 *		sign	- the sign
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
		_context._tseitin = reverseImplication(_context._tseitin);
	}
}

/**
 * void GrounderFactory::descend(Term* t)
 * DESCRIPTION
 *		Visits a child and ensures the context is saved before the visit and restored afterwards.
 * PARAMETERS
 *		child	- the child to be visited.
 */
template<typename T>
void GrounderFactory::descend(T* child) {
	SaveContext();
	_formgrounder = NULL;
	_termgrounder = NULL;
	_setgrounder = NULL;
	_headgrounder = NULL;
	_rulegrounder = NULL;
	_topgrounder = NULL;
	child->accept(this);
	RestoreContext();
}

// TODO it would be useful to be able to guarantee, given some theory, what properties hold for it
// e.g., if pushnegations has been done, some flag might indicate that.

/**
 * TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is not passed to a solver, but stored internally as a EcnfTheory.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory. TODO is this checked or guaranteed?
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 */
Grounder* GrounderFactory::create(const GroundInfo& data) {
	auto groundtheory = new GroundTheory<GroundPolicy>(data.theory->vocabulary(), data.partialstructure->clone());
	GrounderFactory g( { data.partialstructure, data.symbolicstructure }, groundtheory);
	data.theory->accept(&g);
	return g.getTopGrounder();
}
Grounder* GrounderFactory::create(const GroundInfo& data, InteractivePrintMonitor* monitor) {
	auto groundtheory = new GroundTheory<PrintGroundPolicy>(data.partialstructure->clone());
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator(), groundtheory->termtranslator());
	GrounderFactory g( { data.partialstructure, data.symbolicstructure }, groundtheory);
	data.theory->accept(&g);
	return g.getTopGrounder();
}

/**
 * TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is directly passed to the given solver.
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
	auto groundtheory = new SolverTheory(data.theory->vocabulary(), data.partialstructure->clone());
	groundtheory->initialize(solver, getOption(IntType::GROUNDVERBOSITY), groundtheory->termtranslator());
	GrounderFactory g( { data.partialstructure, data.symbolicstructure }, groundtheory);
	data.theory->accept(&g);
	auto grounder = g.getTopGrounder();
	SolverConnection::setTranslator(solver, grounder->getTranslator());
	return grounder;
}
/*Grounder* GrounderFactory::create(const GroundInfo& data, FZRewriter* printer) {
	auto groundtheory = new GroundTheory<SolverPolicy<FZRewriter> >(data.theory->vocabulary(), data.partialstructure->clone());
	groundtheory->initialize(printer, getOption(IntType::GROUNDVERBOSITY), groundtheory->termtranslator());
	GrounderFactory g( { data.partialstructure, data.symbolicstructure }, groundtheory);
	data.theory->accept(&g);
	return g.getTopGrounder();
}*/

SetGrounder* GrounderFactory::create(const SetExpr* set, const GroundStructureInfo& data, AbstractGroundTheory* grounding) {
	GrounderFactory g(data, grounding);
	set->accept(&g);
	return g.getSetGrounder();
}

/**
 * void GrounderFactory::visit(const Theory* theory)
 * DESCRIPTION
 *		Creates a grounder for a non-ground theory.
 * PARAMETERS
 *		theory	- the non-ground theory
 * POSTCONDITIONS
 *		_toplevelgrounder is equal to the created grounder
 */
void GrounderFactory::visit(const Theory* theory) {
	AbstractTheory* tmptheory = theory->clone();
	tmptheory = FormulaUtils::splitComparisonChains(tmptheory, _structure->vocabulary());

	Assert(not getOption(BoolType::GROUNDLAZILY) || not getOption(BoolType::CPSUPPORT));
	// TODO currently not both

	if (getOption(BoolType::GROUNDLAZILY)) {
		//	tmptheory = FormulaUtils::pushQuantifiers(tmptheory);
		// FIXME this introduces sometimes a boolform with only one subformula. This is not wrong, but seems to be no longer treated as toplevel then when it IS a conjunction.
		// so fix it on both places (do not make the boolform, but also treat it correctly if it happens)
	}

	tmptheory = FormulaUtils::graphFuncsAndAggs(tmptheory, _structure);

	Assert(sametypeid<Theory>(*tmptheory));
	auto newtheory = dynamic_cast<Theory*>(tmptheory);

	// Collect all components (sentences, definitions, and fixpoint definitions) of the theory
	const auto& components = newtheory->components(); // NOTE: primitive reorder present: definitions first
	// NOTE: currently, definitions first is important for good lazy grounding
	//TODO Order components the components to optimize the grounding process

	// Create grounders for all components
	vector<Grounder*> children;
	for (size_t n = 0; n < components.size(); ++n) {
		InitContext();

		if (getOption(IntType::GROUNDVERBOSITY) > 0) {
			clog << tabs() << "Creating a grounder for " << toString(components[n]) << "\n";
		}
		components[n]->accept(this);
		children.push_back(_topgrounder);
	}

	_topgrounder = new BoolGrounder(_grounding, children, SIGN::POS, true, _context);

	// Clean up: delete the theory clone.
	//TODO newtheory->recursiveDelete();
}

/**
 * void GrounderFactory::visit(const PredForm* pf)
 * DESCRIPTION
 *		Creates a grounder for an atomic formula.
 * PARAMETERS
 *		pf	- the atomic formula
 * PRECONDITIONS
 *		Each free variable that occurs in pf occurs in varmapping().
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::HEAD:		_headgrounder
 *			CompContext::FORMULA:	_formgrounder
 */
void GrounderFactory::visit(const PredForm* pf) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Grounderfactory visiting: " << toString(pf) << "\n";
		pushtab();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Move all functions and aggregates that are three-valued according
	// to _structure outside the atom. To avoid changing the original atom,
	// we first clone it.
	Formula* temppf = pf->clone();
	Formula* transpf = FormulaUtils::unnestThreeValuedTerms(temppf, _structure, _context._funccontext);
	// TODO can we delete temppf here if different from transpf? APPARANTLY NOT!
	transpf = FormulaUtils::graphFuncsAndAggs(transpf, _structure, _context._funccontext);

	if (not sametypeid<PredForm>(*transpf)) { // The rewriting changed the atom
		Assert(_context._component != CompContext::HEAD);
		transpf->accept(this);
		transpf->recursiveDelete();
		if (getOption(IntType::GROUNDVERBOSITY) > 3) {
			poptab();
		}
		return;
	}

	PredForm* newpf = dynamic_cast<PredForm*>(transpf);

	// Create grounders for the subterms
	vector<TermGrounder*> subtermgrounders;
	vector<SortTable*> argsorttables;
	SaveContext();
	for (size_t n = 0; n < newpf->subterms().size(); ++n) {
		descend(newpf->subterms()[n]);
		subtermgrounders.push_back(_termgrounder);
		argsorttables.push_back(_structure->inter(newpf->symbol()->sorts()[n]));
	}
	RestoreContext();

	// Create checkers and grounder
	if (getOption(BoolType::CPSUPPORT) && VocabularyUtils::isIntComparisonPredicate(newpf->symbol(),_structure->vocabulary())) {
		string name = newpf->symbol()->name();
		CompType comp;
		if (name == "=/2") {
			comp = isPos(pf->sign()) ? CompType::EQ : CompType::NEQ;
		} else if (name == "</2") {
			comp = isPos(pf->sign()) ? CompType::LT : CompType::GEQ;
		} else {
			Assert(name == ">/2");
			comp = isPos(pf->sign()) ? CompType::GT : CompType::LEQ;
		}

		SaveContext();
		if (recursive(newpf)) {
			_context._tseitin = TsType::RULE;
		}
		_formgrounder = new ComparisonGrounder(_grounding, _grounding->termtranslator(), subtermgrounders[0], comp, subtermgrounders[1], _context);
		_formgrounder->setOrig(newpf, varmapping());
		RestoreContext();

		if (_context._component == CompContext::SENTENCE) { // TODO Refactor outside (also other occurences)
			_topgrounder = _formgrounder;
		}
		newpf->recursiveDelete();

		if (getOption(IntType::GROUNDVERBOSITY) > 3) {
			poptab();
		}
		return;
	}

	if (_context._component == CompContext::HEAD) {
		PredInter* inter = _structure->inter(newpf->symbol());
		_headgrounder = new HeadGrounder(_grounding, inter->ct(), inter->cf(), newpf->symbol(), subtermgrounders, argsorttables);
		if (getOption(IntType::GROUNDVERBOSITY) > 3) {
			poptab();
		}
		newpf->recursiveDelete();
		return;
	}

	// Grounding basic predform

	// Create instance checkers
	vector<Sort*> checksorts;
	vector<const DomElemContainer*> checkargs; // Set by grounder, then used by checkers
	vector<SortTable*> tables;
	// NOTE: order is important!
	for (auto it = newpf->subterms().cbegin(); it != newpf->subterms().cend(); ++it) {
		checksorts.push_back((*it)->sort());
		checkargs.push_back(new const DomElemContainer());
		tables.push_back(_structure->inter((*it)->sort()));
	}

	PredTable *certTrueTable = NULL, *possTrueTable = NULL;
	if (getOption(BoolType::GROUNDWITHBOUNDS) && checksorts.size() > 0) { //TODO: didn't work for size 0, i.e. for propositional symbols.  Fix this!
		auto fovars = VarUtils::makeNewVariables(checksorts);
		auto foterms = TermUtils::makeNewVarTerms(fovars);
		auto checkpf = new PredForm(newpf->sign(), newpf->symbol(), foterms, FormulaParseInfo());
		const FOBDD* possTrueBdd = _symstructure->evaluate(checkpf, TruthType::POSS_TRUE);
		const FOBDD* certTrueBdd = _symstructure->evaluate(checkpf, TruthType::CERTAIN_TRUE);
		;
		possTrueTable = new PredTable(new BDDInternalPredTable(possTrueBdd, _symstructure->manager(), fovars, _structure), Universe(tables));
		certTrueTable = new PredTable(new BDDInternalPredTable(certTrueBdd, _symstructure->manager(), fovars, _structure), Universe(tables));
	} else {
		possTrueTable = new PredTable(new FullInternalPredTable(), Universe(tables));
		certTrueTable = new PredTable(new EnumeratedInternalPredTable(), Universe(tables));
	}

	auto possTrueChecker = GeneratorFactory::create(possTrueTable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables), pf);
	auto certTrueChecker = GeneratorFactory::create(certTrueTable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables), pf);
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Possible checker: \n" << tabs() << toString(possTrueChecker) << "\n";
		clog << tabs() << "Certain checker: \n" << tabs() << toString(certTrueChecker) << "\n";
	}

	_formgrounder = new AtomGrounder(_grounding, newpf->sign(), newpf->symbol(), subtermgrounders, checkargs, possTrueChecker, certTrueChecker,
			_structure->inter(newpf->symbol()), argsorttables, _context);

	_formgrounder->setOrig(newpf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
	//newpf->recursiveDelete(); //TODO: this is suspicious since the bdds use variables from newpf...
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}
}

/**
 * Recursively deletes an object AND sets its reference to NULL (causing sigsev when calling it later, instead of not failing at all)
 * TODO should use this throughout the system
 */
template<class T>
void deleteDeep(T& object) {
	object->recursiveDelete();
	object = NULL;
}

/**
 * void GrounderFactory::visit(const BoolForm* bf)
 * DESCRIPTION
 *		Creates a grounder for a conjunction or disjunction of formulas
 * PARAMETERS
 *		bf	- the conjunction or disjunction
 * PRECONDITIONS
 *		Each free variable that occurs in bf occurs in varmapping().
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:	_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Grounderfactory visiting: " << toString(bf) << "\n";
		pushtab();
	}

	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && (bf->isConjWithSign() || bf->subformulas().size() == 1);

	// If a disjunction or conj with one subformula, it's subformula can be treated as if it was the root of this formula
	if (isPos(bf->sign()) && bf->subformulas().size() == 1) {
		bf->subformulas()[0]->accept(this);
	} else {
		if (_context._conjPathUntilNode) {
			createBoolGrounderConjPath(bf);
		} else {
			createBoolGrounderDisjPath(bf);
		}
	}

	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}
}

ClauseGrounder* createB(AbstractGroundTheory* grounding, vector<Grounder*> sub, const set<Variable*>& freevars, SIGN sign, bool conj,
		const GroundingContext& context, bool trydelay) {
	bool mightdolazy = (not conj && context._monotone == Context::POSITIVE) || (conj && context._monotone == Context::NEGATIVE);
	if (context._tseitin == TsType::RULE) { // TODO currently, the many restarts of the SCC detection etc. are too expensive!
		mightdolazy = false;
	}
	if (context._monotone == Context::BOTH) {
		mightdolazy = true;
	}
	if(not trydelay){
		mightdolazy = false;
	}
	if(not getOption(TSEITINDELAY)){
		mightdolazy = false;
	}
	if (getOption(BoolType::GROUNDLAZILY) && sametypeid<SolverTheory>(*grounding) && mightdolazy) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		return new LazyBoolGrounder(freevars, solvertheory, sub, SIGN::POS, conj, context);
	} else {
		return new BoolGrounder(grounding, sub, sign, conj, context);
	}
}

// Handle a top-level conjunction without creating tseitin atoms
void GrounderFactory::createBoolGrounderConjPath(const BoolForm* bf) {
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
		SaveContext();
		descend(*it);
		RestoreContext();
		sub.push_back(_topgrounder);
	}

	bool somequant = false;
	for(auto i=bf->subformulas().cbegin(); i<bf->subformulas().cend(); ++i){
		if(dynamic_cast<QuantForm*>(*i)!=NULL){
			somequant = true;
		}
	}

	auto boolgrounder = createB(_grounding, sub, newbf->freeVars(), newbf->sign(), true, _context, somequant);
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
		sub.push_back(_formgrounder);
		//TODO: here we could check for true/false formulas.  Useful?
	}
	RestoreContext();

	// Create grounder
	SaveContext();
	if (recursive(bf)) {
		_context._tseitin = TsType::RULE;
	}

	bool somequant = false;
	for(auto i=bf->subformulas().cbegin(); i<bf->subformulas().cend(); ++i){
		if(dynamic_cast<QuantForm*>(*i)!=NULL){
			somequant = true;
		}
	}

	_formgrounder = createB(_grounding, sub, bf->freeVars(), bf->sign(), bf->conj(), _context, somequant);
	RestoreContext();
	_formgrounder->setOrig(bf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
}

/**
 * void GrounderFactory::visit(const QuantForm* qf)
 * DESCRIPTION
 *		Creates a grounder for a quantified formula
 * PARAMETERS
 *		qf	- the quantified formula
 * PRECONDITIONS
 *		Each free variable that occurs in qf occurs in varmapping().
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const QuantForm* qf) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Grounderfactory visiting: " << toString(qf) << "\n";
		pushtab();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && qf->isUnivWithSign();

	// Create instance generator
	Formula* newsubformula = qf->subformula()->clone();
	newsubformula = FormulaUtils::unnestThreeValuedTerms(newsubformula, _structure, _context._funccontext);
	newsubformula = FormulaUtils::graphFuncsAndAggs(newsubformula, _structure, _context._funccontext);

	// NOTE: if the checker return valid, then the value of the formula can be decided from the value of the checked instantiation
	//	for universal: checker valid => formula false, for existential: checker valid => formula true

	// !x phi(x) => generate all x possibly false
	// !x phi(x) => check for x certainly false
	GenAndChecker gc = createVarsAndGenerators(newsubformula, qf, qf->isUnivWithSign() ? TruthType::POSS_FALSE : TruthType::POSS_TRUE,
			qf->isUnivWithSign() ? TruthType::CERTAIN_FALSE : TruthType::CERTAIN_TRUE);

	// Handle a top-level conjunction without creating tseitin atoms
	_context.gentype = qf->isUnivWithSign() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
	if (_context._conjunctivePathFromRoot) {
		createTopQuantGrounder(qf, newsubformula, gc);
	} else {
		createNonTopQuantGrounder(qf, newsubformula, gc);
	}
	// TODO newsubformula->recursiveDelete();
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}
}

void checkGeneratorInfinite(InstChecker* gen) {
	if (gen->isInfiniteGenerator()) {
		/*if (original != NULL) { // TODO
		 Warning::possiblyInfiniteGrounding(original->pi().userDefined() ? toString(original->pi().originalobject()) : "", toString(original));
		 }*/
		throw IdpException("Infinite grounding");
	}
}

ClauseGrounder* createQ(AbstractGroundTheory* grounding, FormulaGrounder* subgrounder, SIGN sign, QUANT quant, const set<Variable*>& freevars,
		const GenAndChecker& gc, const GroundingContext& context) {
	bool conj = quant == QUANT::UNIV;
	bool mightdolazy = (not conj && context._monotone == Context::POSITIVE) || (conj && context._monotone == Context::NEGATIVE);
	if (context._monotone == Context::BOTH) {
		mightdolazy = true;
	}
	if (context._tseitin == TsType::RULE) { // TODO currently, the many restarts of the SCC detection etc. are too expensive!
		mightdolazy = false;
	}
	if(not getOption(TSEITINDELAY)){
		mightdolazy = false;
	}
	ClauseGrounder* grounder = NULL;
	if (getOption(BoolType::GROUNDLAZILY) && sametypeid<SolverTheory>(*grounding) && mightdolazy) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		grounder = new LazyQuantGrounder(freevars, solvertheory, subgrounder, sign, quant, gc._generator, gc._checker, context);
	} else {
		if (not getOption(BoolType::GROUNDWITHBOUNDS)) {
			// If not grounding with bounds, we will certainly ground infinitely, so do not even start
			checkGeneratorInfinite(gc._generator);
			checkGeneratorInfinite(gc._checker);
		}
		grounder = new QuantGrounder(grounding, subgrounder, sign, quant, gc._generator, gc._checker, context);
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
	if(not getOption(SATISFIABILITYDELAY)){
		_context._allowDelaySearch = false;
	}
	if (getOption(BoolType::GROUNDLAZILY) && getOption(SATISFIABILITYDELAY) && getContext()._allowDelaySearch) {
		Context lazycontext = Context::BOTH;
		// FIXME BUG BUG: for functions, should ALSO consider the implicit function constraint!!!!!
		auto tuple = FormulaUtils::findDoubleDelayLiteral(newqf, _structure, _grounding->translator(), lazycontext);
		if (tuple.size()!=2) {
			delayablepf = FormulaUtils::findUnknownBoundLiteral(newqf, _structure, _grounding->translator(), lazycontext);
		} else {
			delayablepf = tuple[0];
			twindelayablepf = tuple[1];
		}
	}
	if(delayablepf!=NULL){
		_context._allowDelaySearch = false;
	}

	// Visit subformula
	SaveContext();
	descend(subformula);
	RestoreContext();

	auto subgrounder = dynamic_cast<FormulaGrounder*>(_topgrounder);
	Assert(subgrounder!=NULL);

	FormulaGrounder* grounder = NULL;
	if(delayablepf!=NULL){
		_context._allowDelaySearch = true;
	}
	if (getOption(BoolType::GROUNDLAZILY) && getOption(SATISFIABILITYDELAY) && getContext()._allowDelaySearch) {
		// TODO issue: subformula might get new variables, but there are not reflected in newq, so the varmapping will not contain them (even if the varmapping is not clean when going back up (which is still done))!
		//  one example is when functions are unnested

		auto latestqf = QuantForm(newqf->sign(), newqf->quant(), newqf->quantVars(), subformula, newqf->pi());

		// Research to get up-to-date predforms!
		Context lazycontext = Context::BOTH;
		auto tuple = FormulaUtils::findDoubleDelayLiteral(&latestqf, _structure, _grounding->translator(), lazycontext);
		if (tuple.size()!=2) {
			delayablepf = FormulaUtils::findUnknownBoundLiteral(&latestqf, _structure, _grounding->translator(), lazycontext);
		} else {
			delayablepf = tuple[0];
			twindelayablepf = tuple[1];
		}

		if (delayablepf != NULL) {
			if (twindelayablepf != NULL) {
				auto terms = delayablepf->args();
				terms.insert(terms.end(), twindelayablepf->args().cbegin(), twindelayablepf->args().cend());
				grounder = new LazyTwinDelayUnivGrounder(delayablepf->symbol(), terms, lazycontext, varmapping(), _grounding, subgrounder, getContext());
			} else {
				grounder = new LazyUnknUnivGrounder(delayablepf, lazycontext, varmapping(), _grounding, subgrounder, getContext());
			}
		}
	}
	if (grounder == NULL) {
		grounder = createQ(_grounding, subgrounder, newqf->sign(), newqf->quant(), newqf->freeVars(), gc, getContext());
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
	_formgrounder = createQ(_grounding, _formgrounder, qf->sign(), qf->quant(), qf->freeVars(), gc, getContext());
	_formgrounder->setMaxGroundSize(gc._universe.size() * subsize);

	RestoreContext();

	_formgrounder->setOrig(qf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
	//newsubformula->recursiveDelete();
}

const FOBDD* GrounderFactory::improveGenerator(const FOBDD* bdd, const vector<Variable*>& fovars, double mcpa) {
	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		clog << tabs() << "improving the following (generator) BDD:" << "\n";
		pushtab();
		clog << tabs() << toString(bdd) << "\n";
	}
	auto manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	auto copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable(*it));
	}
	optimizemanager.optimizeQuery(copybdd, copyvars, indices, _structure);

	// 2. Remove certain leaves
	auto pruned = optimizemanager.makeMoreTrue(copybdd, copyvars, indices, _structure, mcpa);

	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		poptab();
		clog << tabs() << "Resulted in:" << "\n";
		pushtab();
		clog << tabs() << toString(pruned) << "\n";
		poptab();
	}
	// 3. Replace result
	return manager->getBDD(pruned, &optimizemanager);
}

const FOBDD* GrounderFactory::improveChecker(const FOBDD* bdd, double mcpa) {
	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		clog << tabs() << "improving the following (checker) BDD:" << "\n";
		pushtab();
		clog << tabs() << toString(bdd) << "\n";
	}
	auto manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	auto copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizeQuery(copybdd, copyvars, indices, _structure);

	// 2. Remove certain leaves
	auto pruned = optimizemanager.makeMoreFalse(copybdd, copyvars, indices, _structure, mcpa);

	if (getOption(IntType::GROUNDVERBOSITY) > 5) {
		poptab();
		clog << tabs() << "Resulted in:" << "\n";
		pushtab();
		clog << tabs() << toString(pruned) << "\n";
		poptab();
	}

	// 3. Replace result
	return manager->getBDD(pruned, &optimizemanager);
}

/**
 * void GrounderFactory::visit(const EquivForm* ef)
 * DESCRIPTION
 *		Creates a grounder for an equivalence.
 * PARAMETERS
 *		ef	- the equivalence
 * PRECONDITIONS
 *		Each free variable that occurs in ef occurs in varmapping().
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const EquivForm* ef) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;
	// Create grounders for the subformulas
	SaveContext();
	DeeperContext(ef->sign());
	_context._funccontext = Context::BOTH;
	_context._monotone = Context::BOTH;
	_context._tseitin = TsType::EQ;

	descend(ef->left());
	auto leftgrounder = _formgrounder;
	descend(ef->right());
	auto rightgrounder = _formgrounder;
	RestoreContext();

	// Create the grounder
	SaveContext();
	if (recursive(ef)) {
		_context._tseitin = TsType::RULE;
	}/*else{
		_context._tseitin = TsType::EQ;
	}*/
	_formgrounder = new EquivGrounder(_grounding, leftgrounder, rightgrounder, ef->sign(), _context);
	RestoreContext();
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
}

void GrounderFactory::visit(const AggForm* af) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	Formula* transaf = FormulaUtils::unnestThreeValuedTerms(af->clone(), _structure, _context._funccontext);
	transaf = FormulaUtils::graphFuncsAndAggs(transaf, _structure, _context._funccontext);
	if (recursive(transaf)) {
		transaf = FormulaUtils::splitIntoMonotoneAgg(transaf);
	}

	if (not sametypeid<AggForm>(*transaf)) { // The rewriting changed the atom
		transaf->accept(this);
	} else { // The rewriting did not change the atom
		auto newaf = dynamic_cast<AggForm*>(transaf);

		// Create grounder for the bound
		descend(newaf->left());
		auto boundgrounder = _termgrounder;

		// Create grounder for the set
		SaveContext();
		if (recursive(newaf)) {
			Assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf));
		}
		DeeperContext((not FormulaUtils::isAntimonotone(newaf)) ? SIGN::POS : SIGN::NEG);
		descend(newaf->right()->set());
		auto setgrounder = _setgrounder;
		RestoreContext();

		// Create aggregate grounder
		SaveContext();
		if (recursive(newaf)) {
			_context._tseitin = TsType::RULE;
		}
		if (isNeg(newaf->sign())) {
			_context._tseitin = reverseImplication(_context._tseitin);
		}
		_formgrounder = new AggGrounder(_grounding, _context, newaf->right()->function(), setgrounder, boundgrounder, newaf->comp(), newaf->sign());
		RestoreContext();
		if (_context._component == CompContext::SENTENCE) {
			_topgrounder = _formgrounder;
		}
	}
	transaf->recursiveDelete();
}

void GrounderFactory::visit(const EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::splitComparisonChains(f, _grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

void GrounderFactory::visit(const VarTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	Assert(varmapping().find(t->var()) != varmapping().cend());
	_termgrounder = new VarTermGrounder(varmapping().find(t->var())->second);
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const DomainTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const FuncTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Create grounders for subterms
	vector<TermGrounder*> subtermgrounders;
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		(*it)->accept(this);
		if (_termgrounder) {
			subtermgrounders.push_back(_termgrounder);
		}
	}

	// Create term grounder
	auto function = t->function();
	auto ftable = _structure->inter(function)->funcTable();
	auto domain = _structure->inter(function->outsort());
	if (getOption(BoolType::CPSUPPORT) && FuncUtils::isIntSum(function, _structure->vocabulary())) {
		if (function->name() == "-/2") {
			_termgrounder = new SumTermGrounder(_grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1], ST_MINUS);
		} else {
			_termgrounder = new SumTermGrounder(_grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1]);
		}
	} else {
		_termgrounder = new FuncTermGrounder(_grounding->termtranslator(), function, ftable, domain, subtermgrounders);
	}
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const AggTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	Assert(getOption(BoolType::CPSUPPORT));

	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	_termgrounder = new AggTermGrounder(_grounding->translator(),_grounding->termtranslator(), t->function(), _setgrounder);
	_termgrounder->setOrig(t, varmapping());
}

void GrounderFactory::visit(const EnumSetExpr* s) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subfgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	AggContext();
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		descend(s->subformulas()[n]);
		subfgr.push_back(_formgrounder);
		descend(s->subterms()[n]);
		subtgr.push_back(_termgrounder);
	}
	RestoreContext();

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding->translator(), subfgr, subtgr);
}

template<typename OrigConstruct>
GenAndChecker GrounderFactory::createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype) {
	vector<const DomElemContainer*> vars;
	vector<SortTable*> tables;
	vector<Variable*> fovars, quantfovars;
	vector<Pattern> pattern;

	for (auto it = subformula->freeVars().cbegin(); it != subformula->freeVars().cend(); ++it) {
		if (orig->quantVars().find(*it) == orig->quantVars().cend()) { // It is a free var of the quantified formula
			Assert(varmapping().find(*it) != varmapping().cend());
			// So should already have a varmapping
			vars.push_back(varmapping().at(*it));
			pattern.push_back(Pattern::INPUT);
		} else { // It is a var quantified in the orig formula, so should create a new varmapping for it
			auto d = createVarMapping(*it);
			vars.push_back(d);
			pattern.push_back(Pattern::OUTPUT);
			quantfovars.push_back(*it);
		}
		fovars.push_back(*it);
		SortTable* st = _structure->inter((*it)->sort());
		tables.push_back(st);
	}

	// FIXME => unsafe to have to pass in fovars explicitly (order is never checked?)
	PredTable *gentable = NULL, *checktable = NULL;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		auto generatorbdd = _symstructure->evaluate(subformula, generatortype); // !x phi(x) => generate all x possibly false
		auto checkerbdd = _symstructure->evaluate(subformula, checkertype); // !x phi(x) => check for x certainly false
		generatorbdd = improveGenerator(generatorbdd, quantfovars, MCPA);
		checkerbdd = improveChecker(checkerbdd, MCPA);
		gentable = new PredTable(new BDDInternalPredTable(generatorbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
		checktable = new PredTable(new BDDInternalPredTable(checkerbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
	} else {
		gentable = new PredTable(new FullInternalPredTable(), Universe(tables));
		checktable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable), Universe(tables));
	}

	auto gen = GeneratorFactory::create(gentable, pattern, vars, Universe(tables), subformula);
	auto check = GeneratorFactory::create(checktable, vector<Pattern>(vars.size(), Pattern::INPUT), vars, Universe(tables), subformula);

	vector<SortTable*> directquanttables;
	for (auto it = orig->quantVars().cbegin(); it != orig->quantVars().cend(); ++it) {
		directquanttables.push_back(_structure->inter((*it)->sort()));
	}

	return GenAndChecker(vars, gen, check, Universe(directquanttables));
}

void GrounderFactory::visit(const QuantSetExpr* origqs) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Move three-valued terms in the set expression
	auto transqs = SetUtils::unnestThreeValuedTerms(origqs->clone(), _structure, _context._funccontext);
	if (not sametypeid<QuantSetExpr>(*transqs)) {
		transqs->accept(this);
		return;
	}

	auto newqs = dynamic_cast<QuantSetExpr*>(transqs);
	Formula* clonedsubformula = newqs->subformulas()[0]->clone();
	Formula* newsubformula = FormulaUtils::unnestThreeValuedTerms(clonedsubformula, _structure, Context::POSITIVE);
	newsubformula = FormulaUtils::graphFuncsAndAggs(newsubformula, _structure, _context._funccontext);

	// NOTE: generator generates possibly true instances, checker checks the certainly true ones
	GenAndChecker gc = createVarsAndGenerators(newsubformula, newqs, TruthType::POSS_TRUE, TruthType::CERTAIN_TRUE);

	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(newqs->subformulas()[0]);
	FormulaGrounder* subgr = _formgrounder;
	RestoreContext();

	// Create grounder for weight
	descend(newqs->subterms()[0]);
	TermGrounder* wgr = _termgrounder;

	// Create grounder
	if (not getOption(BoolType::GROUNDWITHBOUNDS)) {
		checkGeneratorInfinite(gc._generator);
		checkGeneratorInfinite(gc._checker);
	}
	_setgrounder = new QuantSetGrounder(_grounding->translator(), subgr, gc._generator, gc._checker, wgr);
}

/**
 * void GrounderFactory::visit(const Definition* def)
 * DESCRIPTION
 * 		Creates a grounder for a definition.
 */
void GrounderFactory::visit(const Definition* def) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Grounderfactory visiting: " << toString(def) << "\n";
		pushtab();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Store defined predicates
	for (auto it = def->defsymbols().cbegin(); it != def->defsymbols().cend(); ++it) {
		_context._defined.insert(*it);
	}

	_context.currentDefID = def->getID();

	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for (auto it = def->rules().cbegin(); it != def->rules().cend(); ++it) {
		descend(*it);
		subgrounders.push_back(_rulegrounder);
	}

	_topgrounder = new DefinitionGrounder(_grounding, subgrounders, _context);

	_context._defined.clear();
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}
}

template<class VarList>
InstGenerator* GrounderFactory::createVarMapAndGenerator(const Formula* original, const VarList& vars) {
	vector<SortTable*> hvst;
	vector<const DomElemContainer*> hvars;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		auto domelem = createVarMapping(*it);
		hvars.push_back(domelem);
		auto sorttable = structure()->inter((*it)->sort());
		hvst.push_back(sorttable);
	}
	GeneratorFactory gf;
	return gf.create(hvars, hvst, original);
}

DomElemContainer* GrounderFactory::createVarMapping(Variable* const var) {
	Assert(varmapping().find(var)==varmapping().cend());
	_context._mappedvars.insert(var);
	auto d = new DomElemContainer();
	_varmapping[var] = d;
	return d;
}

void GrounderFactory::visit(const Rule* rule) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << tabs() << "Grounderfactory visiting: " << toString(rule) << "\n";
		pushtab();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	auto temprule = rule->clone();
	auto newrule = DefinitionUtils::unnestThreeValuedTerms(temprule, _structure, _context._funccontext);
	if (getOption(BoolType::GROUNDLAZILY)) { // TODO currently, no support for lazy grounding rules with variables within functerms
		newrule = DefinitionUtils::unnestHeadTermsContainingVars(newrule, _structure, _context._funccontext);
	}
	// TODO apparently cannot safely delete temprule here, even if different from newrule
	InstGenerator *headgen = NULL, *bodygen = NULL;

	vector<Variable*> headvars;
	auto groundlazily = getOption(BoolType::GROUNDLAZILY)
			&& _grounding->translator()->canBeDelayedOn(newrule->head()->symbol(), Context::BOTH, _context.getCurrentDefID());
	if(not getOption(SATISFIABILITYDELAY)){
		groundlazily = false;
	}
	if (groundlazily) {
		Assert(sametypeid<SolverTheory>(*_grounding));
		// NOTE: for lazygroundrules, we need a generator for all variables NOT occurring in the head!
		varlist bodyvars;
		for (auto it = newrule->quantVars().cbegin(); it != newrule->quantVars().cend(); ++it) {
			if (not newrule->head()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				headvars.push_back(*it);
				createVarMapping(*it);
			}
		}

		bodygen = createVarMapAndGenerator(newrule->head(), bodyvars);
	} else {
		// Split the quantified variables in two categories:
		//		1. the variables that only occur in the head
		//		2. the variables that occur in the body (and possibly in the head)

		varlist headvars;
		varlist bodyvars;
		for (auto it = newrule->quantVars().cbegin(); it != newrule->quantVars().cend(); ++it) {
			if (newrule->body()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				headvars.push_back(*it);
			}
		}

		headgen = createVarMapAndGenerator(newrule->head(), headvars);
		bodygen = createVarMapAndGenerator(newrule->body(), bodyvars);
		checkGeneratorInfinite(headgen);
	}
	checkGeneratorInfinite(bodygen);

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
	auto bodygrounder = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if (recursive(newrule->body())) {
		_context._tseitin = TsType::RULE;
	}
	if (groundlazily) {
		_rulegrounder = new LazyRuleGrounder(rule, newrule->head()->args(), headgrounder, bodygrounder, bodygen, _context);
	} else {
		_rulegrounder = new FullRuleGrounder(rule, headgrounder, bodygrounder, headgen, bodygen, _context);
	}
	RestoreContext();
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}

	//newrule->recursiveDelete(); INCORRECT, as it deletes its quantvars, which might have been used elsewhere already!
}
