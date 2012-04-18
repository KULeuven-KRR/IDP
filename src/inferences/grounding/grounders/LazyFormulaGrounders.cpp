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

LazyGrounder::LazyGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct)
		: ClauseGrounder(groundtheory, sign, conj, ct), freevars(freevars), alreadyground(tablesize(TableSizeType::TST_EXACT, 0)) {

}

tablesize LazyGrounder::getGroundedSize() const{
	return alreadyground*getSubGroundingSize();
}

LazyQuantGrounder::LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q,
		InstGenerator* gen, InstChecker* checker, const GroundingContext& ct, const tablesize& quantunivsize)
		: LazyGrounder(freevars, groundtheory, sign, q == QUANT::UNIV, ct), _subgrounder(sub), _generator(gen), _checker(checker) {
	if (getOption(GROUNDVERBOSITY) > 0) {
		clog << "Lazy quant grounder for " << toString(this) << "\n";
	}
	setMaxGroundSize(quantunivsize*sub->getMaxGroundSize());
}

tablesize LazyQuantGrounder::getSubGroundingSize() const{
	return getSubGrounder()->getGroundedSize();
}

LazyBoolGrounder::LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj,
		const GroundingContext& ct)
		: LazyGrounder(freevars, groundtheory, sign, conj, ct), _subgrounders(sub) {
	if (getOption(GROUNDVERBOSITY) > 0) {
		clog << "Lazy bool grounder for " << toString(this) << "\n";
	}
	tablesize size = tablesize(TableSizeType::TST_EXACT, 0);
	for(auto i=sub.cbegin(); i<sub.cend(); ++i){
		size = size + (*i)->getMaxGroundSize();
	}
	setMaxGroundSize(size);
}

tablesize LazyBoolGrounder::getSubGroundingSize() const{
	auto t = tablesize(TableSizeType::TST_EXACT, 0);
	for(auto i=getSubGrounders().cbegin(); i<getSubGrounders().cend();++i){
		t = t+(*i)->getGroundedSize();
	}
	return t;
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

	if (verbosity() > 3) {
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
	//inst->checker = _checker->clone(); // TODO add checker support
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
	//do{
	instance->generator->operator ++();
	//}while(not instance->checker->check() && not instance->generator->isAtEnd());  // TODO add checker support
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

	GroundClause clause;

	Lit groundedlit = redundantLiteral();
	int maxit = dynamic_cast<const LazyQuantGrounder*>(this) != NULL ? 10 : 1;
	int counter = 0;
	while (counter < maxit && not isAtEnd(instance)) {
		counter++;
		ConjOrDisj formula;
		formula = ConjOrDisj();
		formula.setType(conjunctive());

		auto subgrounder = getLazySubGrounder(instance);

		if (getOption(GROUNDVERBOSITY) > 1) {
			clog << "Grounding additional subformula " << toString(subgrounder) << "\n";
		}

		runSubGrounder(subgrounder, context()._conjunctivePathFromRoot, formula);
		groundedlit = getReification(formula, getTseitinType());
		increment(instance);

		if (groundedlit == redundantLiteral() || decidesFormula(groundedlit)) {
			break;
		}
		clause.push_back(groundedlit);
	}

	alreadyground = alreadyground + counter;

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

	if (not isAtEnd(instance)) {
		auto newresidual = translator()->createNewUninterpretedNumber();
		clause.push_back(newresidual);
		instance->residual = newresidual;
		if (verbosity() > 3) {
			clog << "Added lazy tseitin: " << toString(instance->residual) << toString(tseitintype) << printFormula() << "[[" << instance->index << " to end ]]"
					<< nt();
		}
		getGrounding()->notifyLazyResidual(instance, tseitintype, &lazyManager); // set on not-decide and add to watchlist
	}

	getGrounding()->add(oldtseitin, tseitintype, clause, conjunctive() == Conn::CONJ, context().getCurrentDefID());
	poptab();
	return isAtEnd(instance);
}

LazyUnknUnivGrounder::LazyUnknUnivGrounder(const PredForm* pf, Context context, const var2dommap& varmapping, AbstractGroundTheory* groundtheory,
		FormulaGrounder* sub, const GroundingContext& ct)
		: FormulaGrounder(groundtheory, ct), DelayGrounder(pf->symbol(), pf->args(), context, -1, groundtheory), _subgrounder(sub) {
	if (verbosity() > 2) {
		clog << "Delaying the grounding " << sub->printFormula() << " on " << toString(pf) << ".\n";
	}
	for (auto i = pf->args().cbegin(); i < pf->args().cend(); ++i) {
		auto var = dynamic_cast<VarTerm*>(*i)->var();
		_varcontainers.push_back(varmapping.at(var));
	}

	// FIXME set max ground size
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

// Returns a list of indices in List which contain the same variable
std::vector<pair<int, int> > findSameArgs(const vector<Term*>& terms) {
	std::vector<pair<int, int> > sameargs;
	std::map<Variable*, int> vartofirstocc;
	int index = 0;
	//cerr <<"Args: ";
	for (auto i = terms.cbegin(); i < terms.cend(); ++i, ++index) {
		//cerr <<toString(*i) <<", ";
		auto varterm = dynamic_cast<VarTerm*>(*i);
		if (varterm == NULL) { // If not a var, it cannot contain free variables!
			Assert((*i)->freeVars().size()==0);
			continue;
		}

		auto first = vartofirstocc.find(varterm->var());
		if (first == vartofirstocc.cend()) {
			vartofirstocc[varterm->var()] = index;
		} else {
			//cerr <<"[=" <<first->second <<"]";
			sameargs.push_back( { first->second, index });
		}
	}
	return sameargs;
}

DelayGrounder::DelayGrounder(PFSymbol* symbol, const vector<Term*>& terms, Context context, int id, AbstractGroundTheory* gt)
		: _id(id), _context(context), _isGrounding(false), _grounding(gt) {
	Assert(gt!=NULL);
	getGrounding()->translator()->notifyDelay(symbol, this);
	sameargs = findSameArgs(terms);
}

void DelayGrounder::notify(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders) {
	getGrounding()->notifyUnknBound(_context, lit, args, grounders);
}

void DelayGrounder::ground(const Lit& boundlit, const ElementTuple& args) {
	_stilltoground.push( { boundlit, args });
	if (not _isGrounding) {
		doGrounding();
	}
}

void DelayGrounder::doGrounding() {
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

	for (auto i = getSameargs().cbegin(); i < getSameargs().cend(); ++i) {
		if (headargs[i->first] != headargs[i->second]) {
			return;
		}
	}

	dominstlist boundvarinstlist = createInst(headargs);

	vector<const DomainElement*> originst;
	overwriteVars(originst, boundvarinstlist);

	ConjOrDisj formula;
	_subgrounder->run(formula);
	addToGrounding(Grounder::getGrounding(), formula);

	restoreOrigVars(originst, boundvarinstlist);
}

// @Precon: terms should be first those of the first predform, followed by those of the second
LazyTwinDelayUnivGrounder::LazyTwinDelayUnivGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, const var2dommap& varmapping,
		AbstractGroundTheory* groundtheory, FormulaGrounder* sub, const GroundingContext& ct)
		: FormulaGrounder(groundtheory, ct), DelayGrounder(symbol, terms, context, -1, groundtheory), _subgrounder(sub) {
	if (verbosity() > 1) {
		clog << "Delaying the grounding " << sub->printFormula() << " on " << "TODO" << " and " << "TODO" << ".\n"; // TODO
	}
	for (auto i = terms.cbegin(); i < terms.cend(); ++i) {
		auto var = dynamic_cast<VarTerm*>(*i)->var();
		Assert(var!=NULL);
		//cerr <<"Searching " <<toString(var) <<" in " <<toString(varmapping) <<"\n";
		//cerr <<"Searching " <<var <<"\n";
		Assert(varmapping.find(var)!=varmapping.cend());
		_varcontainers.push_back(varmapping.at(var));
	}

	// FIXME set max ground size
}

void LazyTwinDelayUnivGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
}

// set the variable instantiations
dominstlist LazyTwinDelayUnivGrounder::createInst(const ElementTuple& args) {
	dominstlist domlist;
	for (size_t i = 0; i < args.size(); ++i) {
		domlist.push_back(dominst { _varcontainers[i], args[i] });
	}
	return domlist;
}

void LazyTwinDelayUnivGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	_seen.push_back(headargs);

	for (auto other = _seen.cbegin(); other < _seen.cend(); ++other) {
		auto tuple = *other;
		tuple.insert(tuple.end(), headargs.cbegin(), headargs.cend());

		// If multiple vars are the same, checks that their instantiation are also the same!
		bool different = false;
		for (auto i = getSameargs().cbegin(); not different && i < getSameargs().cend(); ++i) {
			if (tuple[i->first] != tuple[i->second]) {
				different = true;
			}
		}
		if (different) {
			continue;
		}

		dominstlist boundvarinstlist = createInst(tuple);

		vector<const DomainElement*> originst;
		overwriteVars(originst, boundvarinstlist);

		ConjOrDisj formula;
		_subgrounder->run(formula);
		addToGrounding(Grounder::getGrounding(), formula);

		restoreOrigVars(originst, boundvarinstlist);
	}
}
