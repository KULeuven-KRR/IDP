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

#include "FormulaGrounders.hpp"

#include "IncludeComponents.hpp"
#include "TermGrounders.hpp"
#include "SetGrounders.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "generators/InstGenerator.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "utils/ListUtils.hpp"
#include <cmath>
#include <iostream>
#include "errorhandling/error.hpp"
#include "utils/StringUtils.hpp"

using namespace std;

bool recursive(const PFSymbol& symbol, const GroundingContext& context){
	return context._defined.find(const_cast<PFSymbol*>(&symbol))!=context._defined.cend();
}

FormulaGrounder::FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct)
		: 	Grounder(grounding, ct),
			_origform(NULL) {
}

FormulaGrounder::~FormulaGrounder() {
	if (_origform != NULL) {
		_origform->recursiveDelete();
		_origform = NULL;
	}
	if (not _origvarmap.empty()) {
		for (auto i = _origvarmap.begin(); i != _origvarmap.end(); ++i) {
			delete (i->first);
		}
		_origvarmap.clear();
	}
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

#define dtype(container) decltype(*std::begin(container))

void FormulaGrounder::printorig() const {
	if (_origform == NULL) {
		return;
	}
	clog << tabs() << "Grounding formula " << print(_origform) << "\n";
	if (not _origform->freeVars().empty()) {
		pushtab();
		clog << tabs() << "with instance ";
		printList(clog, _origform->freeVars(), ", ",  [&](std::ostream& output, dtype(_origform->freeVars()) v){ output <<print(v) << " = " << print(_origvarmap.find(v)->second->get());});
		clog << "\n";
		poptab();
	}
}

void FormulaGrounder::put(std::ostream& output) const{
	if (_origform == NULL) {
		return;
	}
	output << print(_origform);
	if (not _origform->freeVars().empty()) {
		output << "[";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			output << print(*it) << " = ";
			auto e = _origvarmap.find(*it)->second->get();
			output << print(e) << ',';
		}
		output << "]";
	}
}

AtomGrounder::AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol* s, const vector<TermGrounder*>& sg,
		const vector<const DomElemContainer*>& checkargs, const vector<SortTable*>& vst,
		const GroundingContext& ct)
		: 	FormulaGrounder(grounding, ct),
			_subtermgrounders(sg),
			_symbol(s),
			_symboloffset(translator()->addSymbol(s)),
			_tables(vst),
			_sign(sign),
			_checkargs(checkargs),
			_recursive(ct._defined.find(s) != ct._defined.cend()),
			_args(_subtermgrounders.size()),
			terms(_subtermgrounders.size(), GroundTerm(NULL)) {
	gentype = ct.gentype;
	setMaxGroundSize(tablesize(TableSizeType::TST_EXACT, 1));
}

AtomGrounder::~AtomGrounder() {
	deleteList(_subtermgrounders);
}

Lit AtomGrounder::run() const {
	notifyGroundedAtom();

	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}
	}

	// Run subterm grounders
	bool alldomelts = true;
	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		auto groundterm = _subtermgrounders[n]->run();
		terms[n] = groundterm;
		if (groundterm.isVariable) {
			alldomelts = false;
		} else {
			auto domelem = groundterm._domelement;
			auto known = translator()->checkApplication(domelem, _tables[n], _subtermgrounders[n]->getDomain(), context()._funccontext, _sign);
			if(known!=TruthValue::Unknown){
				if(verbosity()>2){
				if (_origform != NULL) {
					poptab();
				}
				clog << tabs() << "Result is " <<print(known) << "\n";
				}
				return known==TruthValue::True?_true:_false;
			}
		}
	}

	Lit lit = 0;
	if (not alldomelts) {
		auto temphead = translator()->createNewUninterpretedNumber();
		getGrounding()->addLazyElement(temphead, _symbol, terms, _recursive);
		lit = temphead;
	}else{
		// Run instance checkers
		// NOTE: set all the variables representing the subterms to their current value (these are used in the checkers)
		for (size_t n = 0; n < terms.size(); ++n) {
			_args[n] = terms[n]._domelement;
			*(_checkargs[n]) = _args[n];
		}

		lit = translator()->translateReduced(_symboloffset, _args, _recursive);
	}
	lit = isPos(_sign) ? lit : -lit;

	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result is " << translator()->printLit(lit) << "\n";
	}
	return lit;
}

void AtomGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
	formula.literals.push_back(run());
}

ComparisonGrounder::~ComparisonGrounder() {
	delete (_lefttermgrounder);
	delete (_righttermgrounder);
}

Lit ComparisonGrounder::run() const {
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}
	}
	auto left = _lefttermgrounder->run();
	auto right = _righttermgrounder->run();

	//TODO out-of-bounds check?

	Lit result;
	if (left.isVariable) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if (right.isVariable) {
			CPBound rightbound(right._varid);
			result = translator()->reify(leftterm, _comparator, rightbound, context()._tseitin);
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			result = translator()->reify(leftterm, _comparator, rightbound, context()._tseitin);
		}
	} else {
		Assert(not left.isVariable);
		int leftvalue = left._domelement->value()._int;
		if (right.isVariable) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			result = translator()->reify(rightterm, invertComp(_comparator), leftbound, context()._tseitin);
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			result = compare(leftvalue, _comparator, rightvalue) ? _true : _false;
		}
	}
	if (verbosity() > 2) {
		if (_origform != NULL) {
			poptab();
		}
		clog << tabs() << "Result is " << translator()->printLit(result) << "\n";
	}
	return result;
}

void ComparisonGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
	formula.literals.push_back(run()); // TODO can do better?
}

// TODO incorrect groundsize
AggGrounder::AggGrounder(AbstractGroundTheory* grounding, GroundingContext gc, TermGrounder* bound, CompType comp, AggFunction tp, SetGrounder* sg, SIGN sign)
		: 	FormulaGrounder(grounding, gc),
			_setgrounder(sg),
			_boundgrounder(bound),
			_type(tp),
			_comp(comp),
			_sign(sign) {
	bool noAggComp = comp == CompType::NEQ || comp == CompType::LEQ || comp == CompType::GEQ;
	bool signPosIfStrict = isPos(_sign) == not noAggComp;
	_doublenegtseitin = (gc._tseitin == TsType::RULE)
			&& ((gc._monotone == Context::POSITIVE && signPosIfStrict) || (gc._monotone == Context::NEGATIVE && not signPosIfStrict));
	setMaxGroundSize(tablesize(TableSizeType::TST_EXACT, 1));
}

AggGrounder::~AggGrounder() {
	delete (_setgrounder);
	delete (_boundgrounder);
}

/**
 * Negate the comparator and invert the sign of the tseitin when the aggregate is in a doubly negated context.
 */
//TODO:why?
Lit AggGrounder::handleDoubleNegation(double boundvalue, SetId setnr, CompType comp) const {
	TsType tp = context()._tseitin;
	Lit tseitin = translator()->reify(boundvalue, negateComp(comp), _type, setnr, tp);
	return isPos(_sign) ? -tseitin : tseitin;
}

Lit AggGrounder::finishCard(double truevalue, double boundvalue, SetId setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	auto tsset = translator()->groundset(setnr);
	int maxposscard = tsset.size();
	TsType tp = context()._tseitin;
	bool simplify = false;
	bool conj = false; // Note: Only used if simplify is true
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
		tp = invertImplication(tp);
	}
	if (simplify) {
		if (_doublenegtseitin) {
			if (negateset) {
				Lit tseitin = translator()->reify(tsset.literals(), !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			} else {
				litlist newsetlits(tsset.size());
				for (size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				Lit tseitin = translator()->reify(newsetlits, !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			}
		} else {
			if (negateset) {
				litlist newsetlits(tsset.size());
				for (size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				Lit tseitin = translator()->reify(newsetlits, conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			} else {
				Lit tseitin = translator()->reify(tsset.literals(), conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			}
		}
	} else {
		if (_doublenegtseitin)
			return handleDoubleNegation(double(leftvalue), setnr, _comp);
		else {
			Lit tseitin = translator()->reify(double(leftvalue), _comp, AggFunction::CARD, setnr, tp);
			return isPos(_sign) ? tseitin : -tseitin;
		}
	}
}

/**
 * This method is only made because the solver cannot handle products with sets containing zeros or negative values.
 * If the solver improves, this should be deleted.
 *
 * TODO Can be optimized more (for special cases like in the "finish"-method, but won't be called often anyway.
 */
Lit AggGrounder::splitproducts(double /*boundvalue*/, double newboundvalue, double /*minpossvalue*/, double /*maxpossvalue*/, SetId setnr) const {
	Assert(_type==AggFunction::PROD);
	auto tsset = translator()->groundset(setnr);
	litlist zerolits;
	weightlist zeroweights;

	//All literals (made positive) (including the negative)
	//IMPORTANT: This is NOT only the positives!!!!!!!!!!!
	litlist poslits;
	weightlist posweights;

	//Only the negatives! with weights 1 (for cardinalities)
	litlist neglits;
	weightlist negweights;

	for (size_t i = 0; i < tsset.literals().size(); ++i) {
		if (tsset.weight(i) < 0) {
			poslits.push_back(tsset.literal(i));
			posweights.push_back(-tsset.weight(i));
			neglits.push_back(tsset.literal(i));
			negweights.push_back(1);
		} else if (tsset.weight(i) > 0) {
			poslits.push_back(tsset.literal(i));
			posweights.push_back(tsset.weight(i));
		} else {
			zerolits.push_back(tsset.literal(i));
			zeroweights.push_back(0);
		}
	}

	auto possetnumber = translator()->translateSet(poslits, posweights, tsset.trueweights(), { });
	auto negsetnumber = translator()->translateSet(neglits, negweights, { }, { });

	auto tp = context()._tseitin;
	if (isNeg(_sign)) {
		tp = invertImplication(tp);
	}
	Lit tseitin;
	if (newboundvalue == 0) {
		tseitin = translator()->reify(zerolits, false, tp);
	} else {
		Lit nozeros = -translator()->reify(zerolits, false, tp);
		Lit prodright = translator()->reify(abs(newboundvalue), _comp, AggFunction::PROD, possetnumber, tp);
		litlist possiblecards;
		if (newboundvalue > 0) {
			Lit nonegatives = -translator()->reify(neglits, false, tp); //solver cannot handle empty sets.
			possiblecards.push_back(nonegatives);
			for (size_t i = 2; i <= neglits.size(); i = i + 2) {
				Lit cardisi = translator()->reify(i, CompType::EQ, AggFunction::CARD, negsetnumber, tp);
				possiblecards.push_back(cardisi);
			}
		} else {
			for (size_t i = 1; i <= neglits.size(); i = i + 2) {
				Lit cardisi = translator()->reify(i, CompType::EQ, AggFunction::CARD, negsetnumber, tp);
				possiblecards.push_back(cardisi);
			}
		}
		Lit cardright = translator()->reify(possiblecards, false, tp);
		tseitin = translator()->reify( { nozeros, prodright, cardright }, true, tp);
	}
	return isPos(_sign) ? tseitin : -tseitin;

}

/**
 * General finish method for grounding of sum, product, minimum and maximum aggregates.
 * Checks whether the aggregate will be certainly true or false, based on minimum and maximum possible values and the given bound;
 * and creates a tseitin, handling double negation when necessary;
 */
Lit AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, SetId setnr) const {
	// Check minimum and maximum possible values against the given bound
	auto newcomp = _comp;
	switch (newcomp) {
	case CompType::EQ:
		if (not isInt(newboundvalue)) { // If bound is not an integer, can never be equal
			return _false;
		} // Otherwise fall through
	case CompType::NEQ:
		if (not isInt(newboundvalue)) { // If bound is not an integer, can never be equal
			return _true;
		}
		if (minpossvalue > boundvalue || maxpossvalue < boundvalue) {
			return (isPos(_sign) == (newcomp == CompType::EQ)) ? _false : _true;
		}
		break;
	case CompType::GEQ:
	case CompType::GT:
		if (not isInt(newboundvalue)) { // If not integer, take floor and always make it >
			newboundvalue = floor(newboundvalue);
			newcomp = CompType::GT;
		}
		if (compare(boundvalue, newcomp, maxpossvalue)) {
			return isPos(_sign) ? _true : _false;
		}
		if (compare(boundvalue, negateComp(newcomp), minpossvalue)) {
			return isPos(_sign) ? _false : _true;
		}
		break;
	case CompType::LEQ:
	case CompType::LT:
		if (not isInt(newboundvalue)) { // If not integer, take ceil and always make it <
			newboundvalue = ceil(newboundvalue);
			newcomp = CompType::LT;
		}
		if (compare(boundvalue, newcomp, minpossvalue)) {
			return isPos(_sign) ? _true : _false;
		}
		if (compare(boundvalue, negateComp(newcomp), maxpossvalue)) {
			return isPos(_sign) ? _false : _true;
		}
		break;

	}
	if (_doublenegtseitin) {
		return handleDoubleNegation(newboundvalue, setnr, newcomp);
	} else {
		Lit tseitin;
		TsType tp = context()._tseitin;
		if (isNeg(_sign)) {
			tp = invertImplication(tp);
		}
		tseitin = translator()->reify(newboundvalue, newcomp, _type, setnr, tp);
		return isPos(_sign) ? tseitin : -tseitin;
	}
}

// TODO aggrounder estimate of fullgrounding is incorrect!
Lit AggGrounder::run() const {
	// Run subgrounders
	auto setnr = _setgrounder->run();
	auto groundbound = _boundgrounder->run();
	Assert(not groundbound.isVariable);

	auto bound = groundbound._domelement;

	// Retrieve the set, note that weights might be changed when handling min and max aggregates.
	auto tsset = translator()->groundset(setnr);

	// Retrieve the value of the bound
	Weight boundvalue = bound->type() == DET_INT ? (double) bound->value()._int : bound->value()._double;

	// Compute the value of the aggregate based on weights of literals that are certainly true.
	Weight truevalue = applyAgg(_type, tsset.trueweights());

	// When the set is empty (no more unknown values), return an answer based on the current value of the aggregate.
	if (tsset.empty()) {
		bool returnvalue = compare(boundvalue, _comp, truevalue);
		return isPos(_sign) == returnvalue ? _true : _false;
	}

	// Handle specific aggregates.
	Lit tseitin;
	double minpossvalue = truevalue;
	double maxpossvalue = truevalue;
	switch (_type) {
	case AggFunction::CARD:
		tseitin = finishCard(truevalue, boundvalue, setnr);
		break;
	case AggFunction::SUM:
		// Compute the minimum and maximum possible value of the sum.
		for (size_t n = 0; n < tsset.size(); ++n) {
			if (tsset.weight(n) > 0) {
				maxpossvalue += tsset.weight(n);
			} else if (tsset.weight(n) < 0) {
				minpossvalue += tsset.weight(n);
			}
		}
		// Finish
		tseitin = finish(boundvalue, (boundvalue - truevalue), minpossvalue, maxpossvalue, setnr);
		break;
	case AggFunction::PROD: {
		// Compute the minimum and maximum possible value of the product.
		bool containsneg = false;
		bool containszero = false;
		for (size_t n = 0; n < tsset.size(); ++n) {
			if (abs(tsset.weight(n)) > 1) {
				maxpossvalue *= abs(tsset.weight(n));
			} else if (tsset.weight(n) != 0) {
				minpossvalue *= abs(tsset.weight(n));
			}
			if (tsset.weight(n) == 0) {
				containszero = true;
			}
			if (tsset.weight(n) < 0) {
				containsneg = true;
			}
		}
		if (truevalue == 0) {
			return boundvalue == 0 ? _true : _false;
		}
		if (containsneg) {
			minpossvalue = (-maxpossvalue < minpossvalue ? -maxpossvalue : minpossvalue);
		}
		if (containsneg || containszero) {
			Assert(truevalue != 0);
			//division is safe (see higher check for truevalue ==0)
			tseitin = splitproducts(boundvalue, (boundvalue / truevalue), minpossvalue, maxpossvalue, setnr);
		} else {
			// Finish
			tseitin = finish(boundvalue, (boundvalue / truevalue), minpossvalue, maxpossvalue, setnr);
		}
		break;
	}
	case AggFunction::MIN:
		// Compute the minimum possible value of the set.
		for (size_t n = 0; n < tsset.size();) {
			minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
			// TODO: what if some set is used in multiple expressions? Then we are changing the set???
			if (tsset.weight(n) >= truevalue) {
				tsset.removeLit(n);
			} else {
				++n;
			}
		}
		//INVAR: we know that the real value of the aggregate is at most truevalue.
		if (boundvalue > truevalue) {
			if (_comp == CompType::EQ || _comp == CompType::LEQ || _comp == CompType::LT) {
				return isPos(_sign) ? _false : _true;
			} else {
				return isPos(_sign) ? _true : _false;
			}
		} else if (boundvalue == truevalue) {
			switch (_comp) {
			case CompType::EQ:
			case CompType::LEQ:
				tseitin = -translator()->reify(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
				break;
			case CompType::NEQ:
			case CompType::GT:
				tseitin = translator()->reify(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
				break;
			case CompType::GEQ:
				return isPos(_sign) ? _true : _false;
				break;
			case CompType::LT:
				return isPos(_sign) ? _false : _true;
				break;
			}
		} else { //boundvalue < truevalue
			// Finish
			tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		}
		break;
	case AggFunction::MAX:
		// Compute the maximum possible value of the set.
		for (size_t n = 0; n < tsset.size();) {
			maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
			if (tsset.weight(n) <= truevalue) {
				tsset.removeLit(n);
			} else {
				++n;
			}
		}
		//INVAR: we know that the real value of the aggregate is at least truevalue.
		if (boundvalue < truevalue) {
			if (_comp == CompType::NEQ || _comp == CompType::LEQ || _comp == CompType::LT) {
				return isPos(_sign) ? _true : _false;
			} else {
				return isPos(_sign) ? _false : _true;
			}
		} else if (boundvalue == truevalue) {
			switch (_comp) {
			case CompType::EQ:
			case CompType::GEQ:
				tseitin = -translator()->reify(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
				break;
			case CompType::NEQ:
			case CompType::LT:
				tseitin = translator()->reify(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
				break;
			case CompType::LEQ:
				return isPos(_sign) ? _true : _false;
				break;
			case CompType::GT:
				return isPos(_sign) ? _false : _true;
				break;
			}
		} else { //boundvalue > truevalue
			// Finish
			tseitin = finish(boundvalue, boundvalue, minpossvalue, maxpossvalue, setnr);
		}
		break;
	}
	return tseitin;
}

void AggGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
	formula.literals.push_back(run()); // TODO can do better?
}

bool ClauseGrounder::isRedundantInFormula(Lit l) const {
	return connective() == Conn::CONJ ? l == _true : l == _false;
}

Lit ClauseGrounder::redundantLiteral() const {
	return connective() == Conn::CONJ ? _true : _false;
}

/**
 * conjunction: never true by a literal
 * disjunction: true if literal is true
 * negated conjunction: true if literal is false
 * negated disjunction: never true by a literal
 */
bool ClauseGrounder::makesFormulaTrue(Lit l) const {
	return (connective() == Conn::DISJ && l == _true);
}

/**
 * conjunction: false if a literal is false
 * disjunction: never false by a literal
 * negated conjunction: never false by a literal
 * negated disjunction: false if a literal is true
 */
bool ClauseGrounder::makesFormulaFalse(Lit l) const {
	return (connective() == Conn::CONJ && l == _false);
}

Lit ClauseGrounder::getEmtyFormulaValue() const {
	return connective() == Conn::CONJ ? _true : _false;
}

bool ClauseGrounder::decidesFormula(Lit lit) const {
	return connective() == Conn::CONJ ? lit == _false : lit == _true;
}

TsType ClauseGrounder::getTseitinType() const {
	return isNegative()?invertImplication(context()._tseitin):context()._tseitin;
}

Lit ClauseGrounder::getReification(const ConjOrDisj& formula, TsType tseitintype) const {
	return getOneLiteralRepresenting(formula, tseitintype);
}
Lit ClauseGrounder::getEquivalentReification(const ConjOrDisj& formula, TsType tseitintype) const {
	return getOneLiteralRepresenting(formula, tseitintype == TsType::RULE ? TsType::RULE : TsType::EQ);
}

Lit ClauseGrounder::getOneLiteralRepresenting(const ConjOrDisj& formula, TsType type) const {
	if (formula.literals.empty()) {
		return getEmtyFormulaValue();
	}

	if (formula.literals.size() == 1) {
		return formula.literals[0];
	}

	return createTseitin(formula, type);
}

// Takes context into account!
Lit ClauseGrounder::createTseitin(const ConjOrDisj& formula, TsType type) const {
	Lit tseitin;
	bool asConjunction = formula.getType() == Conn::CONJ;
	if (negativeDefinedContext()) {
		asConjunction = not asConjunction;
	}
	tseitin = translator()->reify(formula.literals, asConjunction, type);
	return tseitin;
}

void ClauseGrounder::run(ConjOrDisj& formula) const {
	internalRun(formula);
	if (isNegative()) {
		formula.literals = {-getReification(formula, getTseitinType())};
	}
}

FormStat ClauseGrounder::runSubGrounder(Grounder* subgrounder, bool conjFromRoot, bool considerAsConjunctiveWithSign, ConjOrDisj& formula) const {
	auto origvalue = subgrounder->context()._conjunctivePathFromRoot;
	if(conjFromRoot && considerAsConjunctiveWithSign){
		subgrounder->setConjUntilRoot(true);
	}
	Assert(formula.getType()==connective());
	_subformula.literals.clear();
	auto& lits = _subformula.literals;
	subgrounder->wrapRun(_subformula);
	if (lits.size() == 0) {
		lits.push_back(_subformula.getType() == Conn::CONJ ? _true : _false);
	}

	auto result = FormStat::UNKNOWN;
	if (lits.size() == 1) {
		Lit l = lits[0];
		if (makesFormulaFalse(l)) {
			formula.literals = litlist { _false };
			result = FormStat::DECIDED;
		} else if (makesFormulaTrue(l)) {
			formula.literals = litlist { _true };
			result = FormStat::DECIDED;
		} else if (not isRedundantInFormula(l)) {
			formula.literals.push_back(l);
		}
	} else {
		if (conjFromRoot && considerAsConjunctiveWithSign) {
			if (_subformula.getType() == Conn::CONJ) {
				for (auto i = lits.cbegin(); i < lits.cend(); ++i) {
					getGrounding()->addUnitClause(*i);
				}
			} else {
				getGrounding()->add(lits);
			}
			Lit l = _true;
			if (makesFormulaFalse(l)) {
				formula.literals = litlist { _false };
				result = FormStat::DECIDED;
			} else if (makesFormulaTrue(l)) {
				formula.literals = litlist { _true };
				result = FormStat::DECIDED;
			} else if (not isRedundantInFormula(l)) {
				formula.literals.push_back(l);
			}
		} else {
			if (_subformula.getType() == formula.getType()) {
				insertAtEnd(formula.literals, lits);
			} else {
				formula.literals.push_back(
						getReification(_subformula,
								subgrounder->context()._tseitin));
			}
		}
	}

	subgrounder->setConjUntilRoot(origvalue);

	return result;
}

BoolGrounder::BoolGrounder(AbstractGroundTheory* grounding, const std::vector<Grounder*>& sub, SIGN sign, bool conj, const GroundingContext& ct)
		: 	ClauseGrounder(grounding, sign, conj, ct),
			_subgrounders(sub) {
	tablesize size = tablesize(TableSizeType::TST_EXACT, 0);
	for (auto i = sub.cbegin(); i < sub.cend(); ++i) {
		size = size + (*i)->getMaxGroundSize();
	}
	setMaxGroundSize(size); // TODO move
}

BoolGrounder::~BoolGrounder() {
	deleteList(_subgrounders);
}

// NOTE: Optimized to avoid looping over the formula after construction
void BoolGrounder::internalRun(ConjOrDisj& formula) const {
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}
	}
	formula.setType(connective());
	for (auto grounder : _subgrounders) {
		CHECKTERMINATION;
		auto considerAsConjunctiveWithSign = conjunctiveWithSign();
		if(grounder==_subgrounders.back() && formula.literals.size()==0 && isPositive()){
			considerAsConjunctiveWithSign = true;
		}
		if (runSubGrounder(grounder, context()._conjunctivePathFromRoot, considerAsConjunctiveWithSign, formula) == FormStat::DECIDED) {
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
			}
			return;
		}
		if(context()._conjunctivePathFromRoot && conjunctiveWithSign()){
			for(auto lit: formula.literals){
				getGrounding()->add(GroundClause{lit});
			}
			formula.literals.clear();
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}

void BoolGrounder::put(std::ostream& output) const {
	if (_origform != NULL) {
		FormulaGrounder::put(output);
	} else {
		printList(output, getSubGrounders(), connective() == Conn::CONJ ? " & " : " | ");
	}
}

QuantGrounder::QuantGrounder(AbstractGroundTheory* grounding, FormulaGrounder* sub, SIGN sign, QUANT quant, InstGenerator* gen, InstChecker* checker,
		const GroundingContext& ct)
		: 	ClauseGrounder(grounding, sign, quant == QUANT::UNIV, ct),
			_subgrounder(sub),
			_generator(gen),
			_checker(checker) {
}

QuantGrounder::~QuantGrounder() {
	delete (_subgrounder);
	delete (_generator);
	delete (_checker);
}

void QuantGrounder::internalRun(ConjOrDisj& formula) const {
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}
	}
	formula.setType(connective());
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
		CHECKTERMINATION;
		if (_checker->check()) {
			formula.literals = litlist {context().gentype == GenType::CANMAKETRUE ? _true : _false};
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
				clog << tabs() << "Checker checked, hence formula decided. Result is " << translator()->printLit(formula.literals.front()) << "\n";
			}
			return;
		}
		if (runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, conjunctiveWithSign(), formula) == FormStat::DECIDED) {
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
			}
			return;
		}
		if(context()._conjunctivePathFromRoot && conjunctiveWithSign()){
			for(auto lit: formula.literals){
				getGrounding()->add(GroundClause{lit});
			}
			formula.literals.clear();
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}

EquivGrounder::EquivGrounder(AbstractGroundTheory* grounding, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct)
		: 	ClauseGrounder(grounding, sign, true, ct),
			_leftgrounder(lg),
			_rightgrounder(rg) {
	auto lsize = lg->getMaxGroundSize();
	auto rsize = rg->getMaxGroundSize();
	setMaxGroundSize(lsize + rsize);
}

EquivGrounder::~EquivGrounder() {
	delete (_leftgrounder);
	delete (_rightgrounder);
}

void EquivGrounder::internalRun(ConjOrDisj& formula) const {
	//Assert(not negated); I think this is not needed.
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}

		clog << tabs() << "Current formula: " << (isNegative() ? "~" : "");
		_leftgrounder->printorig();
		clog << "\n";
		clog << tabs() << " <=> ";
		_rightgrounder->printorig();
		clog << "\n";
	}

	// Run subgrounders
	ConjOrDisj leftformula, rightformula;
	leftformula.setType(connective());
	rightformula.setType(connective());
	Assert(_leftgrounder->context()._monotone==Context::BOTH);
	Assert(_rightgrounder->context()._monotone==Context::BOTH);
	Assert(_leftgrounder->context()._tseitin==TsType::EQ|| _leftgrounder->context()._tseitin==TsType::RULE);
	Assert(_rightgrounder->context()._tseitin==TsType::EQ|| _rightgrounder->context()._tseitin==TsType::RULE);
	runSubGrounder(_leftgrounder, false, conjunctiveWithSign(), leftformula);
	runSubGrounder(_rightgrounder, false, conjunctiveWithSign(), rightformula);
	auto left = getEquivalentReification(leftformula, getTseitinType());
	auto right = getEquivalentReification(rightformula, getTseitinType());

	formula.setType(Conn::CONJ); // Does not matter for all those 1-literal lists
	if (left == right) {
		formula.literals = litlist { _true };
	} else if (left == _true) {
		if (right == _false) {
			formula.literals = litlist { _false };
		} else {
			formula.literals = litlist { right };
		}
	} else if (left == _false) {
		if (right == _true) {
			formula.literals = litlist { _false };
		} else {
			formula.literals = litlist { -right };
		}
	} else if (right == _true) { // left is not true or false
		formula.literals = litlist { left };
	} else if (right == _false) {
		formula.literals = litlist { -left };
	} else {
		// Two ways of encoding A <=> B  => 1: (A and B) or (~A and ~B)
		//									2: (A or ~B) and (~A or B) => already CNF => much better!
		litlist aornotb = { left, -right };
		litlist notaorb = { -left, right };
		if (context()._conjunctivePathFromRoot) {
			if (isPositive()) {
				getGrounding()->add(aornotb);
				getGrounding()->add(notaorb);
				formula.literals = litlist { _true };
			} else {
				getGrounding()->add( { left, right });
				getGrounding()->add( { -left, -right });
				formula.literals = litlist { _false };
			}
		} else {
			auto ts1 = translator()->reify(aornotb, false, context()._tseitin);
			auto ts2 = translator()->reify(notaorb, false, context()._tseitin);
			formula.literals = litlist { ts1, ts2 };
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}
