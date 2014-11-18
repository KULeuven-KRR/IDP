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
#include "lazygrounders/LazyDisjunctiveGrounders.hpp"
#include "LazyGroundingManager.hpp"
//#include "grounders/LazyFormulaGrounders.hpp"
//#include "grounders/LazyRuleGrounder.hpp"
#include "inferences/grounding/grounders/OptimizationTermGrounders.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "inferences/SolverConnection.hpp"
#include "structure/information/IsTwoValued.hpp"

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

#include "theory/transformations/AddIfCompletion.hpp"

#include "utils/CPUtils.hpp"

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

DefId getIDForUndefined() {
	return DefId(-1);
}

template<typename Grounding>
GrounderFactory::GrounderFactory(const Vocabulary* outputvocabulary, StructureInfo structures, Grounding* grounding, bool nbModelsEquivalent)
		: 	_vocabulary(structures.concrstructure->vocabulary()),
			_structure(structures),
			_grounding(grounding),
			_nbmodelsequivalent(nbModelsEquivalent),
			_formgrounder(NULL),
			_termgrounder(NULL),
			_setgrounder(NULL),
			_quantsetgrounder(NULL),
			_headgrounder(NULL),
			_rulegrounder(NULL),
			_topgrounder(NULL) {
	Assert(_structure.symstructure != NULL);

	InitContext();
	_groundingmanager = new LazyGroundingManager(grounding, getContext(), outputvocabulary, structures, _nbmodelsequivalent);

	// Create a symbolic structure if no such structure is given
	if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 2) {
		clog << tabs() << "Using the following symbolic structure to ground:" << "\n";
		clog << tabs() << toString(_structure.symstructure) << "\n";
	}
}

GrounderFactory::GrounderFactory(LazyGroundingManager* manager)
		: 	_vocabulary(manager->getStructureInfo().concrstructure->vocabulary()),
			_structure(manager->getStructureInfo()),
			_grounding(manager->getGrounding()),
			_nbmodelsequivalent(manager->getNbModelEquivalent()),
			_formgrounder(NULL),
			_termgrounder(NULL),
			_setgrounder(NULL),
			_quantsetgrounder(NULL),
			_headgrounder(NULL),
			_rulegrounder(NULL),
			_topgrounder(NULL),
			_groundingmanager(manager) {
	Assert(_structure.symstructure != NULL);

	InitContext();

	// Create a symbolic structure if no such structure is given
	if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 2) {
		clog << tabs() << "Using the following symbolic structure to ground:" << "\n";
		clog << tabs() << print(_structure.symstructure) << "\n";
	}
}

GrounderFactory::~GrounderFactory() {
}

void GrounderFactory::setTopGrounder(Grounder* grounder) {
	_topgrounder = grounder;
	Assert(_topgrounder==NULL || _topgrounder->getContext()._conjunctivePathFromRoot);
}
Grounder* GrounderFactory::getTopGrounder() const {
	if(_topgrounder==NULL){
		throw IdpException("Invalid code path");
	}
	return _topgrounder;
}
FormulaGrounder* GrounderFactory::getFormGrounder() const {
	Assert(_formgrounder!=NULL);
	return _formgrounder;
}
EnumSetGrounder* GrounderFactory::getSetGrounder() const {
	Assert(_setgrounder!=NULL);
	return _setgrounder;
}
RuleGrounder* GrounderFactory::getRuleGrounder() const {
	Assert(_rulegrounder!=NULL);
	return _rulegrounder;
}
TermGrounder* GrounderFactory::getTermGrounder() const {
	Assert(_termgrounder!=NULL);
	return _termgrounder;
}

/**
 * 	Finds out whether a formula contains recursively defined symbols.
 */
bool GrounderFactory::recursive(const Formula* f) {
	for (auto it = _context._defined.cbegin(); it != _context._defined.cend(); ++it) {
		if (FormulaUtils::containsSymbol(*it, f)) {
			return true;
		}
	}
	return false;
}

bool GrounderFactory::recursive(const Term* f) {
	for (auto it = _context._defined.cbegin(); it != _context._defined.cend(); ++it) {
		if (TermUtils::containsSymbol(*it, f)) {
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
	_context._component = CompContext::FORMULA;
	_context._tseitin = _nbmodelsequivalent ? TsType::EQ : TsType::IMPL;
	_context.currentDefID = getIDForUndefined();
	_context._defined.clear();

	_context._conjunctivePathFromRoot = true;
	_context._conjPathUntilNode = true;

	_context._cpablerelation = TruthValue::Unknown;

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
 *	One level deeper
 */
void GrounderFactory::DeeperContext(SIGN sign) {
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
LazyGroundingManager* GrounderFactory::createGrounder(const GroundInfo& data, GroundTheory groundtheory) {
	Assert(data.minimizeterm==NULL || VocabularyUtils::isSubVocabulary(data.minimizeterm->vocabulary(), data.structure.concrstructure->vocabulary()));
	Assert(VocabularyUtils::isSubVocabulary(data.theory->vocabulary(), data.structure.concrstructure->vocabulary()));
	GrounderFactory g(data.outputvocabulary, data.structure, groundtheory, data.nbModelsEquivalent);
	return g.ground(data.theory, data.minimizeterm);
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
LazyGroundingManager* GrounderFactory::create(const GroundInfo& data) {
	auto groundtheory = new GroundTheory<GroundPolicy>(data.theory->vocabulary(), data.structure, data.nbModelsEquivalent);
	groundtheory->initializeTheory();
	return createGrounder(data, groundtheory);
}
LazyGroundingManager* GrounderFactory::create(const GroundInfo& data, InteractivePrintMonitor* monitor) {
	auto groundtheory = new GroundTheory<PrintGroundPolicy>(data.structure, data.nbModelsEquivalent);
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator());
	groundtheory->initializeTheory();
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
LazyGroundingManager* GrounderFactory::create(const GroundInfo& data, PCSolver* solver) {
	auto groundtheory = new SolverTheory(data.theory->vocabulary(), data.structure, data.nbModelsEquivalent);
	groundtheory->initialize(solver, getOption(IntType::VERBOSE_GROUNDING), groundtheory->translator());
	groundtheory->initializeTheory();
	auto grounder = createGrounder(data, groundtheory);
	SolverConnection::setTranslator(solver, grounder->translator());
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

LazyGroundingManager* GrounderFactory::ground(AbstractTheory* theory, Term* minimizeterm) {
	std::vector<Grounder*> grounders;

	theory = FormulaUtils::improveTheoryForInference(theory, getConcreteStructure(), getOption(SKOLEMIZE), _nbmodelsequivalent);

	// NOTE: important that we only add funcconstraints for the theory at hand! e.g. for calculate definitions, we should not find values for the functions not occurring in it!
	FormulaUtils::addFuncConstraints(theory, _vocabulary, funcconstraints, not getOption(BoolType::CPSUPPORT));
	if (minimizeterm != NULL) {
		FormulaUtils::addFuncConstraints(minimizeterm, _vocabulary, funcconstraints, not getOption(BoolType::CPSUPPORT));
	}

	for (auto func2constr : funcconstraints) {
		if(getConcreteStructure()->inter(func2constr.first)->approxTwoValued()){
			continue; // Do not add function constraints for two-valued functions
		}
		InitContext();
		descend(func2constr.second);
		grounders.push_back(getTopGrounder());
	}

	InitContext();
	descend(theory);
	grounders.push_back(getTopGrounder());

	if (minimizeterm != NULL) {
		OptimizationGrounder* optimgrounder;
		if (getOption(BoolType::CPSUPPORT) and CPSupport::allSymbolsEligible(minimizeterm, getConcreteStructure())) {
			InitContext();
			descend(minimizeterm);
			optimgrounder = new VariableOptimizationGrounder(getGrounding(), getTermGrounder(), getContext(), minimizeterm);
			grounders.push_back(optimgrounder);
		} else {
			switch (minimizeterm->type()) {
			case TermType::AGG: {
				auto term = dynamic_cast<const AggTerm*>(minimizeterm);
				Assert(term != NULL);
				if (term->function() == AggFunction::PROD) {
					for (auto i = term->set()->getSubSets().cbegin(); i != term->set()->getSubSets().cend(); ++i) {
						auto sort = (*i)->getTerm()->sort();
						if (not SortUtils::isSubsort(sort, get(STDSORT::NATSORT)) || getConcreteStructure()->inter(sort)->contains(createDomElem(0))) {
							throw notyetimplemented("Minimization over a product aggregate with negative or zero weights");
						}
					}
				}
				InitContext();
				descend(term->set());
				auto optimgrounder = new AggregateOptimizationGrounder(getGrounding(), term->function(), getSetGrounder(), getContext(), minimizeterm);
				grounders.push_back(optimgrounder);
				break;
			}
			case TermType::FUNC:
			case TermType::VAR:
			case TermType::DOM:
				// TODO solution: add one new constant + equality to the theory
				throw notyetimplemented("Optimization over non-aggregate terms for which CP support is not available.");
			}
		}
	}

	for (auto grounder : grounders) {
		_groundingmanager->add(grounder);
	}

	if(useUFSAndOnlyIfSem()){
		AddIfCompletion c;
		auto ifcompsentences = c.getOnlyIfFormulas(*theory);
		for(auto s2sent: ifcompsentences){
			InitContext();
			descend(s2sent.formula);
			_groundingmanager->add(getFormGrounder(), s2sent.head, false, s2sent.definitionid);
		}
	}

	return _groundingmanager; // NOTE: passes ownership!
}

// TODO should not need any func constraints
// TODO NOT allowed to combine quantforms
FormulaGrounder* GrounderFactory::createSentenceGrounder(LazyGroundingManager* manager, Formula* sentence) {
	GrounderFactory g(manager);
	g.InitContext();
	g.descend(sentence);
	return g.getFormGrounder();
}

/**
 *	Visits a child and ensures the context is saved before the visit and restored afterwards.
 */
template<typename T>
void GrounderFactory::descend(T child) {
	Assert(child!=NULL);
	SaveContext();

	if (getContext()._conjPathUntilNode) {
		if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 0) {
			clog << tabs() << "Creating a grounder for " << print(child) << "\n";
			if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 3) {
				pushtab();
			}
		}
	} else {
		if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 3) {
			clog << tabs() << "Creating a subgrounder for " << print(child) << "\n";
			pushtab();
		}
	}

	_formgrounder = NULL;
	_termgrounder = NULL;
	_setgrounder = NULL;
	_headgrounder = NULL;
	_rulegrounder = NULL;
	setTopGrounder(NULL);

	if (not _context._conjPathUntilNode) {
		_context._conjunctivePathFromRoot = false;
	}
	_context._conjPathUntilNode = false; // NOTE: overwrite at start of visit if necessary!

	child->accept(this);

	if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 3) {
		poptab();
	}

	RestoreContext();
}

void GrounderFactory::visit(const Theory* theory) {
	_context._conjPathUntilNode = true;

	// experiment with:
	//tmptheory = FormulaUtils::removeFunctionSymbolsFromDefs(tmptheory, _structure);

	const auto components = theory->getComponents();
	// NOTE: primitive reorder present: definitions first => important for good lazy grounding at the moment
	// TODO Order the components to optimize the grounding process

	// newtheory = FormulaUtils::replaceWithNestedTseitins(newtheory); // FIXME bugged!

	std::vector<Grounder*> children;
	const auto components2 = theory->getComponents(); // NOTE: primitive reorder present: definitions first
	for (auto i = components2.cbegin(); i < components2.cend(); ++i) {
		InitContext();
		descend(*i);
		children.push_back(getTopGrounder());
	}

	setTopGrounder(new TheoryGrounder(getGrounding(), getContext(), children));
}

std::vector<SortTable*> getArgTables(Function* function, Structure* structure) {
	vector<SortTable*> tables;
	for (auto i = function->sorts().cbegin(); i < function->sorts().cend() - 1; ++i) {
		tables.push_back(structure->inter(*i));
	}
	return tables;
}

bool isAggTerm(const Term* term) {
	return term->type() == TermType::AGG;
}

// Rewrite card ~ func, card ~ var, sum ~ func, sum ~ var into sum ~ 0
const AggForm* rewriteSumOrCardIntoSum(const AggForm* af, Structure* structure) {
	if (af->getBound()->type() == TermType::FUNC or af->getBound()->type() == TermType::VAR) {
		if (af->getAggTerm()->function() == AggFunction::CARD) {
			af = new AggForm(af->sign(), af->getBound()->clone(), af->comp(),
					new AggTerm(af->getAggTerm()->set()->clone(), AggFunction::SUM, af->getAggTerm()->pi()), af->pi());
			for (auto i = af->getAggTerm()->set()->getSets().cbegin(); i < af->getAggTerm()->set()->getSets().cend(); ++i) {
				Assert((*i)->getTerm()->type()==TermType::DOM);
				Assert(dynamic_cast<DomainTerm*>((*i)->getTerm())->value()->type()==DomainElementType::DET_INT);
				Assert(dynamic_cast<DomainTerm*>((*i)->getTerm())->value()->value()._int==1);
			}
		}
		if (af->getAggTerm()->function() == AggFunction::SUM && af->getBound()->type()!=TermType::VAR) { // FIXME also for anything else that will be known (better shared set detection)
			//auto minus = get(STDFUNC::UNARYMINUS, { get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, structure->vocabulary());
			//auto newft = new FuncTerm(minus, { af->getBound()->clone() }, TermParseInfo());
			auto prod = get(STDFUNC::PRODUCT, { get(STDSORT::INTSORT), get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, structure->vocabulary());
			auto newft = new FuncTerm(prod, { new DomainTerm(get(STDSORT::INTSORT), createDomElem(-1), TermParseInfo()), af->getBound()->clone() }, TermParseInfo());
			auto newset = af->getAggTerm()->set()->clone();
			newset->addSet(new QuantSetExpr( { }, FormulaUtils::trueFormula(), newft, SetParseInfo()));
			af = new AggForm(af->sign(), new DomainTerm(get(STDSORT::NATSORT), createDomElem(0), TermParseInfo()), af->comp(),
					new AggTerm(newset, af->getAggTerm()->function(), af->getAggTerm()->pi()), af->pi());
		}
	}
	return af;
}

void GrounderFactory::visit(const PredForm* pf) {
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot;

	auto temppf = pf->clone();
	auto transpf = FormulaUtils::unnestThreeValuedTerms(temppf, getConcreteStructure(), _context._defined, getOption(BoolType::CPSUPPORT) and not recursive(pf));

	if (not isa<PredForm>(*transpf)) { // NOTE: the rewriting changed the atom
		Assert(_context._component != CompContext::HEAD);
		transpf->accept(this);
		deleteDeep(transpf);
		return;
	}

	auto newpf = dynamic_cast<PredForm*>(transpf);

	auto aggform = tryToTurnIntoAggForm(newpf);
	if(aggform!=NULL){
		aggform->accept(this);
		delete(aggform);
		return;
	}

	bool cpable = getOption(BoolType::CPSUPPORT) and not recursive(pf) and _context._component != CompContext::HEAD;
	// Ungraph if cp can be used
	if(cpable && newpf->isGraphedFunction()){ // TODO subtle issues if pf is the head of a rule and defines a function, related to aggregates in the head
		auto func = dynamic_cast<Function*>(newpf->symbol());
		if(not CPSupport::eligibleForCP(func, _vocabulary)){
			cpable = false;
		}else{
			auto terms = newpf->subterms();
			auto image = terms.back();
			terms.pop_back();
			newpf = new PredForm(newpf->sign(), get(STDPRED::EQ, image->sort()), {new FuncTerm(func, terms, TermParseInfo()), image}, newpf->pi());
		}
	}
	// But do not use a comparison generator if CP cannot be used for BOTH terms
	if(cpable && VocabularyUtils::isComparisonPredicate(newpf->symbol()) && is(newpf->symbol(), STDPRED::EQ) &&
			((newpf->subterms()[0]->type()==TermType::FUNC && not CPSupport::eligibleForCP(dynamic_cast<FuncTerm*>(newpf->subterms()[0])->function(), _vocabulary))
					||
			 (newpf->subterms()[1]->type()==TermType::FUNC && not CPSupport::eligibleForCP(dynamic_cast<FuncTerm*>(newpf->subterms()[1])->function(), _vocabulary)))){
		cpable = false;
	}

	if(cpable and VocabularyUtils::isIntComparisonPredicate(newpf->symbol(), getConcreteStructure()->vocabulary())){
		// It is is an integer comparison, we check whether one side is already a sum, in which case it we move the other side to it
		// so it a sum aggregate (with fixed bound) will be created during grounding
		auto left = newpf->subterms()[0];
		auto right = newpf->subterms()[1];
		auto leftdom = dynamic_cast<DomainTerm*>(left);
		auto rightdom = dynamic_cast<DomainTerm*>(right);
		if((leftdom==NULL || leftdom->value()!=createDomElem(0)) && (rightdom==NULL || rightdom->value()!=createDomElem(0))){
			auto leftfunc = dynamic_cast<FuncTerm*>(left);
			auto rightfunc = dynamic_cast<FuncTerm*>(right);
			auto ints = get(STDSORT::INTSORT);
			if (leftfunc != NULL && (is(leftfunc->function(), STDFUNC::ADDITION) || is(leftfunc->function(), STDFUNC::SUBSTRACTION))) {
				newpf->subterm(1, new DomainTerm(get(STDSORT::NATSORT), createDomElem(0), TermParseInfo()));
				newpf->subterm(0, new FuncTerm(get(STDFUNC::SUBSTRACTION)->disambiguate( { ints, ints, ints }, NULL), { left, right }, TermParseInfo()));
			} else if (rightfunc != NULL && (is(rightfunc->function(), STDFUNC::ADDITION) || is(rightfunc->function(), STDFUNC::SUBSTRACTION))) {
				newpf->subterm(0, new DomainTerm(get(STDSORT::NATSORT), createDomElem(0), TermParseInfo()));
				newpf->subterm(1, new FuncTerm(get(STDFUNC::SUBSTRACTION)->disambiguate( { ints, ints, ints }, NULL), { right, left }, TermParseInfo()));
			}
		}

		handleWithComparisonGenerator(newpf);
	}else{
		handleGeneralPredForm(newpf);
	}

	checkAndAddAsTopGrounder();

	deleteDeep(newpf);
}

void GrounderFactory::handleWithComparisonGenerator(const PredForm* pf){
	auto comp = VocabularyUtils::getComparisonType(pf->symbol());
	if (isNeg(pf->sign())) {
		comp = negateComp(comp);
	}

	vector<TermGrounder*> subtermgrounders;
	SaveContext();
	for (auto subterm: pf->subterms()) {
		descend(subterm);
		subtermgrounders.push_back(getTermGrounder());
	}
	RestoreContext();

	_formgrounder = new ComparisonGrounder(getGrounding(), subtermgrounders[0], comp, subtermgrounders[1], _context, pf->symbol(), pf->sign());
}

void GrounderFactory::handleGeneralPredForm(const PredForm* pf){
	auto symbol = pf->symbol();
	auto terms = pf->subterms();
	if (VocabularyUtils::isComparisonPredicate(symbol)) {
		auto left = terms[0]; auto right = terms[1];

		if(isAggTerm(left) || isAggTerm(right)){
			auto aggterm = isAggTerm(left)? dynamic_cast<AggTerm*>(left) :  dynamic_cast<AggTerm*>(right);
			auto other = isAggTerm(left)? right :  left;
			auto agg = new AggForm(pf->sign(), other->clone(), invertComp(VocabularyUtils::getComparisonType(pf->symbol())), aggterm->clone(), pf->pi());
			descend(agg);
			deleteDeep(agg);
			return;
		}

		// Graph if CP is not used or not applicable to this equality:
		// * CP is off
		// * No integers
		// * Recursive context
		// * Left or right term are not cpable
		if(not getOption(CPSUPPORT) || not VocabularyUtils::isIntComparisonPredicate(symbol, getConcreteStructure()->vocabulary()) || recursive(pf) || not CPSupport::eligibleForCP(left, getConcreteStructure()) || not CPSupport::eligibleForCP(right, getConcreteStructure())){
			if(is(symbol, STDPRED::EQ) && left->type()==TermType::FUNC){
				auto functerm = dynamic_cast<FuncTerm*>(left);
				terms = functerm->subterms();
				terms.push_back(right);
				symbol = functerm->function();
			}else if(is(symbol, STDPRED::EQ) && right->type()==TermType::FUNC){
				auto functerm = dynamic_cast<FuncTerm*>(right);
				terms = functerm->subterms();
				terms.push_back(left);
				symbol = functerm->function();
			}
		}
	}

	for(auto term:terms){
		if(recursive(term)){
			stringstream ss;
			ss <<print(term) <<" in " <<print(pf) <<" is recursive, which should not have been produced.\n";
			throw InternalIdpException(ss.str());
		}
	}

	vector<TermGrounder*> subtermgrounders;
	vector<SortTable*> argsorttables;
	SaveContext();
	for (size_t n = 0; n < terms.size(); ++n) {
		descend(terms[n]);
		subtermgrounders.push_back(getTermGrounder());
		argsorttables.push_back(getConcreteStructure()->inter(symbol->sorts()[n]));
	}
	RestoreContext();

    if (_context._component == CompContext::HEAD) {
		_headgrounder = new HeadGrounder(getGrounding(), symbol, subtermgrounders, argsorttables, _context);
    } else {
		_formgrounder = new AtomGrounder(getGrounding(), pf->sign(), symbol, subtermgrounders, argsorttables, _context, pf);
    }

	checkAndAddAsTopGrounder();
}

void GrounderFactory::visit(const AggForm* af) {
	auto clonedaf = rewriteSumOrCardIntoSum(af, getConcreteStructure())->clone();
	Formula* transaf = FormulaUtils::unnestThreeValuedTerms(clonedaf, getConcreteStructure(), _context._defined, getOption(CPSUPPORT));
	if (recursive(transaf)) {
		transaf = FormulaUtils::splitIntoMonotoneAgg(transaf);
	}

	if (not isa<AggForm>(*transaf)) {
		descend(transaf);
		deleteDeep(transaf);
		return;
	}

	auto newaf = dynamic_cast<AggForm*>(transaf);
	Assert(not recursive(newaf) or FormulaUtils::isMonotone(newaf) or FormulaUtils::isAntimonotone(newaf));

	auto comp = newaf->comp();
	auto bound = newaf->getBound();
	auto aggterm = newaf->getAggTerm();
	if (getOption(CPSUPPORT) and not recursive(newaf) and CPSupport::eligibleForCP(aggterm, getConcreteStructure()) and CPSupport::eligibleForCP(bound, getConcreteStructure())) {
		groundAggWithCP(newaf->sign(), bound, comp, aggterm);
	} else {
		groundAggWithoutCP(FormulaUtils::isAntimonotone(newaf), recursive(newaf), newaf->sign(), bound, comp, aggterm);
	}

	deleteDeep(newaf);
}

void GrounderFactory::groundAggWithCP(SIGN sign, Term* bound, CompType comp, AggTerm* agg){
	if (isNeg(sign)) {
		comp = negateComp(comp); // TODO tseitin?
	}
	descend(agg);
	auto boundgrounder = getTermGrounder();
	descend(bound);
	auto termgrounder = getTermGrounder();
	_formgrounder = new ComparisonGrounder(getGrounding(), termgrounder, comp, boundgrounder, _context, get(STDPRED::EQ, agg->sort()), sign);
	checkAndAddAsTopGrounder();
}

void GrounderFactory::groundAggWithoutCP(bool antimono, bool recursive, SIGN sign, Term* bound, CompType comp, AggTerm* agg){
	// Create grounder for the bound
	Assert(bound->type()==TermType::DOM or bound->type()==TermType::VAR or isTwoValued(bound, getConcreteStructure()));
	descend(bound);
	auto boundgrounder = getTermGrounder();

	// Create grounder for the set
	SaveContext();
	DeeperContext((not antimono) ? SIGN::POS : SIGN::NEG);
	descend(agg->set());
	auto setgrounder = getSetGrounder();
	RestoreContext();

	// Create aggregate grounder
	SaveContext();
	if (recursive) {
		_context._tseitin = TsType::RULE;
	}
	_formgrounder = new AggGrounder(getGrounding(), _context, boundgrounder, comp, agg->function(), setgrounder, sign);

	checkAndAddAsTopGrounder();

	RestoreContext();
}

AggForm* GrounderFactory::tryToTurnIntoAggForm(const PredForm* pf){
	if (not VocabularyUtils::isComparisonPredicate(pf->symbol())) {
		return NULL;
	}
	AggTerm* aggterm = NULL;
	Term* bound = NULL;
	bool aggwasfirst = false;
	if (isAggTerm(pf->subterms()[0])) {
		aggterm = dynamic_cast<AggTerm*>(pf->subterms()[0]);
		bound = pf->subterms()[1];
		aggwasfirst = true;
	} else if (isAggTerm(pf->subterms()[1])) {
		aggterm = dynamic_cast<AggTerm*>(pf->subterms()[1]);
		bound = pf->subterms()[0];
	}
	if (aggterm == NULL) {
		return NULL;
	}
	// Rewrite card op func, card op var, sum op func, sum op var into sum op 0
	if (bound->type() == TermType::FUNC or bound->type() == TermType::VAR) {
		bool newagg = false, newbound = false;
		if (aggterm->function() == AggFunction::CARD) {
			aggterm = new AggTerm(aggterm->set()->clone(), AggFunction::SUM, aggterm->pi());
			newagg = true;
		}
		if (aggterm->function() == AggFunction::SUM && bound->type()!=TermType::VAR) { // TODO or anything else known at ground time
			//auto minus = get(STDFUNC::UNARYMINUS, { get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, getConcreteStructure()->vocabulary());
			//auto newft = new FuncTerm(minus, { bound->clone() }, TermParseInfo());
			auto prod = get(STDFUNC::PRODUCT, { get(STDSORT::INTSORT), get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, getConcreteStructure()->vocabulary());
			auto newft = new FuncTerm(prod, { new DomainTerm(get(STDSORT::INTSORT), createDomElem(-1), TermParseInfo()), bound->clone() }, TermParseInfo());
			auto newset = aggterm->set()->clone();
			newset->addSubSet(new QuantSetExpr( { }, FormulaUtils::trueFormula(), newft, SetParseInfo()));
			bound = new DomainTerm(get(STDSORT::NATSORT), createDomElem(0), TermParseInfo());
			aggterm = new AggTerm(newset, aggterm->function(), aggterm->pi());
			newagg = true;
			newbound = true;
		}
		if (not newagg) {
			aggterm = aggterm->clone();
		}
		if (not newbound) {
			bound = bound->clone();
		}
	}
	auto comp = VocabularyUtils::getComparisonType(pf->symbol());
	if(aggwasfirst){
		comp = invertComp(comp);
	}
	return new AggForm(pf->sign(), bound, comp, aggterm, pf->pi());
}

void GrounderFactory::visit(const BoolForm* bf) {
	if(bf->subformulas().size() == 1){
		_context._conjPathUntilNode = _context._conjunctivePathFromRoot;
		auto subf = bf->subformulas()[0];
		if(isNeg(bf->sign())){
			subf->negate();
		}
		descend(subf);
		return;
	}

	_context._conjPathUntilNode = (_context._conjunctivePathFromRoot and bf->isConjWithSign());

	if (_context._conjPathUntilNode) {
		createBoolGrounderConjPath(bf);
	} else {
		createBoolGrounderDisjPath(bf);
	}
}

ClauseGrounder* createB(AbstractGroundTheory* grounding, const vector<FormulaGrounder*>& sub, SIGN sign, bool conj,
		const GroundingContext& context, bool recursive) {
	auto disjunction = (not conj && context._monotone == Context::POSITIVE) || (conj && context._monotone == Context::NEGATIVE);
	auto lazyAllowed = getOption(TSEITINDELAY) && (disjunction || context._monotone == Context::BOTH) && sub.size() > 10;
	if (lazyAllowed && isa<SolverTheory>(*grounding)) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		return new LazyDisjGrounder(solvertheory, sub, sign, conj, context, recursive);
	} else {
		return new BoolGrounder(grounding, sub, sign, conj, context);
	}
}

// Handle a top-level conjunction without creating tseitin atoms
void GrounderFactory::createBoolGrounderConjPath(const BoolForm* bf) {
	// NOTE: to reduce the number of created tseitins, if bf is a negated disjunction, push the negation one level deeper.
	// Take a clone to avoid changing bf
	auto newbf = bf->clone();
	if (not newbf->conj()) {
		newbf->conj(true);
		newbf->negate();
		for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
			(*it)->negate();
		}
	}

	// Visit the subformulas
	vector<FormulaGrounder*> sub;
	for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
		descend(*it);
		sub.push_back(getFormGrounder());
	}

	_formgrounder = createB(getGrounding(), sub, newbf->sign(), true, _context, recursive(bf));
	checkAndAddAsTopGrounder();
	deleteDeep(newbf);
}

// Formula bf is not a top-level conjunction
void GrounderFactory::createBoolGrounderDisjPath(const BoolForm* bf) {
	// Create grounders for subformulas
	SaveContext();
	DeeperContext(bf->sign());
	vector<FormulaGrounder*> sub;
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
	_formgrounder = createB(getGrounding(), sub, bf->sign(), bf->conj(), _context, recursive(bf));
	RestoreContext();
	checkAndAddAsTopGrounder();
}

void GrounderFactory::visit(const QuantForm* qf) {
	Formula* tempqf = qf->cloneKeepVars();
	if(not getOption(SATISFIABILITYDELAY)){
//#warning Correctness currently depends on this (when splitting) => Instead of not pushing, lazy grounding should pull quantifications before searching delays!
		tempqf = FormulaUtils::pushQuantifiersAndNegations(tempqf);
	}
	if(not isa<QuantForm>(*tempqf)){
		tempqf->accept(this);
		deleteDeep(tempqf);
		return;
	}

	auto newqf = dynamic_cast<QuantForm*>(tempqf);
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot && newqf->isUnivWithSign();

	// Create instance generator
	Formula* newsubformula = newqf->subformula()->cloneKeepVars();
	// !x phi(x) => generate all x possibly false
	// !x phi(x) => check for x certainly false
	auto gc = createVarsAndGenerators(newsubformula, newqf, newqf->isUnivWithSign() ? TruthType::POSS_FALSE : TruthType::POSS_TRUE,
			newqf->isUnivWithSign() ? TruthType::CERTAIN_FALSE : TruthType::CERTAIN_TRUE);

	// Handle a top-level conjunction without creating tseitin atoms
	_context.gentype = newqf->isUnivWithSign() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
	if (_context._conjunctivePathFromRoot) {
		createTopQuantGrounder(newqf, newsubformula, gc);
	} else {
		createNonTopQuantGrounder(newqf, newsubformula, gc);
	}
	deleteDeep(newsubformula);
	deleteDeep(newqf);
}

ClauseGrounder* createQ(LazyGroundingManager* manager, AbstractGroundTheory* grounding, FormulaGrounder* subgrounder, QuantForm const * const qf, const GenAndChecker& gc,
		const GroundingContext& context, bool recursive) {
	auto conj = (qf->quant() == QUANT::UNIV);
	auto existential = (not conj and context._monotone == Context::POSITIVE) or (conj and context._monotone == Context::NEGATIVE);
	auto lazyAllowed = getOption(TSEITINDELAY) && (existential || context._monotone == Context::BOTH);

	ClauseGrounder* grounder = NULL;
	if (isa<SolverTheory>(*grounding) and lazyAllowed) {
		auto solvertheory = dynamic_cast<SolverTheory*>(grounding);
		grounder = new LazyExistsGrounder(solvertheory, subgrounder, gc._generator, gc._checker, context, recursive, qf->sign(), qf->quant(), gc._generates, gc._universe.size());
	} else {
		grounder = new QuantGrounder(manager, grounding, subgrounder, gc._generator, gc._checker, context, qf->sign(), qf->quant(), gc._generates, gc._universe.size());
	}
	return grounder;
}

FormulaGrounder* GrounderFactory::checkDenotationGrounder(const QuantForm* qf){
	if(not getOption(CPSUPPORT) || qf->quantVars().size()!=1){
		return NULL;
	}

	auto existsquant = not qf->isUnivWithSign();
	auto subsign = qf->subformula()->sign();
	FuncTerm* ft = NULL;
	Variable* x = NULL;

	Term *st1 = NULL, *st2 = NULL;
	auto pf = dynamic_cast<PredForm*>(qf->subformula());
	if(pf!=NULL && is(pf->symbol(), STDPRED::EQ)){
		st1 = pf->subterms()[0];
		st2 = pf->subterms()[1];
	}

	auto ef = dynamic_cast<EqChainForm*>(qf->subformula());
	if(ef!=NULL && ef->subterms().size()==2 && (ef->comps()[0]==CompType::EQ||ef->comps()[0]==CompType::NEQ) ){
		if(ef->comps()[0]==CompType::NEQ){
			subsign = not subsign;
		}
		st1 = ef->subterms()[0];
		st2 = ef->subterms()[1];
	}

	if(st1!=NULL){
		if(st1->type()==TermType::FUNC && st2->type()==TermType::VAR){
			ft = dynamic_cast<FuncTerm*>(st1);
			x = dynamic_cast<VarTerm*>(st2)->var();
		}else if(st2->type()==TermType::FUNC && st1->type()==TermType::VAR){
			ft = dynamic_cast<FuncTerm*>(st2);
			x = dynamic_cast<VarTerm*>(st1)->var();
		}
	}

	if(existsquant && subsign == SIGN::NEG){
		return NULL;
	}
	if(not existsquant && subsign == SIGN::POS){ // TODO can be simplified (forall x: c=x)
		return NULL;
	}
	if(ft==NULL
			|| x!=*qf->quantVars().begin()
			|| _structure.concrstructure->inter(x->sort())->empty()
			|| not SortUtils::isSubsort(ft->function()->outsort(), x->sort(), _vocabulary)
			|| TermUtils::contains(x,ft)
			|| not CPSupport::eligibleForCP(ft->function(), _vocabulary)){
		return NULL;
	}
	for(auto t: ft->subterms()){
		if(t->type()!=TermType::DOM && t->type()!=TermType::VAR){
			return NULL; // TODO need lazy denotation grounder for this
		}
	}

	SaveContext();
	std::vector<TermGrounder*> grounders;
	for(auto t: ft->subterms()){
		descend(t);
		grounders.push_back(getTermGrounder());
	}
	RestoreContext();
	return new DenotationGrounder(getGrounding(), existsquant?SIGN::POS:SIGN::NEG, ft, grounders, getContext());
}

void GrounderFactory::createTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc) {
	// NOTE: to reduce the number of tseitins created, negations are pushed deeper whenever relevant:
	// If qf is a negated exist, push the negation one level deeper. Take a clone to avoid changing qf;
	QuantForm* tempqf = NULL;
	if (not qf->isUniv() and qf->sign() == SIGN::NEG) {
		tempqf = qf->cloneKeepVars();
		tempqf->quant(QUANT::UNIV);
		tempqf->negate();
		subformula->negate();
	}
	auto newqf = (tempqf == NULL ? qf : tempqf);

	auto grounder = checkDenotationGrounder(newqf);
	tablesize subsize(TST_EXACT, 1);
	if (grounder == NULL) {
		// Visit subformula
		SaveContext();
		descend(subformula);
		RestoreContext();

		auto subgrounder = getFormGrounder();
		Assert(subgrounder!=NULL);
		grounder = createQ(_groundingmanager, getGrounding(), subgrounder, newqf, gc, getContext(), recursive(newqf));
		subsize = subgrounder->getMaxGroundSize();
	}
	Assert(grounder!=NULL);

	grounder->setMaxGroundSize(gc._universe.size() * subsize);

	_formgrounder = grounder;
	checkAndAddAsTopGrounder();

	if (tempqf != NULL) {
		deleteDeep(tempqf);
	}
}

void GrounderFactory::createNonTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc) {
	_formgrounder = NULL;
	SaveContext();
	_formgrounder = checkDenotationGrounder(qf);
	RestoreContext();

	tablesize subsize(TST_EXACT, 1);
	if(_formgrounder==NULL){
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

		auto subfg = _formgrounder;

		subsize = subfg->getMaxGroundSize();
		_formgrounder = createQ(_groundingmanager, getGrounding(), subfg, qf, gc, getContext(), recursive(qf));
		RestoreContext();
	}
	_formgrounder->setMaxGroundSize(gc._universe.size() * subsize);


	checkAndAddAsTopGrounder();
}

void GrounderFactory::checkAndAddAsTopGrounder() {
	if (_context._conjunctivePathFromRoot) {
		setTopGrounder(getFormGrounder());
	}
}

void GrounderFactory::visit(const EquivForm* ef) {
	if(getOption(SATISFIABILITYDELAY)){ // TODO remove this when it has been fixed properly for lazy grounding!
		auto changed = FormulaUtils::removeEquivalences(ef->cloneKeepVars());
		changed->accept(this);
		// TODO memory
		return;
	}
	_context._conjPathUntilNode = false;

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

	checkAndAddAsTopGrounder();
}

void GrounderFactory::visit(const EqChainForm* ef) {
	_context._conjPathUntilNode = _context._conjunctivePathFromRoot;
	Formula* f = ef->cloneKeepVars();
	f = FormulaUtils::splitComparisonChains(f, getGrounding()->vocabulary());
	f->accept(this);
	checkAndAddAsTopGrounder();
	deleteDeep(f);
}

void GrounderFactory::visit(const VarTerm* t) {
	Assert(varmapping().find(t->var()) != varmapping().cend());
	_termgrounder = new VarTermGrounder(getGrounding()->translator(), getConcreteStructure()->inter(t->sort()), t->var(), varmapping().find(t->var())->second);
}

void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->sort(), t->value());
}

void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> stg;
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		descend(*it);
		stg.push_back(getTermGrounder());
	}

	// Create term grounder
	auto function = t->function();
	auto agt = getArgTables(function, getConcreteStructure());
	auto ftable = getConcreteStructure()->inter(function)->funcTable();
	auto domain = getConcreteStructure()->inter(function->outsort());
	_termgrounder = NULL;
	if(getOption(BoolType::CPSUPPORT) && not recursive(t)){
		if (FuncUtils::isIntSum(function, getConcreteStructure()->vocabulary())) {
			_termgrounder = new TwinTermGrounder(getGrounding()->translator(), function, is(function, STDFUNC::ADDITION) ? TwinTT::PLUS : TwinTT::MIN, ftable, domain, stg[0], stg[1]);
		} else if (is(function, STDFUNC::UNARYMINUS) and FuncUtils::isIntFunc(function, _vocabulary)) {
			auto product = get(STDFUNC::PRODUCT, { get(STDSORT::INTSORT), get(STDSORT::INTSORT), get(STDSORT::INTSORT) }, getConcreteStructure()->vocabulary());
			auto producttable = getConcreteStructure()->inter(product)->funcTable();
			auto factorterm = new DomainTerm(get(STDSORT::INTSORT), createDomElem(-1), TermParseInfo());
			descend(factorterm);
			factorterm->recursiveDelete();
			auto factorgrounder = getTermGrounder();
			_termgrounder = new TwinTermGrounder(getGrounding()->translator(), function, TwinTT::PROD, producttable, domain, factorgrounder, stg[0]);
		} else if (FuncUtils::isIntProduct(function, getConcreteStructure()->vocabulary())) {
			_termgrounder = new TwinTermGrounder(getGrounding()->translator(), function, TwinTT::PROD, ftable, domain, stg[0], stg[1]);
		}
	}
	if(_termgrounder==NULL){
		_termgrounder = new FuncTermGrounder(getGrounding()->translator(), function, ftable, domain, agt, stg);
	}
}

void GrounderFactory::visit(const AggTerm* t) {
	if(getOption(BoolType::CPSUPPORT) && recursive(t)){
		throw IdpException("Invalid code path");
	}

	if(t->function()==AggFunction::CARD){
		t = new AggTerm(t->set()->clone(), AggFunction::SUM, t->pi());
	}

	// Create set grounder
	SaveContext();
	if (CPSupport::eligibleForCP(t->function())) {
		_context._cpablerelation = TruthValue::True;
	}
	descend(t->set());
	RestoreContext();

	// Compute domain
	SortTable* domain = NULL;
	if (CPSupport::eligibleForCP(t, getConcreteStructure())) {
		domain = TermUtils::deriveSmallerSort(t, getConcreteStructure())->interpretation();
	}

	// Create term grounder
	_termgrounder = new AggTermGrounder(getGrounding()->translator(), t->function(), domain, getSetGrounder());
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

	std::vector<const DomElemContainer*> tuple;
	for(auto freevar: s->freeVars()){
		tuple.push_back(varmapping().at(freevar));
	}
	_setgrounder = new EnumSetGrounder(tuple, getGrounding()->translator(), subgrounders);
}

void GrounderFactory::visit(const QuantSetExpr* origqs) {
	// Move three-valued terms in the set expression: from term to condition
	auto transqs = SetUtils::unnestThreeValuedTerms(origqs->clone(), getConcreteStructure(), _context._defined, getOption(CPSUPPORT), _context._cpablerelation);
	if (not isa<QuantSetExpr>(*transqs)) {
		descend(transqs);
		return;
	}
	auto newqs = dynamic_cast<QuantSetExpr*>(transqs);

	// NOTE: generator generates possibly true instances, checker checks the certainly true ones
	auto gc = createVarsAndGenerators(newqs->getCondition(), newqs, TruthType::POSS_TRUE, TruthType::CERTAIN_TRUE);

	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(newqs->getCondition());
	auto subgr = getFormGrounder();
	RestoreContext();

	// Create grounder for weight
	descend(newqs->getTerm());
	auto wgr = getTermGrounder();

	std::vector<const DomElemContainer*> tuple;
	for(auto freevar: newqs->freeVars()){
		tuple.push_back(varmapping().at(freevar));
	}
	_quantsetgrounder = new QuantSetGrounder(newqs->clone(), tuple, getGrounding()->translator(), subgr, gc._generator, gc._checker, wgr);
	newqs->recursiveDelete();
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

	setTopGrounder(new DefinitionGrounder(getGrounding(), subgrounders, _context));
}

void GrounderFactory::visit(const Rule* rule) {
	auto newrule = rule->clone();

	//TODO: if negations are already pushed, this is too much work. But on the other hand, checking if they are pushed is as expensive as pushing them
	//However, pushing negations here is important to avoid errors such as {p <- ~~p} turning into {p <- ~q; q<- ~p}
	newrule->body(FormulaUtils::pushQuantifiers(FormulaUtils::pushNegations(newrule->body())));
	newrule = DefinitionUtils::unnestThreeValuedTerms(newrule, getConcreteStructure(), _context._defined, getOption(CPSUPPORT));

	if (getOption(SATISFIABILITYDELAY)) { // NOTE: lazy grounding cannot handle head terms containing nested variables
		newrule = DefinitionUtils::unnestHeadTermsNotVarsOrDomElems(newrule, getConcreteStructure());
	}

	newrule = DefinitionUtils::moveOnlyBodyQuantifiers(newrule);

	// Split the quantified variables in two categories:
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	varlist bodyvars, headvars, nonheadvars;
	for (auto var : newrule->quantVars()) {
		if (newrule->body()->contains(var)) {
			bodyvars.push_back(var);
		} else {
			headvars.push_back(var);
		}
	}

	// NOTE: Not both sets of generators will ever be effectively used at the same time, but creating them both allows to delay this decision.
	auto headgen = createVarMapAndGenerator(newrule->head(), headvars);
	auto bodygen = createVarMapAndGenerator(newrule->body(), bodyvars);

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
	_rulegrounder = new FullRuleGrounder(newrule, headgrounder, bodygrounder, headgen, bodygen, _context);

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
InstGenerator* GrounderFactory::createVarMapAndGenerator(const Formula* original, const VarList& vars, bool safelyreuse) {
	vector<SortTable*> varsorts;
	vector<const DomElemContainer*> varcontainers;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		const DomElemContainer* varmap = NULL;
		auto varit = varmapping().find(*it);
		if (safelyreuse && varit != varmapping().cend()) {
			varmap = varit->second;
		} else {
			varmap = createVarMapping(*it);
		}
		varcontainers.push_back(varmap);
		varsorts.push_back(getConcreteStructure()->inter((*it)->sort()));
	}
	auto gen = GeneratorFactory::create(varcontainers, varsorts, original);

	if (not getOption(BoolType::GROUNDWITHBOUNDS) && not useLazyGrounding()) {
		checkGeneratorInfinite(gen, original);
	}
	return gen;
}

template<typename OrigConstruct>
GenAndChecker GrounderFactory::createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype) {
	vector<Variable*> fovars, quantfovars;
	for (auto it = orig->quantVars().cbegin(); it != orig->quantVars().cend(); ++it) {
		quantfovars.push_back(*it);
	}
	for (auto it = orig->freeVars().cbegin(); it != orig->freeVars().cend(); ++it) {
		fovars.push_back(*it);
	}

	auto dataWpattern = getPatternAndContainers(quantfovars, fovars);
	/* TODO possible future code for better grounding (faster termination)
	InstGenerator* generator = NULL;
	Assert(generatortype==TruthType::POSS_TRUE or generatortype==TruthType::POSS_FALSE);
	if(generatortype==TruthType::POSS_TRUE){
		generator = getGenerator(subformula, TruthType::CERTAIN_TRUE, dataWpattern.first, dataWpattern.second, getSymbolicStructure());
		auto generator2 = getGenerator(subformula, TruthType::POSS_TRUE, dataWpattern.first, dataWpattern.second, getSymbolicStructure());
		generator = new UnionGenerator({generator, generator2},{new EmptyGenerator(), new EmptyGenerator()});
	}else{
		generator = getGenerator(subformula, TruthType::CERTAIN_FALSE, dataWpattern.first, dataWpattern.second, getSymbolicStructure());
		auto generator2 = getGenerator(subformula, TruthType::POSS_FALSE, dataWpattern.first, dataWpattern.second, getSymbolicStructure());
		generator = new UnionGenerator({generator, generator2},{new EmptyGenerator(), new EmptyGenerator()});
	}
	 */
	auto generator = getGenerator(subformula, generatortype, dataWpattern.first, dataWpattern.second, getSymbolicStructure(), _structure.concrstructure, getContext()._defined);
	auto checker = getChecker(subformula, checkertype, dataWpattern.first, getSymbolicStructure(), _structure.concrstructure, getContext()._defined);

	vector<SortTable*> directquanttables;
	for (auto it = orig->quantVars().cbegin(); it != orig->quantVars().cend(); ++it) {
		directquanttables.push_back(getConcreteStructure()->inter((*it)->sort()));
	}

	if (not getOption(BoolType::GROUNDWITHBOUNDS) && not useLazyGrounding()) {
		checkGeneratorInfinite(generator, orig);
		checkGeneratorInfinite(checker, orig);
	}

	std::set<const DomElemContainer*> containerset;
	for(auto var : dataWpattern.first.quantfovars){
		containerset.insert(_varmapping[var]);
	}
	return GenAndChecker(containerset, generator, checker, Universe(directquanttables));
}

std::pair<GeneratorData, std::vector<Pattern> > GrounderFactory::getPatternAndContainers(std::vector<Variable*> quantfovars, std::vector<Variable*> remvars) {
	std::pair<GeneratorData, std::vector<Pattern>> both;
	auto& data = both.first;
	data.funccontext = getContext()._funccontext;
	data.quantfovars = quantfovars;
	data.fovars = data.quantfovars;
	insertAtEnd(data.fovars, remvars);
	data.structure = getConcreteStructure();
	for (auto i = data.fovars.cbegin(); i < data.fovars.cend(); ++i) {
		auto st = getConcreteStructure()->inter((*i)->sort());
		data.tables.push_back(st);
	}

	for (auto it = quantfovars.cbegin(); it != quantfovars.cend(); ++it) {
		auto d = createVarMapping(*it);
		data.containers.push_back(d);
		both.second.push_back(Pattern::OUTPUT);
	}
	for (auto it = remvars.cbegin(); it != remvars.cend(); ++it) {
		if(varmapping().find(*it) == varmapping().cend()){
			stringstream ss;
			ss <<"Could not find mapping for variable " <<toString(*it);
			throw InternalIdpException(ss.str());
		}
		// Should already have a varmapping
		data.containers.push_back(varmapping().at(*it));
		both.second.push_back(Pattern::INPUT);
	}
	return both;
}

DomElemContainer* GrounderFactory::createVarMapping(Variable* const var) {
	Assert(varmapping().find(var)==varmapping().cend());
	_context._mappedvars.insert(var);
	auto d = new DomElemContainer();
	_varmapping[var] = d;
	return d;
}

InstGenerator* GrounderFactory::getGenerator(Formula* subformula, TruthType generatortype, const GeneratorData& data, const std::vector<Pattern>& pattern, 
		SymbolicStructure symstructure, const Structure* structure, std::set<PFSymbol*> definedsymbols, bool forceexact) {
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 0) {
		clog << "Creating generator for truthtype " << print(generatortype) << " for subformula " << print(subformula);
		pushtab();
		clog << nt();
	}
	PredTable* gentable = NULL;
	bool approxastrue = generatortype == TruthType::POSS_TRUE || generatortype == TruthType::POSS_FALSE;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		gentable = createTable(subformula, generatortype, data.quantfovars, approxastrue, data, symstructure, structure, definedsymbols, forceexact);
	} else {
		if (approxastrue) {
			gentable = TableUtils::createFullPredTable(Universe(data.tables));
		} else {
			gentable = TableUtils::createPredTable(Universe(data.tables));
		}
	}
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 0) {
		poptab();
	}
	return createGen("Generator", generatortype, data, gentable, subformula, pattern);
}

//Checkers for atoms CANNOT be approximated:
// * Either they are trivial (true or false) and the approximation is uselesss
// * Or there is a reason why they are not trivial: they are used in the propagation, but no LUP has been done on them!!!
InstChecker* GrounderFactory::getChecker(Formula* subformula, TruthType checkertype, const GeneratorData& data, SymbolicStructure symstructure,
		const Structure* structure, std::set<PFSymbol*> definedsymbols, bool forceexact) {
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 0) {
		clog << "Creating Checker for truthtype " << print(checkertype) << " for subformula " << print(subformula);
		pushtab();
		clog << nt();
	}
	PredTable* checktable = NULL;
	bool approxastrue = checkertype == TruthType::POSS_TRUE || checkertype == TruthType::POSS_FALSE;
	if (getOption(BoolType::GROUNDWITHBOUNDS)) {
		checktable = createTable(subformula, checkertype, { }, approxastrue, data, symstructure, structure, definedsymbols, forceexact);
	} else {
		if (approxastrue) {
			checktable = TableUtils::createFullPredTable(Universe(data.tables));
		} else {
			checktable = TableUtils::createPredTable(Universe(data.tables));
		}
	}
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 0) {
		poptab();
	}
	return createGen("Checker", checkertype, data, checktable, subformula, std::vector<Pattern>(data.containers.size(), Pattern::INPUT));
}

InstGenerator* GrounderFactory::createGen(const std::string& name, TruthType type, const GeneratorData& data, PredTable* table, Formula* ,
		const std::vector<Pattern>& pattern) {
	auto instgen = GeneratorFactory::create(table, pattern, data.containers, Universe(data.tables));
	//In either case, the newly created tables are now useless: the bddtable is turned into a treeinstgenerator, the other are also useless
	delete (table);
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 0) {
		clog << tabs() << name << " for " << print(type) << ": \n" << tabs() << print(instgen) << "\n";
	}
	return instgen;
}

PredTable* GrounderFactory::createTable(Formula* subformula, TruthType type, const std::vector<Variable*>& quantfovars, bool approxvalue,
		const GeneratorData& data, SymbolicStructure symstructure, const Structure* structure, std::set<PFSymbol*> definedsymbols, bool forceexact) {
	auto tempsubformula = subformula->clone();
	tempsubformula = FormulaUtils::unnestTerms(tempsubformula, data.structure);
	tempsubformula = FormulaUtils::splitComparisonChains(tempsubformula);
	tempsubformula = FormulaUtils::graphFuncsAndAggs(tempsubformula, data.structure, definedsymbols, true, false, data.funccontext);
	tempsubformula = FormulaUtils::pushQuantifiers(tempsubformula);
	auto bdd = symstructure->evaluate(tempsubformula, type, structure); // !x phi(x) => generate all x possibly false
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 1) {
		clog << "For formula " << print(tempsubformula) << ", I found the following BDD (might be improved)" << nt() << print(bdd) << nt();
	}
	if(not forceexact){ // BUG: within definition, should NOT built on cf/ct of the symbols defined in it!
		bdd = improve(approxvalue, bdd, quantfovars, data.structure, symstructure, definedsymbols);
	}
	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 1) {
		clog << "Using the following (final) BDD" << nt() << print(bdd) << nt();
	}
	auto table = new PredTable(new BDDInternalPredTable(bdd, symstructure->obtainManager(), data.fovars, data.structure), Universe(data.tables));
	deleteDeep(tempsubformula);
	return table;
}

const FOBDD* GrounderFactory::simplify(const vector<Variable*>& fovars, std::shared_ptr<FOBDDManager> manager, bool approxastrue, const FOBDD* bdd,
		const std::set<PFSymbol*>& definedsymbols, double cost_per_answer, const Structure* structure) {
	fobddvarset bddvars;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		bddvars.insert(manager->getVariable(*it));
	}
	if (approxastrue) {
		bdd = manager->makeMoreTrue(bdd, definedsymbols);
			// Defined symbols have to be removed because the grounding of a definition should not simplify the body when it implies the head has to be true (could lose loops)
			// NOTE: this is not a proper fix, as it is not done when simplify is not called and also drops unnecessarily much information
		bdd = manager->makeMoreTrue(bdd, bddvars, { }, structure, cost_per_answer);
	} else {
		bdd = manager->makeMoreFalse(bdd, definedsymbols);
		bdd = manager->makeMoreFalse(bdd, bddvars, { }, structure, cost_per_answer);
	}
	return bdd;
}

const FOBDD* GrounderFactory::improve(bool approxastrue, const FOBDD* bdd, const vector<Variable*>& fovars, const Structure* structure,
		SymbolicStructure symstructure, std::set<PFSymbol*> definedsymbols) {
	if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 5) {
		clog << tabs() << "improving the following " << (approxastrue ? "maketrue" : "makefalse") << " BDD:" << "\n";
		pushtab();
		clog << tabs() << print(bdd) << "\n";
	}
	auto manager = symstructure->obtainManager();

	double cost_per_answer = 1; // TODO experiment with variations?
	double smaller_cost_per_answer =  cost_per_answer / 10;

	bdd = simplify(fovars, manager, approxastrue, bdd, definedsymbols, smaller_cost_per_answer, structure);
	// Optimize the query
	auto optimizemanager = FOBDDManager::createManager();
	auto copybdd = optimizemanager->getBDD(bdd, manager);

	fobddvarset copyvars;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		copyvars.insert(optimizemanager->getVariable(*it));
	}

	optimizemanager->optimizeQuery(copybdd, copyvars, { }, structure);

	if (getOption(IntType::VERBOSE_GEN_AND_CHECK) > 2) {
		clog << "optimized but not yet pruned BDD" << nt() << print(bdd) << nt();
	}

	// Remove certain leaves
	const FOBDD* pruned = NULL;
	pruned = simplify(fovars,optimizemanager,approxastrue,copybdd,definedsymbols,cost_per_answer,structure);

	if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 5) {
		poptab();
		clog << tabs() << "Resulted in:" << "\n";
		pushtab();
		clog << tabs() << print(pruned) << "\n";
		poptab();
	}

	return manager->getBDD(pruned, optimizemanager);
}
