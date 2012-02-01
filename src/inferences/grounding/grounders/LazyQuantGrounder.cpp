/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "LazyQuantGrounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "GroundUtils.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void LazyGroundingManager::notifyBoundSatisfied(ResidualAndFreeInst * instance) {
	notifyBoundSatisfiedInternal(instance);
}

// restructured code to prevent recursion
void LazyGroundingManager::notifyBoundSatisfiedInternal(ResidualAndFreeInst* instance) const {
	// FIXME duplication and const issues!
	queuedtseitinstoground.push(instance);
	if (not currentlyGrounding) {
		groundMore();
	}
}

void LazyGroundingManager::groundMore() const {
	// if value is decided, allow to erase the formula

	currentlyGrounding = true;
	while (queuedtseitinstoground.size() > 0) {
		CHECKTERMINATION

		auto instance = queuedtseitinstoground.front();
		queuedtseitinstoground.pop();

		vector<const DomainElement*> originst;
		overwriteVars(originst, instance->freevarinst);

		instance->grounder->groundMore(instance);

		restoreOrigVars(originst, instance->freevarinst);
	}

	currentlyGrounding = false;
}

LazyQuantGrounder::LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q,
		InstGenerator* gen, InstChecker* checker, const GroundingContext& ct)
		: QuantGrounder(groundtheory, sub, sign, q, gen, checker, ct),
		  freevars(freevars) {
}

// TODO delete free var ref and generator when at end
// TODO use the checker
// NOTE: generators are CLONED, SAVED and REUSED!

void LazyQuantGrounder::groundMore(ResidualAndFreeInst* instance) const {
	pushtab();
	if (verbosity() > 2) {
		clog <<"CONTINUED: ";
		printorig();
	}

	auto generator = instance->generator;

	Lit groundedlit = redundantLiteral();
	while(groundedlit==redundantLiteral() && not generator->isAtEnd()){
		ConjOrDisj formula;
		formula = ConjOrDisj();
		formula.setType(conn_);

		runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, formula);
		groundedlit = getReification(formula);
		generator->operator ++();
	}

	Lit oldtseitin = instance->residual;

	// FIXME always adding EQ instead of the original tseitin
	auto tseitintype = context()._tseitin;
	if(context()._tseitin!=TsType::RULE){
		tseitintype = TsType::EQ;
	}

	if(decidesFormula(groundedlit)){
		getGrounding()->add(oldtseitin, tseitintype, {}, redundantLiteral()==_false, context().getCurrentDefID()); // NOTE: if false, then can stop if non-ground part is true, so tseitin true
		poptab();
		return; // Formula is true, so do not need to continue grounding
	}else if(groundedlit==redundantLiteral() && generator->isAtEnd()){
		getGrounding()->add(oldtseitin, tseitintype, {},redundantLiteral()==_true, context().getCurrentDefID());  // NOTE: if false, then all are non-stoppable, so tseitin false
		poptab();
		return; // Formula is false, so do not need to continue grounding
	}

	GroundClause clause;
	clause.push_back(groundedlit);

	// TODO notify lazy should check whether the tseitin already has a value and request more grounding immediately!
	if (not generator->isAtEnd()) {
		Lit newresidual = translator()->createNewUninterpretedNumber();
		clause.push_back(newresidual);
		instance->residual = newresidual;
		// TODO optimize by only watching truth or falsity in pure monotone or anti-monotone contexts
		getGrounding()->notifyLazyResidual(instance, &lazyManager); // set on not-decide and add to watchlist
	} else {
		// TODO deletion
	}

	// FIXME always watching both signs

	getGrounding()->add(oldtseitin, tseitintype, clause, conn_ == Conn::CONJ, context().getCurrentDefID());
	poptab();
}

void LazyQuantGrounder::internalRun(ConjOrDisj& formula) const {
	pushtab();
	formula.setType(Conn::DISJ);
	if (verbosity() > 2) {
		clog <<"INITIAL: ";
		printorig();
	}

	_generator->begin();
	if (_generator->isAtEnd()) {
		return;
	}

	// Save the current instantiation of the free variables
	auto inst = new ResidualAndFreeInst();
	for (auto var = freevars.cbegin(); var != freevars.cend(); ++var) {
		auto tuple = varmap().at(*var);
		inst->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	inst->grounder = this;
	inst->generator = _generator->clone();

	// NOTE: initially, we do not ground anything, but just create a tseitin representing the whole formula.
	// Only when it occurs in the ground theory, will we start lazy grounding anything!
	translator()->translate(&lazyManager, inst, context()._tseitin);

	if (isNegative()) {
		inst->residual = -inst->residual;
	}
	formula.literals.push_back(inst->residual);
	poptab();
}

LazyBoolGrounder::LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj, const GroundingContext& ct)
		: BoolGrounder(groundtheory, sub, sign, conj, ct),
		  freevars(freevars) {
}

void LazyBoolGrounder::groundMore(ResidualAndFreeInst* instance) const {
	pushtab();
	if (verbosity() > 2) {
		clog <<"CONTINUED: ";
		printorig();
	}

	auto& index = instance->index;

	Lit groundedlit = redundantLiteral();
	while(groundedlit==redundantLiteral() && index<getSubGrounders().size()){
		ConjOrDisj formula;
		formula = ConjOrDisj();
		formula.setType(conn_);

		runSubGrounder(getSubGrounders()[index], context()._conjunctivePathFromRoot, formula);
		groundedlit = getReification(formula);
		++index;
	}

	Lit oldtseitin = instance->residual;

	// FIXME always adding EQ instead of the original tseitin
	auto tseitintype = context()._tseitin;
	if(context()._tseitin!=TsType::RULE){
		tseitintype = TsType::EQ;
	}

	if(decidesFormula(groundedlit)){
		getGrounding()->add(oldtseitin, tseitintype, {}, redundantLiteral()==_false, context().getCurrentDefID()); // NOTE: if false, then can stop if non-ground part is true, so tseitin true
		poptab();
		return; // Formula is true, so do not need to continue grounding
	}else if(groundedlit==redundantLiteral() && index>=getSubGrounders().size()){
		getGrounding()->add(oldtseitin, tseitintype, {},redundantLiteral()==_true, context().getCurrentDefID());  // NOTE: if false, then all are non-stoppable, so tseitin false
		poptab();
		return; // Formula is false, so do not need to continue grounding
	}

	GroundClause clause;
	clause.push_back(groundedlit);

	// TODO notify lazy should check whether the tseitin already has a value and request more grounding immediately!
	if (index<getSubGrounders().size()) {
		Lit newresidual = translator()->createNewUninterpretedNumber();
		clause.push_back(newresidual);
		instance->residual = newresidual;
		// TODO optimize by only watching truth or falsity in pure monotone or anti-monotone contexts
		getGrounding()->notifyLazyResidual(instance, &lazyManager); // set on not-decide and add to watchlist
	} else {
		// TODO deletion
	}

	// FIXME always watching both signs

	getGrounding()->add(oldtseitin, tseitintype, clause, conn_ == Conn::CONJ, context().getCurrentDefID());
	poptab();
}

void LazyBoolGrounder::internalRun(ConjOrDisj& formula) const {
	formula.setType(conn_);
	pushtab();
	if (verbosity() > 2) {
		clog <<"INITIAL: ";
		printorig();
	}

	if(getSubGrounders().size()==0){
		return;
	}

	// Save the current instantiation of the free variables
	auto inst = new ResidualAndFreeInst();
	for (auto var = freevars.cbegin(); var != freevars.cend(); ++var) {
		auto tuple = varmap().at(*var);
		inst->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	inst->grounder = this;
	inst->generator = NULL;
	inst->index = 0;

	// NOTE: initially, we do not ground anything, but just create a tseitin representing the whole formula.
	// Only when it occurs in the ground theory, will we start lazy grounding anything!
	translator()->translate(&lazyManager, inst, context()._tseitin);

	if (isNegative()) {
		inst->residual = -inst->residual;
	}
	formula.literals.push_back(inst->residual);
	poptab();
}
