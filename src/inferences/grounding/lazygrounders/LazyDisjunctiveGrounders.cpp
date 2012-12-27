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

#include "LazyDisjunctiveGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/BasicCheckersAndGenerators.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "LazyInst.hpp"

#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

LazyDisjunctiveGrounder::LazyDisjunctiveGrounder(AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct, bool explicitTseitins)
		: 	ClauseGrounder(groundtheory, sign, conj, ct),
			alreadyground(tablesize(TableSizeType::TST_EXACT, 0)),
			useExplicitTseitins(explicitTseitins),
			_instance(NULL) {

}

LazyDisjunctiveGrounder::~LazyDisjunctiveGrounder() {
	if (_instance != NULL) {
		if (_instance->generator != NULL) {
			delete (_instance->generator);
		}
		delete (_instance);
	}
}

/**
 * Notifies the grounder that its tseitin occurs in the grounding and that it should add itself to the solver.
 */
void LazyDisjunctiveGrounder::notifyTheoryOccurrence(LazyInstantiation* instance, TsType type) const {
	if (useExplicitTseitins) {
		bool stilldelayed = true;
		notifyGroundingRequested(-1, false, instance, stilldelayed);
	} else {
		getGrounding()->startLazyFormula(instance, type, connective() == Conn::CONJ);
	}
}

void LazyDisjunctiveGrounder::notifyGroundingRequested(int ID, bool groundall, LazyInstantiation * instance, bool& stilldelayed) const {
	vector<const DomainElement*> originst;
	overwriteVars(originst, instance->freevarinst);

	auto oldtseitin = instance->residual;
	auto lits = groundMore(groundall, instance, stilldelayed);
	Assert(not groundall or not stilldelayed);

	restoreOrigVars(originst, instance->freevarinst);

	if (useExplicitTseitins) {
		getGrounding()->add(oldtseitin, getTseitinType(), lits, connective() == Conn::CONJ, getContext().getCurrentDefID());
	} else {
		getGrounding()->notifyLazyAddition(lits, ID);
	}
}

bool LazyDisjunctiveGrounder::isRedundant(Lit l) const {
	return connective()==Conn::CONJ ? l == _true : l == _false;
}

bool LazyDisjunctiveGrounder::decidesFormula(Lit l) const {
	return connective()==Conn::CONJ ? l == _false : l == _true;
}

// NOTE: generators are CLONED, SAVED and REUSED!
// @return true if no more grounding is necessary
litlist LazyDisjunctiveGrounder::groundMore(bool groundall, LazyInstantiation * instance, bool& stilldelayed) const {
	pushtab();

	Assert(instance->getGrounder()==this);

	prepareToGroundForVarInstance(instance);

	litlist subfgrounding;

	if (getOption(VERBOSE_GROUNDING) > 1) {
		clog << "Grounding lazy tseitin " << toString(instance->residual) << "\n";
	}

	auto nbiterations = isa<LazyExistsGrounder>(*this) ? getOption(EXISTSEXPANSIONSTEPS) : 3;
	auto counter = 0;
	auto decidedformula = false;
	while ((groundall || counter < nbiterations) && not isAtEnd(instance) && not decidedformula) {
		ConjOrDisj formula;
		formula.setType(connective());

		auto subgrounder = getLazySubGrounder(instance);
		if (getOption(VERBOSE_GROUNDING) > 1) {
			clog << "Grounding additional subformula " << print(subgrounder) << "\n";
		}
		auto lgr = LazyGroundingRequest( { });
		runSubGrounder(subgrounder, getContext()._conjunctivePathFromRoot, conjunctiveWithSign(), formula, lgr);

		decidedformula = incrementAndCheckDecided(instance);
		if (decidedformula) {
			continue;
		}

		auto groundedlit = getReification(formula, getTseitinType());
		if (decidesFormula(groundedlit)) {
			decidedformula = true;
			continue;
		}
		if (isRedundant(groundedlit)) {
			continue;
		}
		counter++;
		subfgrounding.push_back(groundedlit);
	}

	Assert(isAtEnd(instance) || groundall || decidedformula || not subfgrounding.empty());

	alreadyground = alreadyground + counter;

	if (decidedformula) {
		stilldelayed = false;
		auto lit = translator()->createNewUninterpretedNumber(); // TODO creating this tseitin is done as there is not one literal representing true/false at the moment
		getGrounding()->add( { lit });
		poptab();
		// Formula is true, so do not need to continue grounding and can delete
		// NOTE: if false, then can stop if non-ground part is true, so tseitin true
		return {isRedundant(_false)?lit:-lit};
	}

	if (isAtEnd(instance)) {
		stilldelayed = false;
	} else if (useExplicitTseitins) {
		auto tseitintype = getTseitinType();
		auto newresidual = translator()->createNewUninterpretedNumber();
		subfgrounding.push_back(newresidual);
		instance->residual = newresidual;
		if (verbosity() > 3) {
			clog << "Added explicit lazy tseitin: " << print(instance->residual) << print(tseitintype) << print(getFormula()) << "[[" << instance->index
					<< " to end ]]" << nt();
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
void LazyDisjunctiveGrounder::internalClauseRun(ConjOrDisj& formula, LazyGroundingRequest&) {
	formula.setType(connective());

	pushtab();

	// Save the current instantiation of the free variables
	_instance = new LazyInstantiation(this);
	for (auto var = getFormula()->freeVars().cbegin(); var != getFormula()->freeVars().cend(); ++var) {
		auto tuple = getVarmapping().at(*var);
		_instance->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	initializeInst(_instance);

	if (isAtEnd(_instance)) {
		if (verbosity() > 3) {
			clog << "Empty grounder for the formula " << print(this) << "\n";
		}
		delete (_instance);
		_instance = NULL;
		poptab();
		return;
	}

	auto tseitintype = getTseitinType();
	auto tseitin = translator()->reify(_instance, tseitintype);

	if (isNegative()) {
		tseitin = -tseitin;
	}
	formula.literals.push_back(tseitin);
	_instance->residual = tseitin;

	if (verbosity() > 3) {
		clog << "Added lazy tseitin: " << print(tseitin) << print(tseitintype) << print(this) << nt();
	}

	poptab();
}

LazyExistsGrounder::LazyExistsGrounder(AbstractGroundTheory* groundtheory, FormulaGrounder* sub, InstGenerator* gen, InstChecker* checker,
		const GroundingContext& ct, bool explicitTseitins, SIGN sign, QUANT quant, const std::set<const DomElemContainer*>& generates, const tablesize& quantsize)
		: LazyDisjunctiveGrounder(groundtheory, sign, quant == QUANT::UNIV, ct, explicitTseitins), _subgrounder(sub), _generator(gen), _checker(checker) {

	addAll(_varmap, sub->getVarmapping());
	varset generatedvars;
	for (auto var : sub->getFormula()->freeVars()) {
		if (contains(generates, _varmap.at(var))) {
			generatedvars.insert(var);
		}
	}
	setFormula(new QuantForm(sign, quant, generatedvars, sub->getFormula()->cloneKeepVars(), { }));

	setMaxGroundSize(quantsize * sub->getMaxGroundSize());
}
LazyExistsGrounder::~LazyExistsGrounder() {
	delete (_subgrounder);
	delete (_generator);
	delete (_checker);
}

LazyDisjGrounder::LazyDisjGrounder(AbstractGroundTheory* groundtheory, std::vector<FormulaGrounder*> sub, SIGN sign,
		bool conj, const GroundingContext& ct, bool explicitTseitins)
		: LazyDisjunctiveGrounder(groundtheory, sign, conj, ct, explicitTseitins), _subgrounders(sub) {

	std::vector<Formula*> formulas;
	for (auto sg : _subgrounders) {
		addAll(_varmap, sg->getVarmapping());
		formulas.push_back(sg->getFormula()->cloneKeepVars());
	}
	setFormula(new BoolForm(sign, conj, formulas, { }));

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
	inst->index = 0;
}
void LazyExistsGrounder::initializeInst(LazyInstantiation* inst) const {
	inst->generator = _generator->clone();
	inst->generator->begin();
}

FormulaGrounder* LazyExistsGrounder::getLazySubGrounder(LazyInstantiation*) const {
	return getSubGrounder();
}
FormulaGrounder* LazyDisjGrounder::getLazySubGrounder(LazyInstantiation* instance) const {
	return getSubGrounders()[instance->index];
}

bool LazyExistsGrounder::incrementAndCheckDecided(LazyInstantiation* instance) const {
//	cerr <<"Generator: " <<toString(instance->generator) <<"\n";
//	cerr <<"Checker: " <<toString(_checker) <<"\n";
	instance->generator->operator ++();
	return _checker->check();
}
bool LazyDisjGrounder::incrementAndCheckDecided(LazyInstantiation* instance) const {
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
