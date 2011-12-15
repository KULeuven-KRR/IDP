/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
#include <cmath>

using namespace std;

int verbosity() {
	return getOption(IntType::GROUNDVERBOSITY);
}

//TODO: a lot of the "int" returns here should be "Lit": Issue 57199

FormulaGrounder::FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct) :
		Grounder(grounding, ct), _origform(NULL) {
}

GroundTranslator* FormulaGrounder::translator() const {
	return getGrounding()->translator();
}

void FormulaGrounder::setOrig(const Formula* f, const map<Variable*, const DomElemContainer*>& mvd) {
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
	if (_origform == NULL) {
		return;
	}
	clog << "Grounding formula " << toString(_origform);
	if (not _origform->freeVars().empty()) {
		clog << " with instance ";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << toString(e) << ' ';
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
				throw notyetimplemented("Partial function issue in grounding an atom.");
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
			clog << "Result is " << translator()->printLit(gentype == GenType::CANMAKETRUE ? _true : _false) << "\n";
		}
		return gentype == GenType::CANMAKETRUE ? _true : _false;
	}
	if (_inter->isTrue(args)) {
		if (verbosity() > 2) {
			clog << "Result is " << (isPos(_sign) ? "true" : "false") << "\n";
		}
		return isPos(_sign) ? _true : _false;
	}
	if (_inter->isFalse(args)) {
		if (verbosity() > 2) {
			clog << "Result is " << (isPos(_sign) ? "false" : "true") << "\n";
		}
		return isPos(_sign) ? _false : _true;
	}

	// Return grounding
	Lit lit = translator()->translate(_symbol, args);
	if (isNeg(_sign)) {
		lit = -lit;
	}
	if (verbosity() > 2) {
		clog << "Result is " << translator()->printLit(lit) << "\n";
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

	Lit result;
	if (left.isVariable) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if (right.isVariable) {
			CPBound rightbound(right._varid);
			result = translator()->translate(leftterm, _comparator, rightbound, TsType::EQ); //TODO use _context._tseitin?
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			result = translator()->translate(leftterm, _comparator, rightbound, TsType::EQ); //TODO use _context._tseitin?
		}
	} else {
		Assert(not left.isVariable);
		int leftvalue = left._domelement->value()._int;
		if (right.isVariable) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			result = translator()->translate(rightterm, invertComp(_comparator), leftbound, TsType::EQ); //TODO use _context._tseitin?
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			switch (_comparator) {
			case CompType::EQ:
				result = leftvalue == rightvalue ? _true : _false; break;
			case CompType::NEQ:
				result = leftvalue != rightvalue ? _true : _false; break;
			case CompType::LEQ:
				result = leftvalue <= rightvalue ? _true : _false; break;
			case CompType::GEQ:
				result = leftvalue >= rightvalue ? _true : _false; break;
			case CompType::LT:
				result = leftvalue < rightvalue ? _true : _false; break;
			case CompType::GT:
				result = leftvalue > rightvalue ? _true : _false; break;
			}
		}
	}
	return result;
}

void ComparisonGrounder::run(ConjOrDisj& formula) const {
	formula.literals.push_back(run()); // TODO can do better?
}

/**
 * Negate the comparator and invert the sign of the tseitin when the aggregate is in a doubly negated context.
 */
//TODO:why?
Lit AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	TsType tp = context()._tseitin;
	int tseitin = translator()->translate(boundvalue, negateComp(_comp), _type, setnr, tp);
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
	case CompType::EQ: // x = #{..}
	case CompType::NEQ: // x ~= #{..}
		if (leftvalue < 0 || leftvalue > maxposscard) {
			return ((_comp == CompType::EQ) == isPos(_sign)) ? _false : _true;
		} else if (leftvalue == 0) { //0 = #{..} ---> !x: ~... OF 0 ~= #{..} ---> ?x:..
			simplify = true;
			conj = _comp == CompType::EQ;
			negateset = _comp == CompType::EQ;
		} else if (leftvalue == maxposscard) { //= = #{..} ---> !x: ... OF m ~= #{..} ---> ?x:~..
			simplify = true;
			conj = _comp == CompType::EQ;
			negateset = _comp != CompType::EQ;
		}
		break;
	case CompType::LT: // x < #{..}
	case CompType::GEQ: // ~(x < #{..})
		if (leftvalue < 0) {
			return ((_comp == CompType::LT) == isPos(_sign)) ? _true : _false;
		} else if (leftvalue == 0) {
			simplify = true;
			conj = _comp != CompType::LT;
			negateset = _comp != CompType::LT;
		} else if (leftvalue == maxposscard - 1) {
			simplify = true;
			conj = _comp == CompType::LT;
			negateset = _comp != CompType::LT;
		} else if (leftvalue >= maxposscard) {
			return ((_comp == CompType::LT) == isPos(_sign)) ? _false : _true;
		}
		break;
	case CompType::GT: // x < #{..}
	case CompType::LEQ: // ~(x < #{..})
		if (leftvalue <= 0) {
			return ((_comp == CompType::GT) == isPos(_sign)) ? _false : _true;
		} else if (leftvalue == 1) {
			simplify = true;
			conj = (_comp == CompType::GT);
			negateset = (_comp == CompType::GT);
		} else if (leftvalue == maxposscard) {
			simplify = true;
			conj = (_comp != CompType::GT);
			negateset = (_comp == CompType::GT);
		} else if (leftvalue > maxposscard) {
			return ((_comp == CompType::GT) == isPos(_sign)) ? _true : _false;
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
			int tseitin = translator()->translate(double(leftvalue), _comp, AggFunction::CARD, setnr, tp);
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
	switch (_comp) {
	case CompType::EQ:
	case CompType::NEQ:
		if (minpossvalue > boundvalue || maxpossvalue < boundvalue) {
			return (isPos(_sign) == (_comp == CompType::EQ)) ? _false : _true;
		}
		break;
	case CompType::GEQ:
	case CompType::GT:
		if (compare(boundvalue, _comp, maxpossvalue)) {
			return isPos(_sign) ? _true : _false;
		}
		if (compare(boundvalue, negateComp(_comp), minpossvalue)) {
			return isPos(_sign) ? _false : _true;
		}
		break;
	case CompType::LEQ:
	case CompType::LT:
		if (compare(boundvalue, _comp, minpossvalue)) {
			return isPos(_sign) ? _true : _false;
		}
		if (compare(boundvalue, negateComp(_comp), maxpossvalue)) {
			return isPos(_sign) ? _false : _true;
		}
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
		tseitin = translator()->translate(newboundvalue, _comp, _type, setnr, tp);
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

	// When the set is empty (no more unknown values), return an answer based on the current value of the aggregate.
	if (tsset.empty()) {
		bool returnvalue = compare(boundvalue, _comp, truevalue);
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
			} else if (tsset.weight(n) != 0) {
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
		for (unsigned int n = 0; n < tsset.size();) {
			minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
			// TODO: what if some set is used in multiple expressions? Then we are changing the set???
			if (tsset.weight(n) >= truevalue) {
				tsset.removeLit(n);
			} else {
				++n;
			}
		}
		//INVAR: we know that the real value of the aggregate is at most truevalue.
		if(boundvalue > truevalue){
			if(_comp == CompType::EQ || _comp == CompType::LEQ || _comp==CompType::LT){
				return isPos(_sign) ? _false : _true;
			}
			else{
				return isPos(_sign) ? _true : _false;
			}
		}
		else if(boundvalue == truevalue){
			if(_comp == CompType::EQ||_comp==CompType::LEQ){
				tseitin = -translator()->translate(tsset.literals(),false,TsType::EQ);
				tseitin = isPos(_sign)? tseitin : - tseitin;
			}
			else if(_comp == CompType::NEQ|| _comp == CompType::GT){
				tseitin = translator()->translate(tsset.literals(),false,TsType::EQ);
				tseitin = isPos(_sign)? tseitin : - tseitin;
			}
			else if(_comp == CompType::GEQ){
				return isPos(_sign) ? _true : _false;
			}
			else if(_comp == CompType::LT){
				return isPos(_sign) ?  _false: _true;
			}
		}
		else{ //boundvalue < truevalue
			// Finish
			tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		}
		break;
	case AggFunction::MAX:
		// Compute the maximum possible value of the set.
		for (unsigned int n = 0; n < tsset.size();) {
			maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
			if (tsset.weight(n) <= truevalue) {
				tsset.removeLit(n);
			} else {
				++n;
			}
		}
		//INVAR: we know that the real value of the aggregate is at least truevalue.
		if(boundvalue < truevalue){
			if(_comp == CompType::NEQ || _comp == CompType::LEQ || _comp==CompType::LT){
				return isPos(_sign) ?  _true: _false;
			}
			else{
				return isPos(_sign) ?  _false: _true;
			}
		}
		else if(boundvalue == truevalue){
			if(_comp == CompType::EQ||_comp==CompType::GEQ){
				tseitin = - translator()->translate(tsset.literals(),false,TsType::EQ);
				tseitin = isPos(_sign)? tseitin : - tseitin;
			}
			else if(_comp == CompType::NEQ|| _comp == CompType::LT){
				tseitin = translator()->translate(tsset.literals(),false,TsType::EQ);
				tseitin = isPos(_sign)? tseitin : - tseitin;
			}
			else if(_comp == CompType::LEQ){
				return isPos(_sign) ? _true : _false;
			}
			else if(_comp == CompType::GT){
				return isPos(_sign) ?  _false: _true;
			}
		}
		else{ //boundvalue > truevalue
			// Finish
			tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		}
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
	Assert(formula.type==conn_);
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
				getGrounding()->addUnitClause(*i);
			}
		} else {
			getGrounding()->add(subformula.literals);
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
		CHECKTERMINATION
		if (runSubGrounder(*g, context()._conjunctivePathFromRoot, formula, negate) == FormStat::DECIDED) {
			return;
		}
	}
}

void QuantGrounder::run(ConjOrDisj& formula, bool negated) const {
	if (verbosity() > 2)
		printorig();

	formula.type = conn_;

	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
		CHECKTERMINATION
		if (_checker->check()) {
			formula.literals = litlist { context().gentype == GenType::CANMAKETRUE ? _false : _true };
			return;
		}

		if (runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, formula, negated) == FormStat::DECIDED) {
			return;
		}
	}
}

Lit EquivGrounder::getLitEquivWith(const ConjOrDisj& form) const{
	if(form.literals.size()>2){
		return getReification(form);
	}
	if(form.literals.size()==0){
		if(form.type==Conn::CONJ){
			return _true;
		}else{
			return _false;
		}
	}else{
		return form.literals[0];
	}
}

void EquivGrounder::run(ConjOrDisj& formula, bool negated) const {
	Assert(not negated);
	if (verbosity() > 2){
		printorig();

		clog <<"Current formula: " <<(negated?"~":"");
		_leftgrounder->printorig();
		clog <<" <=> ";
		_rightgrounder->printorig();
	}

	// Run subgrounders
	ConjOrDisj leftformula, rightformula;
	leftformula.type = conn_;
	rightformula.type = conn_;
	runSubGrounder(_leftgrounder, false, leftformula, false);
	runSubGrounder(_rightgrounder, false, rightformula, false);
	auto left = getLitEquivWith(leftformula);
	auto right = getLitEquivWith(rightformula);

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
