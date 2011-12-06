#include "inferences/grounding/grounders/FormulaGrounders.hpp"

#include <iostream>
#include "vocabulary.hpp"
#include "ecnf.hpp"
#include "inferences/grounding/grounders/TermGrounders.hpp"
#include "inferences/grounding/grounders/SetGrounders.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"
#include "common.hpp"
#include "generators/InstGenerator.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "checker.hpp"
#include <cmath>

using namespace std;

//TODO: a lot of the "int" returns here should be "Lit": Issue 57199

FormulaGrounder::FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct) :
		Grounder(grounding, ct), _verbosity(0) {
}

GroundTranslator* FormulaGrounder::translator() const {
	return grounding()->translator();
}

void FormulaGrounder::setOrig(const Formula* f, const map<Variable*, const DomElemContainer*>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*, Variable*> mvv;
	for (auto it = f->freeVars().cbegin(); it != f->freeVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), ParseInfo());
		mvv[*it] = v;
		_varmap[*it] = mvd.find(*it)->second;
		_origvarmap[v] = mvd.find(*it)->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	clog << "Grounding formula " <<toString(_origform);
	if (not _origform->freeVars().empty()) {
		clog << " with instance ";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			clog << (*it)->toString() << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << e->toString() << ' ';
		}
	}
	clog << "\n";
}

AtomGrounder::AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol* s, const vector<TermGrounder*>& sg,
		const vector<const DomElemContainer*>& checkargs, InstChecker* pic, InstChecker* cic, PredInter* inter, const vector<SortTable*>& vst,
		const GroundingContext& ct) :
		FormulaGrounder(grounding, ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic), _symbol(translator()->addSymbol(s)), _tables(vst), _sign(
				sign), _checkargs(checkargs), _inter(inter) {
	gentype = ct.gentype;
}

Lit AtomGrounder::run() const {
	if (verbosity() > 2) {
		printorig();
	}

	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());

	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if (groundsubterms[n].isVariable) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
			// Check partial functions
			if (args[n] == NULL) {
				thrownotyetimplemented("Partial function issue in grounding an atom.");
				// FIXME what should happen here?
				/*//TODO: produce a warning!
				 if(context()._funccontext == Context::BOTH) {
				 // TODO: produce an error
				 }
				 if(verbosity() > 2) {
				 clog << "Partial function went out of bounds\n";
				 clog << "Result is " << (context()._funccontext != Context::NEGATIVE  ? "true" : "false") << "\n";
				 }
				 return context()._funccontext != Context::NEGATIVE  ? _true : _false;*/
			}

			// Checking out-of-bounds
			if (not _tables[n]->contains(args[n])) {
				if (verbosity() > 2) {
					clog << "Term value out of predicate type\n";
					clog << "Result is " << (isPos(_sign) ? "false" : "true") << "\n";
				}
				return isPos(_sign) ? _false : _true;
			}
		}
	}

	Assert(alldomelts);
	// If P(t) and (not isCPSymbol(P)) and isCPSymbol(t) then it should have been rewritten, right?

	// Run instance checkers
	// NOTE: set all the variables representing the subterms to their current value (these are used in the checkers)
	for (size_t n = 0; n < args.size(); ++n) {
		*(_checkargs[n]) = args[n];
	}
	if (not _pchecker->check()) { // Literal is irrelevant in its occurrences
		if (verbosity() > 2) {
			clog << "Possible checker failed\n";
			clog << "Result is " << (gentype == GenType::CANMAKETRUE ? "false" : "true") << "\n";
		}
		return gentype == GenType::CANMAKETRUE ? _false : _true;
	}
	if (_cchecker->check()) { // Literal decides formula if checker succeeds
		if (verbosity() > 2) {
			clog << "Certain checker succeeded\n";
			clog << "Result is " << translator()->printLit(gentype == GenType::CANMAKETRUE ? _true : _false, false) << "\n"; //TODO longnames?
		}
		return gentype == GenType::CANMAKETRUE ? _true : _false;
	}
	if (_inter->isTrue(args)) {
		return isPos(_sign) ? _true : _false;
	}
	if (_inter->isFalse(args)) {
		return isPos(_sign) ? _false : _true;
	}

	// Return grounding
	Lit lit = translator()->translate(_symbol, args);
	if (isNeg(_sign)) {
		lit = -lit;
	}
	if (verbosity() > 2) {
		clog << "Result is " << translator()->printLit(lit, false) << "\n"; // TODO longnames?
	}
	return lit;
}

void AtomGrounder::run(ConjOrDisj& formula) const {
	formula.literals.push_back(run());
}

Lit ComparisonGrounder::run() const {
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();

	//TODO Is following check necessary??
	if ((not left._domelement && not left._varid) || (not right._domelement && not right._varid)) {
		return context()._funccontext != Context::NEGATIVE ? _true : _false;
	}

	//TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???

	if (left.isVariable) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if (right.isVariable) {
			CPBound rightbound(right._varid);
			return translator()->translate(leftterm, _comparator, rightbound, TsType::EQ); //TODO use _context._tseitin?
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			return translator()->translate(leftterm, _comparator, rightbound, TsType::EQ); //TODO use _context._tseitin?
		}
	} else {
		Assert(not left.isVariable);
		int leftvalue = left._domelement->value()._int;
		if (right.isVariable) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			return translator()->translate(rightterm, invertComp(_comparator), leftbound, TsType::EQ); //TODO use _context._tseitin?
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			switch (_comparator) {
			case CompType::EQ:
				return leftvalue == rightvalue ? _true : _false;
			case CompType::NEQ:
				return leftvalue != rightvalue ? _true : _false;
			case CompType::LEQ:
				return leftvalue <= rightvalue ? _true : _false;
			case CompType::GEQ:
				return leftvalue >= rightvalue ? _true : _false;
			case CompType::LT:
				return leftvalue < rightvalue ? _true : _false;
			case CompType::GT:
				return leftvalue > rightvalue ? _true : _false;
			}
		}
	}
}

void ComparisonGrounder::run(ConjOrDisj& formula) const {
	formula.literals.push_back(run()); // TODO can do better?
}

CompType Agg2Comp(AGG_COMP_TYPE comp) {
	switch (comp) {
	case AGG_EQ:
		return CompType::EQ;
	case AGG_LT:
		return CompType::LT;
	case AGG_GT:
		return CompType::GT;
	}
}

/**
 * Negate the comparator and invert the sign of the tseitin when the aggregate is in a doubly negated context.
 */
//TODO:why?
Lit AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	TsType tp = context()._tseitin;
	int tseitin = translator()->translate(boundvalue, negateComp(Agg2Comp(_comp)),  _type, setnr, tp);
	return isPos(_sign) ? -tseitin : tseitin;
}

Lit AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	const TsSet& tsset = translator()->groundset(setnr);
	int maxposscard = tsset.size();
	TsType tp = context()._tseitin;
	bool simplify = false;
	bool conj;
	bool negateset;
	switch (_comp) {
	case AGG_EQ:
		if (leftvalue < 0 || leftvalue > maxposscard) {
			return isPos(_sign) ? _false : _true;
		} else if (leftvalue == 0) {
			simplify = true;
			conj = true;
			negateset = true;
		} else if (leftvalue == maxposscard) {
			simplify = true;
			conj = true;
			negateset = false;
		}
		break;
	case AGG_LT:
		if (leftvalue < 0) {
			return isPos(_sign) ? _true : _false;
		} else if (leftvalue == 0) {
			simplify = true;
			conj = false;
			negateset = false;
		} else if (leftvalue == maxposscard - 1) {
			simplify = true;
			conj = true;
			negateset = false;
		} else if (leftvalue >= maxposscard) {
			return isPos(_sign) ? _false : _true;
		}
		break;
	case AGG_GT:
		if (leftvalue <= 0) {
			return isPos(_sign) ? _false : _true;
		} else if (leftvalue == 1) {
			simplify = true;
			conj = true;
			negateset = true;
		} else if (leftvalue == maxposscard) {
			simplify = true;
			conj = false;
			negateset = true;
		} else if (leftvalue > maxposscard) {
			return isPos(_sign) ? _true : _false;
		}
		break;
	}
	if (isNeg(_sign)) {
		if (tp == TsType::IMPL)
			tp = TsType::RIMPL;
		else if (tp == TsType::RIMPL)
			tp = TsType::IMPL;
	}
	if (simplify) {
		if (_doublenegtseitin) {
			if (negateset) {
				int tseitin = translator()->translate(tsset.literals(), !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			} else {
				vector<int> newsetlits(tsset.size());
				for (unsigned int n = 0; n < tsset.size(); ++n)
					newsetlits[n] = -tsset.literal(n);
				int tseitin = translator()->translate(newsetlits, !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			}
		} else {
			if (negateset) {
				vector<int> newsetlits(tsset.size());
				for (unsigned int n = 0; n < tsset.size(); ++n)
					newsetlits[n] = -tsset.literal(n);
				int tseitin = translator()->translate(newsetlits, conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			} else {
				int tseitin = translator()->translate(tsset.literals(), conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			}
		}
	} else {
		if (_doublenegtseitin)
			return handleDoubleNegation(double(leftvalue), setnr);
		else {
			CompType comp = Agg2Comp(_comp);
			int tseitin = translator()->translate(double(leftvalue), comp, AggFunction::CARD, setnr, tp);
			return isPos(_sign) ? tseitin : -tseitin;
		}
	}
}

/**
 * General finish method for grounding of sum, product, minimum and maximum aggregates.
 * Checks whether the aggregate will be certainly true or false, based on minimum and maximum possible values and the given bound;
 * and creates a tseitin, handling double negation when necessary;
 */
Lit AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const {
	// Check minimum and maximum possible values against the given bound
	switch (_comp) { //TODO more complicated propagation is possible!
	case AGG_EQ:
		if (minpossvalue > boundvalue || maxpossvalue < boundvalue)
			return isPos(_sign) ? _false : _true;
		break;
	case AGG_LT:
		if (boundvalue < minpossvalue)
			return isPos(_sign) ? _true : _false;
		else if (boundvalue >= maxpossvalue)
			return isPos(_sign) ? _false : _true;
		break;
	case AGG_GT:
		if (boundvalue > maxpossvalue)
			return isPos(_sign) ? _true : _false;
		else if (boundvalue <= minpossvalue)
			return isPos(_sign) ? _false : _true;
		break;
	}
	if (_doublenegtseitin)
		return handleDoubleNegation(newboundvalue, setnr);
	else {
		int tseitin;
		TsType tp = context()._tseitin;
		if (isNeg(_sign)) {
			if (tp == TsType::IMPL)
				tp = TsType::RIMPL;
			else if (tp == TsType::RIMPL)
				tp = TsType::IMPL;
		}
		tseitin = translator()->translate(newboundvalue, Agg2Comp(_comp), _type, setnr, tp);
		return isPos(_sign) ? tseitin : -tseitin;
	}
}

Lit AggGrounder::run() const {
	// Run subgrounders
	int setnr = _setgrounder->run();
	const GroundTerm& groundbound = _boundgrounder->run();
	Assert(not groundbound.isVariable);

	const DomainElement* bound = groundbound._domelement;

	// Retrieve the set, note that weights might be changed when handling min and max aggregates.
	TsSet& tsset = translator()->groundset(setnr);

	// Retrieve the value of the bound
	double boundvalue = bound->type() == DET_INT ? (double) bound->value()._int : bound->value()._double;

	// Compute the value of the aggregate based on weights of literals that are certainly true.
	double truevalue = applyAgg(_type, tsset.trueweights());

	// When the set is empty, return an answer based on the current value of the aggregate.
	if (tsset.empty()) {
		bool returnvalue;
		switch (_comp) {
		case AGG_LT:
			returnvalue = boundvalue < truevalue;
			break;
		case AGG_GT:
			returnvalue = boundvalue > truevalue;
			break;
		case AGG_EQ:
			returnvalue = boundvalue == truevalue;
			break;
		}
		return isPos(_sign) == returnvalue ? _true : _false;
	}

	// Handle specific aggregates.
	int tseitin;
	double minpossvalue = truevalue;
	double maxpossvalue = truevalue;
	switch (_type) {
	case AggFunction::CARD:
		tseitin = finishCard(truevalue, boundvalue, setnr);
		break;
	case AggFunction::SUM:
		// Compute the minimum and maximum possible value of the sum.
		for (unsigned int n = 0; n < tsset.size(); ++n) {
			if (tsset.weight(n) > 0)
				maxpossvalue += tsset.weight(n);
			else if (tsset.weight(n) < 0)
				minpossvalue += tsset.weight(n);
		}
		// Finish
		tseitin = finish(boundvalue, (boundvalue - truevalue), minpossvalue, maxpossvalue, setnr);
		break;
	case AggFunction::PROD: {
		// Compute the minimum and maximum possible value of the product.
		bool containsneg = false;
		for (unsigned int n = 0; n < tsset.size(); ++n) {
			if (abs(tsset.weight(n)) > 1) {
				maxpossvalue *= abs(tsset.weight(n));
			} else if(tsset.weight(n) != 0){
				minpossvalue *= abs(tsset.weight(n));
			}
			if (tsset.weight(n) < 0)
				containsneg = true;
		}
		if (containsneg)
			minpossvalue = (-maxpossvalue < minpossvalue ? -maxpossvalue : minpossvalue);
		// Finish
		tseitin = finish(boundvalue, (boundvalue / truevalue), minpossvalue, maxpossvalue, setnr);
		break;
	}
	case AggFunction::MIN:
		// Compute the minimum possible value of the set.
		for (unsigned int n = 0; n < tsset.size(); ++n) {
			minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
			// Decrease all weights greater than truevalue to truevalue. // TODO why not just drop all those?
			// FIXME: what if some set is used in multiple expressions? Then we are changing weights???
			// TODO: what advantage does this have? -> Issue 55773
			if (tsset.weight(n) > truevalue)
				tsset.setWeight(n, truevalue);
		}
		// Finish
		tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		break;
	case AggFunction::MAX:
		// Compute the maximum possible value of the set.
		for (unsigned int n = 0; n < tsset.size(); ++n) {
			maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
			// Increase all weights less than truevalue to truevalue. // TODO why not just drop all those?
			if (tsset.weight(n) < truevalue)
				tsset.setWeight(n, truevalue);
		}
		// Finish
		tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		break;
	}
	return tseitin;
}

void AggGrounder::run(ConjOrDisj& formula) const {
	formula.literals.push_back(run()); // TODO can do better?
}

bool ClauseGrounder::isRedundantInFormula(Lit l, bool negated) const {
	if (negated) {
		return conn_ == Conn::CONJ ? l == _false : l == _true;
	} else {
		return conn_ == Conn::CONJ ? l == _true : l == _false;
	}
}

/**
 * conjunction: never true by a literal
 * disjunction: true if literal is true
 * negated conjunction: true if literal is false
 * negated disjunction: never true by a literal
 */
bool ClauseGrounder::makesFormulaTrue(Lit l, bool negated) const {
	return (negated && conn_ == Conn::CONJ && l == _false) || (~negated && conn_ == Conn::DISJ && l == _true);
}

/**
 * conjunction: false if a literal is false
 * disjunction: never false by a literal
 * negated conjunction: never false by a literal
 * negated disjunction: false if a literal is true
 */
bool ClauseGrounder::makesFormulaFalse(Lit l, bool negated) const {
	return (negated && conn_ == Conn::DISJ && l == _true) || (not negated && conn_ == Conn::CONJ && l == _false);
}

Lit ClauseGrounder::getEmtyFormulaValue() const {
	return conjunctive() ? _true : _false;
}

TsType ClauseGrounder::getTseitinType() const {
	return context()._tseitin;
}

// Takes context into account!
Lit ClauseGrounder::createTseitin(const ConjOrDisj& formula) const {
	TsType type = getTseitinType();
	if (isNegative()) {
		type = reverseImplication(type);
	}

	Lit tseitin;
	bool asConjunction = formula.type == Conn::CONJ;
	if (negativeDefinedContext()) {
		asConjunction = not asConjunction;
	}
	tseitin = translator()->translate(formula.literals, asConjunction, type);
	return isNegative() ? -tseitin : tseitin;
}

Lit ClauseGrounder::getReification(const ConjOrDisj& formula) const {
	if (formula.literals.empty()) {
		return getEmtyFormulaValue();
	}

	if (formula.literals.size() == 1) {
		return formula.literals[0];
	}

	return createTseitin(formula);
}

void ClauseGrounder::run(ConjOrDisj& formula) const {
	run(formula, isNegative());
}

FormStat ClauseGrounder::runSubGrounder(Grounder* subgrounder, bool conjFromRoot, ConjOrDisj& formula, bool negated) const {
	ConjOrDisj subformula;
	subgrounder->run(subformula);

	if (subformula.literals.size() == 0) {
		Lit value = subformula.type == Conn::CONJ ? _true : _false;
		if (makesFormulaTrue(value, negated)) {
			formula.literals = litlist { negated ? -_true : _true };
			return FormStat::DECIDED;
		} else if (makesFormulaFalse(value, negated)) {
			formula.literals = litlist { negated ? -_false : _false };
			return FormStat::DECIDED;
		} else if (subformula.type != formula.type) {
			formula.literals.push_back(negated ? -value : value);
			return (FormStat::UNKNOWN);
		}
		return (FormStat::UNKNOWN);
	} else if (subformula.literals.size() == 1) {
		Lit l = subformula.literals[0];
		if (makesFormulaFalse(l, negated)) {
			formula.literals = litlist { negated ? -_false : _false };
			return FormStat::DECIDED;
		} else if (makesFormulaTrue(l, negated)) {
			formula.literals = litlist { negated ? -_true : _true };
			return FormStat::DECIDED;
		} else if (not isRedundantInFormula(l, negated)) {
			formula.literals.push_back(negated ? -l : l);
		}
		return FormStat::UNKNOWN;
	} // otherwise INVAR: subformula is not true nor false and does not contain true nor false literals
	if (conjFromRoot && conjunctive()) {
		if (subformula.type == Conn::CONJ) {
			for (auto i = subformula.literals.cbegin(); i < subformula.literals.cend(); ++i) {
				grounding()->addUnitClause(*i);
			}
		} else {
			grounding()->add(subformula.literals);
		}
	} else {
		if (subformula.type == formula.type) {
			formula.literals.insert(formula.literals.begin(), subformula.literals.cbegin(), subformula.literals.cend());
		} else {
			formula.literals.push_back(getReification(subformula));
		}
	}
	return FormStat::UNKNOWN;
}

// NOTE: Optimized to avoid looping over the formula after construction
void BoolGrounder::run(ConjOrDisj& formula, bool negate) const {
	if (verbosity() > 2)
		printorig();
	formula.type = conn_;
	for (auto g = _subgrounders.cbegin(); g < _subgrounders.cend(); g++) {
		if (runSubGrounder(*g, context()._conjunctivePathFromRoot, formula, negate) == FormStat::DECIDED) {
			return;
		}
	}
}

// TODO do toplevel checks in all grounders

void QuantGrounder::run(ConjOrDisj& formula, bool negated) const {
	if (verbosity() > 2)
		printorig();

	formula.type = conn_;

	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
		if (_checker->check()) {
			formula.literals = litlist { context().gentype == GenType::CANMAKETRUE ? _false : _true };
			return;
		}

		// Allows to jump out when grounding infinitely
		// TODO should be a faster check?
		// TODO add on other places
		if(GlobalData::instance()->terminateRequested()){
			throw IdpException("Terminate requested");
		}

		if (runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, formula, negated) == FormStat::DECIDED) {
			return;
		}
	}
}

void EquivGrounder::run(ConjOrDisj& formula, bool negated) const {
	Assert(not negated);
	if (verbosity() > 2)
		printorig();

	// Run subgrounders
	ConjOrDisj leftformula, rightformula;
	runSubGrounder(_leftgrounder, false, leftformula, false);
	runSubGrounder(_rightgrounder, false, rightformula, false);
	Assert(leftformula.literals.size()==1);
	Assert(rightformula.literals.size()==1);
	Lit left = leftformula.literals[0];
	Lit right = rightformula.literals[0];

	if (left == right) {
		formula.literals = litlist { isPositive() ? _true : _false };
	} else if (left == _true) {
		if (right == _false) {
			formula.literals = litlist { isPositive() ? _false : _true };
		} else {
			formula.literals = litlist { isPositive() ? right : -right };
		}
	} else if (left == _false) {
		if (right == _true) {
			formula.literals = litlist { isPositive() ? _false : _true };
		} else {
			formula.literals = litlist { isPositive() ? -right : right };
		}
	} else if (right == _true) { // left is not true or false
		formula.literals = litlist { isPositive() ? left : -left };
	} else if (right == _false) {
		formula.literals = litlist { isPositive() ? -left : left };
	} else {
		// Two ways of encoding A <=> B  => 1: (A and B) or (~A and ~B)
		//									2: (A or ~B) and (~A or B) => already CNF => much better!
		litlist aornotb = { left, isPositive() ? -right : right };
		litlist notaorb = { -left, isPositive() ? right : -right };
		TsType tp = context()._tseitin;
		Lit ts1 = translator()->translate(aornotb, false, tp);
		Lit ts2 = translator()->translate(notaorb, false, tp);
		formula.literals = litlist { ts1, ts2 };
		formula.type = Conn::CONJ;
	}
}
