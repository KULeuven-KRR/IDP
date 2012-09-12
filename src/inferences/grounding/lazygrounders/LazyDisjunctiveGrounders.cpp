/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "LazyDisjunctiveGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "LazyInst.hpp"

#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

LazyDisjunctiveGrounder::LazyDisjunctiveGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj,
		const GroundingContext& ct, bool explicitTseitins)
		: 	ClauseGrounder(groundtheory, sign, conj, ct),
			freevars(freevars),
			alreadyground(tablesize(TableSizeType::TST_EXACT, 0)),
			useExplicitTseitins(explicitTseitins) {

}

/**
 * Notifies the grounder that its tseitin occurs in the grounding and that it should add itself to the solver.
 */
void LazyDisjunctiveGrounder::notifyTheoryOccurrence(LazyInstantiation* instance, TsType type) const {
	if(useExplicitTseitins){
		bool stilldelayed = true;
		notifyGroundingRequested(-1, false, instance, stilldelayed);
	}else{
		getGrounding()->startLazyFormula(instance, type, connective() == Conn::CONJ);
	}
}

void LazyDisjunctiveGrounder::notifyGroundingRequested(int ID, bool groundall, LazyInstantiation * instance, bool& stilldelayed) const {
	CHECKTERMINATION

	vector<const DomainElement*> originst;
	overwriteVars(originst, instance->freevarinst);

	auto oldtseitin = instance->residual;
	auto lits = groundMore(groundall, instance, stilldelayed);
	Assert(not groundall or not stilldelayed);

	restoreOrigVars(originst, instance->freevarinst);

	if (not stilldelayed) { // No more grounding necessary
		//delete (instance); TODO who has deletion authority?
	}
	if(useExplicitTseitins){
		getGrounding()->add(oldtseitin, context()._tseitin, lits, connective() == Conn::CONJ, context().getCurrentDefID());
	}else{
		getGrounding()->notifyLazyAddition(lits, ID);
	}
}

// NOTE: generators are CLONED, SAVED and REUSED!
// @return true if no more grounding is necessary
litlist LazyDisjunctiveGrounder::groundMore(bool groundall, LazyInstantiation * instance, bool& stilldelayed) const {
	pushtab();

	Assert(instance->getGrounder()==this);

	prepareToGroundForVarInstance(instance);

	litlist subfgrounding;

	auto nbiterations = dynamic_cast<const LazyExistsGrounder*>(this) != NULL ? 10 : 1;
	auto counter = 0;
	auto decidedformula = false;
	while ((groundall || counter < nbiterations) && not isAtEnd(instance) && not decidedformula) {
		ConjOrDisj formula;
		formula.setType(connective());

		auto subgrounder = getLazySubGrounder(instance);
		if (getOption(VERBOSE_GROUNDING) > 1) {
			clog << "Grounding additional subformula " << toString(subgrounder) << "\n";
		}
		runSubGrounder(subgrounder, context()._conjunctivePathFromRoot, formula);

		decidedformula = increment(instance);
		if(decidedformula){
			continue;
		}

		auto groundedlit = getReification(formula, getTseitinType());
		if (decidesFormula(groundedlit)) {
			decidedformula = true;
			continue;
		}
		if (groundedlit == redundantLiteral()) {
			continue;
		}
		counter++;
		subfgrounding.push_back(groundedlit);
	}

	Assert(isAtEnd(instance) || groundall || decidedformula || not subfgrounding.empty());

	alreadyground = alreadyground + counter;

	if (decidedformula) {
		stilldelayed = false;
		auto lit = translator()->createNewUninterpretedNumber(); // NOTE this is done as there is not one literal representing true/false at the moment TODO
		getGrounding()->add( { lit });
		poptab();
		// Formula is true, so do not need to continue grounding and can delete
		// NOTE: if false, then can stop if non-ground part is true, so tseitin true
		return {redundantLiteral()==_false?lit:-lit};
	}

	if (isAtEnd(instance)) {
		stilldelayed = false;
	} else if (useExplicitTseitins) {
		auto tseitintype = context()._tseitin;
		auto newresidual = translator()->createNewUninterpretedNumber();
		subfgrounding.push_back(newresidual);
		instance->residual = newresidual;
		if (verbosity() > 3) {
			clog << "Added lazy tseitin: " << toString(instance->residual) << toString(tseitintype) << printFormula() << "[[" << instance->index << " to end ]]"
					<< nt();
		}
		getGrounding()->notifyLazyResidual(instance, tseitintype); // set on not-decide and add to watchlist
	}
	poptab();
	return subfgrounding;
}

/**
 * Initial call to run the grounder. Only called once.
 *
 * Initially, we do not ground anything, but just create a tseitin representing the whole formula.
 * Only when it occurs in the ground theory, will we start lazy grounding anything.
 */
void LazyDisjunctiveGrounder::internalRun(ConjOrDisj& formula) const {
	if (verbosity() > 0) {
		clog << "Lazy disjunctive grounder for " << toString(this) << "\n";
	}

	formula.setType(connective());

	pushtab();

	// Save the current instantiation of the free variables
	auto inst = new LazyInstantiation();
	for (auto var = freevars.cbegin(); var != freevars.cend(); ++var) {
		auto tuple = varmap().at(*var);
		inst->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	inst->setGrounder(this);
	initializeInst(inst);

	if (isAtEnd(inst)) {
		if (verbosity() > 3) {
			clog << "Empty grounder" << toString(this) << "\n";
		}
		delete (inst);
		poptab();
		return;
	}

	auto tseitintype = context()._tseitin;
	auto tseitin = translator()->translate(inst, tseitintype);

	if (isNegative()) {
		tseitin = -tseitin;
	}
	formula.literals.push_back(tseitin);
	inst->residual = tseitin;

	if (verbosity() > 3) {
		clog << "Added lazy tseitin: " << toString(tseitin) << toString(tseitintype) << printFormula() << nt();
	}

	poptab();
}

LazyExistsGrounder::LazyExistsGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q,
		InstGenerator* gen, InstChecker* checker, const GroundingContext& ct, bool explicitTseitins)
		: 	LazyDisjunctiveGrounder(freevars, groundtheory, sign, q == QUANT::UNIV, ct, explicitTseitins),
			_subgrounder(sub),
			_generator(gen),
			_checker(checker) {
	// FIXME calculate grounding size (checker!)
}
LazyExistsGrounder::~LazyExistsGrounder() {
	delete (_subgrounder);
	delete (_generator);
	delete (_checker);
}

LazyDisjGrounder::LazyDisjGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj,
		const GroundingContext& ct, bool explicitTseitins)
		: 	LazyDisjunctiveGrounder(freevars, groundtheory, sign, conj, ct, explicitTseitins),
			_subgrounders(sub) {

	auto size = tablesize(TableSizeType::TST_EXACT, 0);
	for (auto i = sub.cbegin(); i < sub.cend(); ++i) {
		size = size + (*i)->getMaxGroundSize();
	}
	setMaxGroundSize(size);
}
LazyDisjGrounder::~LazyDisjGrounder() {
	deleteList(_subgrounders);
}

void LazyDisjGrounder::initializeInst(LazyInstantiation* inst) const {
	inst->generator = NULL;
	inst->index = 0;
}
void LazyExistsGrounder::initializeInst(LazyInstantiation* inst) const {
	inst->generator = _generator->clone();
	inst->generator->begin();
}

Grounder* LazyExistsGrounder::getLazySubGrounder(LazyInstantiation*) const {
	return getSubGrounder();
}
Grounder* LazyDisjGrounder::getLazySubGrounder(LazyInstantiation* instance) const {
	return getSubGrounders()[instance->index];
}

bool LazyExistsGrounder::increment(LazyInstantiation* instance) const {
	instance->generator->operator ++();
	return _checker->check();
}
bool LazyDisjGrounder::increment(LazyInstantiation* instance) const {
	instance->index++;
	return false;
}

bool LazyExistsGrounder::isAtEnd(LazyInstantiation* instance) const {
	return instance->generator->isAtEnd();
}
bool LazyDisjGrounder::isAtEnd(LazyInstantiation* instance) const {
	return instance->index >= getSubGrounders().size();
}

void LazyExistsGrounder::prepareToGroundForVarInstance(LazyInstantiation* instance) const {
	instance->generator->setVarsAgain();
}
