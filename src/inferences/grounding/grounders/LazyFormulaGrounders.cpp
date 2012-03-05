/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "LazyFormulaGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "GroundUtils.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void LazyGroundingManager::notifyDelayTriggered(ResidualAndFreeInst * instance) const {
	queuedtseitinstoground.push(instance);
	if (not currentlyGrounding) {
		groundMore();
	}
}

void LazyGroundingManager::groundMore() const {
	currentlyGrounding = true;

	while (queuedtseitinstoground.size() > 0) {
		CHECKTERMINATION

		auto instance = queuedtseitinstoground.front();
		queuedtseitinstoground.pop();

		vector<const DomainElement*> originst;
		overwriteVars(originst, instance->freevarinst);

		auto deleteinstance = instance->grounder->groundMore(instance);

		restoreOrigVars(originst, instance->freevarinst);

		if (deleteinstance) {
			delete (instance);
		}
	}

	currentlyGrounding = false;
}

LazyGrounder::LazyGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct) :
		ClauseGrounder(groundtheory, sign, conj, ct), freevars(freevars) {

}

LazyQuantGrounder::LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign,
		QUANT q, InstGenerator* gen, const GroundingContext& ct) :
		LazyGrounder(freevars, groundtheory, sign, q == QUANT::UNIV, ct), _subgrounder(sub), _generator(gen) {
}

LazyBoolGrounder::LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign,
		bool conj, const GroundingContext& ct) :
		LazyGrounder(freevars, groundtheory, sign, conj, ct), _subgrounders(sub) {
}

void LazyGrounder::internalRun(ConjOrDisj& formula) const {
	formula.setType(conjunctive());
	pushtab();

	if (grounderIsEmpty()) {
		return;
	}

	// Save the current instantiation of the free variables
	auto inst = new ResidualAndFreeInst();
	for (auto var = freevars.cbegin(); var != freevars.cend(); ++var) {
		auto tuple = varmap().at(*var);
		inst->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	inst->grounder = this;
	initializeInst(inst);

	auto tseitintype = context()._tseitin;

	// NOTE: initially, we do not ground anything, but just create a tseitin representing the whole formula.
	// Only when it occurs in the ground theory, will we start lazy grounding anything!
	translator()->translate(&lazyManager, inst, tseitintype);

	if (isNegative()) {
		inst->residual = -inst->residual;
	}
	formula.literals.push_back(inst->residual);

	if (verbosity() > 1) {
		clog << "Added lazy tseitin: " << toString(inst->residual) << toString(tseitintype) << printFormula() << nt();
	}

	poptab();
}

bool LazyQuantGrounder::grounderIsEmpty() const {
	_generator->begin();
	return _generator->isAtEnd();
}

bool LazyBoolGrounder::grounderIsEmpty() const {
	return _subgrounders.size() == 0;
}

void LazyBoolGrounder::initializeInst(ResidualAndFreeInst* inst) const {
	inst->generator = NULL;
	inst->index = 0;
}

void LazyQuantGrounder::initializeInst(ResidualAndFreeInst* inst) const {
	inst->generator = _generator->clone();
}

Grounder* LazyQuantGrounder::getLazySubGrounder(ResidualAndFreeInst*) const {
	auto grounder = getSubGrounder();
	return grounder;
}

Grounder* LazyBoolGrounder::getLazySubGrounder(ResidualAndFreeInst* instance) const {
	auto grounder = getSubGrounders()[instance->index];
	return grounder;
}

void LazyQuantGrounder::increment(ResidualAndFreeInst* instance) const {
	instance->generator->operator ++();
}

void LazyBoolGrounder::increment(ResidualAndFreeInst* instance) const {
	instance->index++;
}

bool LazyQuantGrounder::isAtEnd(ResidualAndFreeInst* instance) const {
	return instance->generator->isAtEnd();
}

bool LazyBoolGrounder::isAtEnd(ResidualAndFreeInst* instance) const {
	return instance->index >= getSubGrounders().size();
}

void LazyQuantGrounder::initializeGroundMore(ResidualAndFreeInst* instance) const {
	instance->generator->setVarsAgain(); // TODO check whether this is correct in all cases
}

// TODO use the checker
// NOTE: generators are CLONED, SAVED and REUSED!
// @return true if no more grounding is necessary
bool LazyGrounder::groundMore(ResidualAndFreeInst* instance) const {
	pushtab();

	Assert(instance->grounder==this);

	initializeGroundMore(instance);

	Lit groundedlit = redundantLiteral();
	while (groundedlit == redundantLiteral() && not isAtEnd(instance)) {
		ConjOrDisj formula;
		formula = ConjOrDisj();
		formula.setType(conjunctive());

		runSubGrounder(getLazySubGrounder(instance), context()._conjunctivePathFromRoot, formula);
		groundedlit = getReification(formula);
		increment(instance);
	}

	Lit oldtseitin = instance->residual;

	auto tseitintype = context()._tseitin;

	if (decidesFormula(groundedlit)) {
		getGrounding()->add(oldtseitin, tseitintype, { }, redundantLiteral() == _false, context().getCurrentDefID()); // NOTE: if false, then can stop if non-ground part is true, so tseitin true
		poptab();
		return true; // Formula is true, so do not need to continue grounding and can delete
	} else if (groundedlit == redundantLiteral() && isAtEnd(instance)) {
		getGrounding()->add(oldtseitin, tseitintype, { }, redundantLiteral() == _true, context().getCurrentDefID()); // NOTE: if false, then all are non-stoppable, so tseitin false
		poptab();
		return true; // Formula is false, so do not need to continue grounding
	}

	GroundClause clause;
	clause.push_back(groundedlit);

	if (not isAtEnd(instance)) {
		auto newresidual = translator()->createNewUninterpretedNumber();
		clause.push_back(newresidual);
		instance->residual = newresidual;
		if (verbosity() > 1) {
			clog << "Added lazy tseitin: " << toString(instance->residual) << toString(tseitintype) << printFormula() << "[[" << instance->index
					<< " to end ]]" << nt();
		}
		getGrounding()->notifyLazyResidual(instance, tseitintype, &lazyManager); // set on not-decide and add to watchlist
	}

	getGrounding()->add(oldtseitin, tseitintype, clause, conjunctive() == Conn::CONJ, context().getCurrentDefID());
	poptab();
	return isAtEnd(instance);
}

LazyUnknUnivGrounder::LazyUnknUnivGrounder(const PredForm* pf, Context context, const var2dommap& varmapping,
		AbstractGroundTheory* groundtheory, FormulaGrounder* sub, const GroundingContext& ct) :
		FormulaGrounder(groundtheory, ct), LazyUnknBoundGrounder(pf->symbol(), context, -1, groundtheory), _subgrounder(sub) {
	for(auto i=pf->args().cbegin(); i<pf->args().cend(); ++i) {
		auto var = dynamic_cast<VarTerm*>(*i)->var();
		_varcontainers.push_back(varmapping.at(var));
	}
}

void LazyUnknUnivGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
}

// set the variable instantiations
dominstlist LazyUnknUnivGrounder::createInst(const ElementTuple& args) {
	dominstlist domlist;
	for (size_t i = 0; i < args.size(); ++i) {
		domlist.push_back(dominst { _varcontainers[i], args[i] });
	}
	return domlist;
}

LazyUnknBoundGrounder::LazyUnknBoundGrounder(PFSymbol* symbol, Context context, unsigned int id, AbstractGroundTheory* gt) :
		_id(id), _isGrounding(false), _context(context), _grounding(gt) {
	Assert(gt!=NULL);
	getGrounding()->translator()->notifyDelayUnkn(symbol, this);
}

void LazyUnknBoundGrounder::notify(const Lit& lit, const ElementTuple& args, const std::vector<LazyUnknBoundGrounder*>& grounders) {
	getGrounding()->notifyUnknBound(_context, lit, args, grounders);
}

void LazyUnknBoundGrounder::ground(const Lit& boundlit, const ElementTuple& args) {
	_stilltoground.push( { boundlit, args });
	if (not _isGrounding) {
		doGrounding();
	}
}

void LazyUnknBoundGrounder::doGrounding() {
	_isGrounding = true;
	while (not _stilltoground.empty()) {
		auto elem = _stilltoground.front();
		_stilltoground.pop();
		doGround(elem.first, elem.second);
	}
	_isGrounding = false;
}

void LazyUnknUnivGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	dominstlist boundvarinstlist = createInst(headargs);

	vector<const DomainElement*> originst;
	overwriteVars(originst, boundvarinstlist);

	ConjOrDisj formula;
	_subgrounder->run(formula);
	addToGrounding(Grounder::getGrounding(), formula);

	restoreOrigVars(originst, boundvarinstlist);
}
