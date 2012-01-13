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
#include "common.hpp"

#include <typeinfo>
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <utility> // for relational operators (namespace rel_ops)
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "ecnf.hpp"
#include "options.hpp"
#include "generators/GeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "common.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/PrintGroundPolicy.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "inferences/grounding/grounders/TermGrounders.hpp"
#include "inferences/grounding/grounders/SetGrounders.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"
#include "inferences/grounding/grounders/LazyQuantGrounder.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

#include "generators/BasicGenerators.hpp"
#include "generators/TableGenerator.hpp"

#include "IdpException.hpp"

#include "utils/TheoryUtils.hpp"

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFactory.hpp"

using namespace std;
using namespace rel_ops;

GenType operator not(GenType orig) {
	switch (orig) {
	case GenType::CANMAKEFALSE:
		return GenType::CANMAKETRUE;
	case GenType::CANMAKETRUE:
		return GenType::CANMAKEFALSE;
	}
}

double MCPA = 1; // TODO: constant currently used when pruning bdds. Should be made context dependent

GrounderFactory::GrounderFactory(AbstractStructure* structure, GenerateBDDAccordingToBounds* symstructure) :
		_structure(structure), _symstructure(symstructure) {

	Assert(_symstructure!=NULL);

	// Create a symbolic structure if no such structure is given
	if (getOption(IntType::GROUNDVERBOSITY) > 2) {
		clog << "Using the following symbolic structure to ground: " << endl;
		_symstructure->put(clog);
	}

}

set<const PFSymbol*> GrounderFactory::findCPSymbols(const AbstractTheory* theory) {
	Vocabulary* vocabulary = theory->vocabulary();
	for (auto funcit = vocabulary->firstFunc(); funcit != vocabulary->lastFunc(); ++funcit) {
		Function* function = funcit->second;
		bool passtocp = false;
		// Check whether the (user-defined) function's outsort is over integers
		if (function->overloaded()) {
			set<Function*> nonbuiltins = function->nonbuiltins();
			for (auto nbfit = nonbuiltins.cbegin(); nbfit != nonbuiltins.cend(); ++nbfit) {
				passtocp = FuncUtils::isIntFunc(function,vocabulary); 
			}
		} else if (not function->builtin()) {
			passtocp = FuncUtils::isIntFunc(function,vocabulary); 
		}
		if (passtocp) {
			_cpsymbols.insert(function);
		}
	}
	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		clog << "User-defined symbols that can be handled by the constraint solver: ";
		for (auto it = _cpsymbols.cbegin(); it != _cpsymbols.cend(); ++it) {
			clog << toString(*it) << " ";
		}
		clog << "\n";
	}
	return _cpsymbols;
}

bool GrounderFactory::isCPSymbol(const PFSymbol* symbol) const {
	return VocabularyUtils::isComparisonPredicate(symbol) || (_cpsymbols.find(symbol) != _cpsymbols.cend());
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
	_context._tseitin = getOption(MODELCOUNTEQUIVALENCE) ? TsType::EQ : TsType::IMPL;
	_context._defined.clear();
	_context._conjunctivePathFromRoot = true; // NOTE: default true: needs to be set to false in each visit in grounderfactory in which it is no longer the case
	_context._conjPathUntilNode = true;

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
}

/**
 * void GrounderFactory::RestoreContext()
 * DESCRIPTION
 *		Restores the context to the top of the stack and pops the stack
 */
void GrounderFactory::RestoreContext() {
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
 *		Visits a term and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		t	- the visited term
 */
void GrounderFactory::descend(Term* t) {
	SaveContext();
	t->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(SetExpr* s)
 * DESCRIPTION
 *		Visits a set and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		s	- the visited set
 */
void GrounderFactory::descend(SetExpr* s) {
	SaveContext();
	s->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(Formula* f)
 * DESCRIPTION
 *		Visits a formula and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		f	- the visited formula
 */
void GrounderFactory::descend(Formula* f) {
	SaveContext();
	f->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(Rule* r)
 * DESCRIPTION
 *		Visits a rule and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		r	- the visited rule
 */
void GrounderFactory::descend(Rule* r) {
	SaveContext();
	r->accept(this);
	RestoreContext();
}

/**
 * TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is not passed to a solver, but stored internally as a EcnfTheory.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 */
Grounder* GrounderFactory::create(const AbstractTheory* theory) {
	// Allocate an ecnf theory to be returned by the grounder
	GroundTheory<GroundPolicy>* groundtheory = new GroundTheory<GroundPolicy>(theory->vocabulary(), _structure->clone());
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if (getOption(BoolType::CPSUPPORT)) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
}

// TODO comment
Grounder* GrounderFactory::create(const AbstractTheory* theory, InteractivePrintMonitor* monitor) {
	GroundTheory<PrintGroundPolicy>* groundtheory = new GroundTheory<PrintGroundPolicy>(_structure->clone());
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator(), groundtheory->termtranslator());
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if (getOption(BoolType::CPSUPPORT)) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
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
Grounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver) {
	// Allocate a solver theory
	GroundTheory<SolverPolicy>* groundtheory = new GroundTheory<SolverPolicy>(theory->vocabulary(), _structure->clone());
	groundtheory->initialize(solver, getOption(IntType::GROUNDVERBOSITY), groundtheory->termtranslator());
	_grounding = groundtheory;

	// Find function that can be passed to CP solver.
	if (getOption(BoolType::CPSUPPORT)) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
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
	tmptheory = FormulaUtils::splitComparisonChains(tmptheory,_structure->vocabulary());
	if (not getOption(BoolType::CPSUPPORT)) {
		tmptheory = FormulaUtils::splitComparisonChains(tmptheory,_structure->vocabulary());
		tmptheory = FormulaUtils::graphFuncsAndAggs(tmptheory,_structure);
	}
	Assert(sametypeid<Theory>(*tmptheory));
	auto newtheory = dynamic_cast<Theory*>(tmptheory);

	// Collect all components (sentences, definitions, and fixpoint definitions) of the theory
	set<TheoryComponent*> tcomps = newtheory->components();
	vector<TheoryComponent*> components(tcomps.cbegin(), tcomps.cend());
	//TODO Order components the components to optimize the grounding process

	// Create grounders for all components
	vector<Grounder*> children(components.size());
	for (size_t n = 0; n < components.size(); ++n) {
		InitContext();

		if (getOption(IntType::GROUNDVERBOSITY) > 0) {
			clog << "Creating a grounder for ";
			components[n]->put(clog);
			clog << "\n";
		}
		components[n]->accept(this);
		children[n] = _topgrounder;
	}

	_topgrounder = new BoolGrounder(_grounding, children, SIGN::POS, true, _context);

	// Clean up: delete the theory clone.
	newtheory->recursiveDelete();
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
		clog << "Grounderfactory visiting: " << toString(pf);
		pushtab();
		clog << "\n" << tabs();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Move all functions and aggregates that are three-valued according
	// to _structure outside the atom. To avoid changing the original atom,
	// we first clone it.
	// FIXME aggregaten moeten correct worden herschreven als ze niet tweewaardig zijn -> issue #23?
	Formula* transpf = FormulaUtils::unnestThreeValuedTerms(pf->clone(), _structure, _context._funccontext, getOption(BoolType::CPSUPPORT), _cpsymbols);
	//transpf = FormulaUtils::splitComparisonChains(transpf);
	if (not getOption(BoolType::CPSUPPORT)) { // TODO Check not present in quantgrounder
		transpf = FormulaUtils::graphFuncsAndAggs(transpf,_structure,_context._funccontext);
	} //TODO issue #23

	if (not sametypeid<PredForm>(*transpf)) { // The rewriting changed the atom
		Assert(_context._component != CompContext::HEAD);
		if (getOption(IntType::GROUNDVERBOSITY) > 1) {
			clog << "Rewritten " << toString(pf) << " to " << toString(transpf) << "\n" << tabs();
		}
		transpf->accept(this);
		transpf->recursiveDelete();
		if (getOption(IntType::GROUNDVERBOSITY) > 3)
			poptab();
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
	if (getOption(BoolType::CPSUPPORT) && VocabularyUtils::isComparisonPredicate(newpf->symbol())) {
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

		_formgrounder = new ComparisonGrounder(_grounding, _grounding->termtranslator(), subtermgrounders[0], comp, subtermgrounders[1], _context);
		_formgrounder->setOrig(newpf, varmapping());
		if (_context._component == CompContext::SENTENCE) { // TODO Refactor outside?
			_topgrounder = _formgrounder;
		}
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

	PredTable *posstable = NULL, *certtable = NULL;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		auto fovars = VarUtils::makeNewVariables(checksorts);
		auto foterms = TermUtils::makeNewVarTerms(fovars);
		auto checkpf = new PredForm(newpf->sign(), newpf->symbol(), foterms, FormulaParseInfo());
		const FOBDD* possbdd;
		const FOBDD* certbdd;
		if (_context.gentype == GenType::CANMAKETRUE) {
			possbdd = _symstructure->evaluate(checkpf, TruthType::POSS_TRUE);
			certbdd = _symstructure->evaluate(checkpf, TruthType::CERTAIN_TRUE);
		} else {
			possbdd = _symstructure->evaluate(checkpf, TruthType::POSS_FALSE);
			certbdd = _symstructure->evaluate(checkpf, TruthType::CERTAIN_FALSE);
		}

		posstable = new PredTable(new BDDInternalPredTable(possbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
		certtable = new PredTable(new BDDInternalPredTable(certbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
	} else {
		posstable = new PredTable(new FullInternalPredTable(), Universe(tables));
		certtable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable()), Universe(tables));
	}

	auto possch = GeneratorFactory::create(posstable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables), pf);
	auto certainch = GeneratorFactory::create(certtable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables), pf);
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << "Certainly table: \n" << tabs() << toString(certtable) << "\n" << tabs();
		clog << "Possible table: \n" << tabs() << toString(posstable) << "\n" << tabs();
		clog << "Possible checker: \n" << tabs() << toString(possch) << "\n" << tabs();
		clog << "Certain checker: \n" << tabs() << toString(certainch) << "\n" << tabs();
	}

	_formgrounder = new AtomGrounder(_grounding, newpf->sign(), newpf->symbol(), subtermgrounders, checkargs, possch, certainch,
			_structure->inter(newpf->symbol()), argsorttables, _context);

	_formgrounder->setOrig(newpf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
	newpf->recursiveDelete();
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		poptab();
	}
}

/**
 * Recursively deletes an object AND sets its reference to NULL (causing sigsev is calling it later, instead of not failing at all)
 * TODO should use this throughout
 */
template<class T>
void deleteDeep(T& object){
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
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << "Grounderfactory visiting: " << toString(bf);
		pushtab();
		clog << "\n" << tabs();
	}

	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && bf->isConjWithSign();

	// Handle a top-level conjunction without creating tseitin atoms
	if (_context._conjPathUntilNode) {
		// If bf is a negated disjunction, push the negation one level deeper.
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
		_topgrounder = new BoolGrounder(_grounding, sub, newbf->sign(), true, _context);
		deleteDeep(newbf);

		if (getOption(IntType::GROUNDVERBOSITY) > 3){
			poptab();
		}
		return;

	}
	// Formula bf is not a top-level conjunction
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
	_formgrounder = new BoolGrounder(_grounding, sub, bf->sign(), bf->conj(), _context);
	RestoreContext();
	_formgrounder->setOrig(bf, varmapping());
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}

	if (getOption(IntType::GROUNDVERBOSITY) > 3)
		poptab();
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
		clog << "Grounderfactory visiting: " << toString(qf);
		pushtab();
		clog << "\n" << tabs();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && qf->isUnivWithSign();
	// TODO guarantee that e.g. no more double negations exist? => FLAGS bijhouden van wat er met de theorie gebeurd

	// Create instance generator
	Formula* newsubformula = qf->subformula()->clone();
	newsubformula = FormulaUtils::unnestThreeValuedTerms(newsubformula, _structure, _context._funccontext);
	//newsubformula = FormulaUtils::splitComparisonChains(newsubformula);
	newsubformula = FormulaUtils::graphFuncsAndAggs(newsubformula,_structure,_context._funccontext);

	// NOTE: if the checker return valid, then the value of the formula can be decided from the value of the checked instantiation
	//	for universal: checker valid => formula false, for existential: checker valid => formula true

	// !x phi(x) => generate all x possibly false
	// !x phi(x) => check for x certainly false
	// FIXME SUBFORMULA got cloned, not the formula itself! REVIEW CODE!

	GenAndChecker gc = createVarsAndGenerators(newsubformula, qf, qf->isUnivWithSign() ? TruthType::POSS_FALSE : TruthType::POSS_TRUE,
			qf->isUnivWithSign() ? TruthType::CERTAIN_FALSE : TruthType::CERTAIN_TRUE);

	// Handle a top-level conjunction without creating tseitin atoms
	if (_context._conjPathUntilNode) {
		// If qf is a negated exist, push the negation one level deeper.
		// Take a clone to avoid changing qf;
		QuantForm* newqf = qf->clone();
		if (not newqf->isUnivWithSign()) {
			newqf->quant(QUANT::UNIV);
			newqf->negate();
			newqf->subformula()->negate();
		}

		// Visit the subformulas
		SaveContext();
		_context.gentype = qf->isUnivWithSign() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
		descend(newsubformula);
		RestoreContext();

		newqf->recursiveDelete();

		//FIXME: lazy stuff in this case?
		_topgrounder = new QuantGrounder(_grounding, dynamic_cast<FormulaGrounder*>(_topgrounder), SIGN::POS, QUANT::UNIV, gc._generator, gc._checker,
				_context);
	} else {

		// Create grounder for subformula
		SaveContext();

		DeeperContext(qf->sign());
		_context.gentype = qf->isUnivWithSign() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
		descend(qf->subformula());
		RestoreContext();

		// Create the grounder
		SaveContext();
		if (recursive(qf)) {
			_context._tseitin = TsType::RULE;
		}

		bool canlazyground = false;
		if (not qf->isUniv() && _context._monotone == Context::POSITIVE && _context._tseitin == TsType::IMPL) {
			canlazyground = true;
		}

		// FIXME add better under-approximation of what to lazily ground
		if (getOption(BoolType::GROUNDLAZILY) && canlazyground && typeid(*_grounding) == typeid(SolverTheory)) {
			_formgrounder = new LazyQuantGrounder(qf->freeVars(), dynamic_cast<SolverTheory*>(_grounding), _formgrounder, qf->sign(), qf->quant(),
					gc._generator, gc._checker, _context);
		} else {
			_formgrounder = new QuantGrounder(_grounding, _formgrounder, qf->sign(), qf->quant(), gc._generator, gc._checker, _context);
		}
		RestoreContext();

		_formgrounder->setOrig(qf, varmapping());

		if (_context._component == CompContext::SENTENCE) {
			_topgrounder = _formgrounder;
		}

	}
	newsubformula->recursiveDelete();
	if (getOption(IntType::GROUNDVERBOSITY) > 3)
		poptab();

}

const FOBDD* GrounderFactory::improve_generator(const FOBDD* bdd, const vector<Variable*>& fovars, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable(*it));
	}
	optimizemanager.optimizequery(copybdd, copyvars, indices, _structure);

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_true(copybdd, copyvars, indices, _structure, mcpa);

	// 3. Replace result
	return manager->getBDD(pruned, &optimizemanager);
}

const FOBDD* GrounderFactory::improve_checker(const FOBDD* bdd, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizequery(copybdd, copyvars, indices, _structure);

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_false(copybdd, copyvars, indices, _structure, mcpa);

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
	FormulaGrounder* leftg = _formgrounder;
	descend(ef->right());
	FormulaGrounder* rightg = _formgrounder;
	RestoreContext();

	// Create the grounder
	SaveContext();
	if (recursive(ef)) {
		_context._tseitin = TsType::RULE;
	}
	_formgrounder = new EquivGrounder(_grounding, leftg, rightg, ef->sign(), _context);
	RestoreContext();
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
}

void GrounderFactory::visit(const AggForm* af) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	Formula* transaf = FormulaUtils::unnestThreeValuedTerms(af->clone(), _structure, _context._funccontext, getOption(BoolType::CPSUPPORT), _cpsymbols);
	transaf = FormulaUtils::graphFuncsAndAggs(transaf, _structure, _context._funccontext);
	if (recursive(transaf)) {
		transaf = FormulaUtils::splitIntoMonotoneAgg(transaf);
	}

	if (not sametypeid<AggForm>(*transaf)) { // The rewriting changed the atom
		if (getOption(IntType::GROUNDVERBOSITY) > 1) {
			clog << "Rewritten " << toString(af) << " to " << toString(transaf) << "\n";
		}
		transaf->accept(this);
	} else { // The rewriting did not change the atom
		AggForm* newaf = dynamic_cast<AggForm*>(transaf);
		// Create grounder for the bound
		descend(newaf->left());
		TermGrounder* boundgr = _termgrounder;

		// Create grounder for the set
		SaveContext();
		if (recursive(newaf)) {
			Assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf));
		}
		DeeperContext((not FormulaUtils::isAntimonotone(newaf)) ? SIGN::POS : SIGN::NEG);
		descend(newaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();

		// Create aggregate grounder
		SaveContext();
		if (recursive(newaf)) {
			_context._tseitin = TsType::RULE;
		}
		if (isNeg(newaf->sign())) {
			if (_context._tseitin == TsType::IMPL) {
				_context._tseitin = TsType::RIMPL;
			} else if (_context._tseitin == TsType::RIMPL) {
				_context._tseitin = TsType::IMPL;
			}
		}
		_formgrounder = new AggGrounder(_grounding, _context, newaf->right()->function(), setgr, boundgr, newaf->comp(), newaf->sign());
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
	_termgrounder->setOrig(t, varmapping(), getOption(IntType::GROUNDVERBOSITY));
}

void GrounderFactory::visit(const DomainTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t, varmapping(), getOption(IntType::GROUNDVERBOSITY));
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
	Function* function = t->function();
	FuncTable* ftable = _structure->inter(function)->funcTable();
	SortTable* domain = _structure->inter(function->outsort());
	if (getOption(BoolType::CPSUPPORT) && FuncUtils::isIntSum(function, _structure->vocabulary())) {
		if (function->name() == "-/2") {
			_termgrounder = new SumTermGrounder(_grounding, _grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1],
					ST_MINUS);
		} else {
			_termgrounder = new SumTermGrounder(_grounding, _grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1]);
		}
	} else {
		_termgrounder = new FuncTermGrounder(_grounding->termtranslator(), function, ftable, domain, subtermgrounders);
	}
	_termgrounder->setOrig(t, varmapping(), getOption(IntType::GROUNDVERBOSITY));
}

void GrounderFactory::visit(const AggTerm* t) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	_termgrounder = new AggTermGrounder(_grounding->translator(), t->function(), _setgrounder);
	_termgrounder->setOrig(t, varmapping(), getOption(IntType::GROUNDVERBOSITY));
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

// TODO verify
template<typename OrigConstruct>
GrounderFactory::GenAndChecker GrounderFactory::createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype) {
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
		generatorbdd = improve_generator(generatorbdd, quantfovars, MCPA);
		checkerbdd = improve_checker(checkerbdd, MCPA);
		gentable = new PredTable(new BDDInternalPredTable(generatorbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
		checktable = new PredTable(new BDDInternalPredTable(checkerbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
	} else {
		gentable = new PredTable(new FullInternalPredTable(), Universe(tables));
		checktable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable), Universe(tables));
	}

	auto gen = GeneratorFactory::create(gentable, pattern, vars, Universe(tables), subformula);
	auto check = GeneratorFactory::create(checktable, vector<Pattern>(vars.size(), Pattern::INPUT), vars, Universe(tables), subformula);
	return GenAndChecker(gen, check);
}

void GrounderFactory::visit(const QuantSetExpr* origqs) {
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Move three-valued terms in the set expression
	auto transqs = SetUtils::unnestThreeValuedTerms(origqs->clone(), _structure, _context._funccontext, getOption(BoolType::CPSUPPORT), _cpsymbols);
	if (not sametypeid<QuantSetExpr>(*transqs)) {
		if (getOption(IntType::GROUNDVERBOSITY) > 1) {
			clog << "Rewritten " << toString(origqs) << " to " << toString(transqs) << "\n";
		}
		transqs->accept(this);
		return;
	}

	auto newqs = dynamic_cast<QuantSetExpr*>(transqs);
	Formula* clonedformula = newqs->subformulas()[0]->clone();
	Formula* newsubformula = FormulaUtils::unnestThreeValuedTerms(clonedformula, _structure, Context::POSITIVE);
	//newsubformula = FormulaUtils::splitComparisonChains(newsubformula);
	newsubformula = FormulaUtils::graphFuncsAndAggs(newsubformula,_structure,_context._funccontext); //TODO issue #23

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
	_setgrounder = new QuantSetGrounder(_grounding->translator(), subgr, gc._generator, gc._checker, wgr);
}

/**
 * void GrounderFactory::visit(const Definition* def)
 * DESCRIPTION
 * 		Creates a grounder for a definition.
 */
void GrounderFactory::visit(const Definition* def) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << "Grounderfactory visiting: " << toString(def);
		pushtab();
		clog << "\n" << tabs();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// Store defined predicates
	for (auto it = def->defsymbols().cbegin(); it != def->defsymbols().cend(); ++it) {
		_context._defined.insert(*it);
	}

	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for (auto it = def->rules().cbegin(); it != def->rules().cend(); ++it) {
		descend(*it);
		subgrounders.push_back(_rulegrounder);
	}

	_topgrounder = new DefinitionGrounder(_grounding, subgrounders, _context);

	_context._defined.clear();
	if (getOption(IntType::GROUNDVERBOSITY) > 3)
		poptab();
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
	auto d = new DomElemContainer();
	_varmapping[var] = d;
	return d;
}

void GrounderFactory::visit(const Rule* rule) {
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << "Grounderfactory visiting: " << toString(rule);
		pushtab();
		clog << "\n" << tabs();
	}
	_context._conjunctivePathFromRoot = _context._conjPathUntilNode;
	_context._conjPathUntilNode = false;

	// TODO for lazygroundrules, we need a generator for all variables NOT occurring in the head!
	Rule* newrule = DefinitionUtils::unnestThreeValuedTerms(rule->clone(), _structure, _context._funccontext, getOption(BoolType::CPSUPPORT), _cpsymbols);
	InstGenerator *headgen = NULL, *bodygen = NULL;

	if (getOption(BoolType::GROUNDLAZILY)) {
		Assert(sametypeid<SolverTheory>(*_grounding));
		// TODO resolve this in a clean way
		// for lazy ground rules, need a generator which generates bodies given a head, so only vars not occurring in the head!
		varlist bodyvars;
		for (auto it = newrule->quantVars().cbegin(); it != newrule->quantVars().cend(); ++it) {
			if (not newrule->head()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				createVarMapping(*it);
			}
		}

		bodygen = createVarMapAndGenerator(rule->head(), bodyvars);
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

		headgen = createVarMapAndGenerator(rule->head(), headvars);
		bodygen = createVarMapAndGenerator(rule->body(), bodyvars);
	}

	// Create head grounder
	SaveContext();
	_context._component = CompContext::HEAD;
	descend(newrule->head());
	HeadGrounder* headgr = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._funccontext = Context::NEGATIVE; // minimize truth value of rule bodies
	_context._monotone = Context::POSITIVE;
	_context.gentype = GenType::CANMAKETRUE; // body instance generator corresponds to an existential quantifier
	_context._component = CompContext::FORMULA;
	_context._tseitin = TsType::EQ;
	descend(newrule->body());
	FormulaGrounder* bodygr = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if (recursive(newrule->body())) {
		_context._tseitin = TsType::RULE; //TODO: is this right??? Shouldn't it be higher (before createing the bodygrounder)?
	}
	if (getOption(BoolType::GROUNDLAZILY)) {
		_rulegrounder = new LazyRuleGrounder(headgr, bodygr, bodygen, _context);
	} else {
		_rulegrounder = new RuleGrounder(headgr, bodygr, headgen, bodygen, _context);
	}
	RestoreContext();
	if (getOption(IntType::GROUNDVERBOSITY) > 3)
		poptab();
}
