/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "FormulaGrounders.hpp"

#include "IncludeComponents.hpp"
#include "TermGrounders.hpp"
#include "SetGrounders.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"
#include "generators/InstGenerator.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "utils/ListUtils.hpp"
#include <cmath>
#include <iostream>

using namespace std;

FormulaGrounder::FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct)
		: Grounder(grounding, ct), _origform(NULL) {
}

FormulaGrounder::~FormulaGrounder() {
	if (_origform != NULL) {
		_origform->recursiveDelete();
		_origform = NULL;
	}
	if (not _origvarmap.empty()) {
		for (auto i = _origvarmap.begin(); i != _origvarmap.end(); ++i) {
			delete (i->first);
			//delete (i->second);
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

void FormulaGrounder::printorig() const {
	if (_origform == NULL) {
		return;
	}
	clog << tabs() << "Grounding formula " << toString(_origform) << "\n";
	if (not _origform->freeVars().empty()) {
		pushtab();
		clog << tabs() << "with instance ";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << toString(e) << ' ';
		}
		clog << "\n";
		poptab();
	}
}

std::string FormulaGrounder::printFormula() const {
	if (_origform == NULL) {
		return "";
	}
	stringstream ss;
	ss << toString(_origform);
	if (not _origform->freeVars().empty()) {
		ss << "[";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			ss << toString(*it) << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			ss << toString(e) << ',';
		}
		ss << "]";
	}
	return ss.str();
}

AtomGrounder::AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol* s, const vector<TermGrounder*>& sg,
		const vector<const DomElemContainer*>& checkargs, InstChecker* ptchecker, InstChecker* ctchecker, PredInter* inter, const vector<SortTable*>& vst,
		const GroundingContext& ct)
		: FormulaGrounder(grounding, ct), _subtermgrounders(sg), _ptchecker(ptchecker), _ctchecker(ctchecker), _symbol(translator()->addSymbol(s)),
			_tables(vst), _sign(sign), _checkargs(checkargs), _inter(inter), args(_subtermgrounders.size()) {
	gentype = ct.gentype;
	setMaxGroundSize(tablesize(TableSizeType::TST_EXACT, 1));
}

AtomGrounder::~AtomGrounder() {
	deleteList(_subtermgrounders);
	delete (_ptchecker);
	delete (_ctchecker);
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
		if (groundterm.isVariable) {
			alldomelts = false;
		} else {
			args[n] = groundterm._domelement;
			// Check partial functions
			if (args[n] == NULL) {
				//throw notyetimplemented("Partial function issue in grounding an atom.");
				// FIXME what should happen here?
				/*//TODO: produce a warning!
				 if(context()._funccontext == Context::BOTH) {
				 // TODO: produce an error
				 }*/
				if (verbosity() > 2) {
					clog << tabs() << "Partial function went out of bounds\n";
					clog << tabs() << "Result is " << "false" << "\n";
				}
				return _false;
			}

			// Checking out-of-bounds
			if (not _tables[n]->contains(args[n])) {
				if (verbosity() > 2) {
					clog << tabs() << "Term value out of predicate type" << "\n"; //TODO should be a warning
					if (_origform != NULL) { poptab(); }
					clog << tabs() << "Result is " << (isPos(_sign) ? "false" : "true") << "\n";
				}

				return isPos(_sign) ? _false : _true;
			}
		}
	}

	Assert(alldomelts); // If P(t) and (not isCPSymbol(P)) and isCPSymbol(t) then it should have been rewritten, right?

	// Run instance checkers
	// NOTE: set all the variables representing the subterms to their current value (these are used in the checkers)
	for (size_t n = 0; n < args.size(); ++n) {
		*(_checkargs[n]) = args[n];
	}
	if (_ctchecker->check()) { // Literal is irrelevant in its occurrences
		if (verbosity() > 2) {
			clog << tabs() << "Certainly true checker succeeded" << "\n";
			if (_origform != NULL) { poptab(); }
			//clog << tabs() << "Result is " << translator()->printLit(gentype == GenType::CANMAKETRUE ? _true : _false) << "\n";
			clog << tabs() << "Result is true" << "\n";
		}
		//return gentype == GenType::CANMAKETRUE ? _true : _false;
		return _true;
	}
	if (not _ptchecker->check()) { // Literal decides formula if checker succeeds
		if (verbosity() > 2) {
			clog << tabs() << "Possibly true checker failed" << "\n";
			if (_origform != NULL) { poptab(); }
			clog << tabs() << "Result is false" << "\n";
		}
		return _false;
	}
	if (_inter->isTrue(args)) {
		if (verbosity() > 2) {
			if (_origform != NULL) { poptab(); }
			clog << tabs() << "Result is " << (isPos(_sign) ? "true" : "false") << "\n";
		}
		return isPos(_sign) ? _true : _false;
	}
	if (_inter->isFalse(args)) {
		if (verbosity() > 2) {
			if (_origform != NULL) { poptab(); }
			clog << tabs() << "Result is " << (isPos(_sign) ? "false" : "true") << "\n";
		}
		return isPos(_sign) ? _false : _true;
	}

	// Return grounding
	Lit lit = translator()->translate(_symbol, args);
	if (isNeg(_sign)) {
		lit = -lit;
	}
	if (verbosity() > 2) {
		if (_origform != NULL) { poptab(); }
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
		if (_origform != NULL) { pushtab(); }
	}
	auto left = _lefttermgrounder->run();
	auto right = _righttermgrounder->run();

	//TODO out-of-bounds check?

	Lit result;
	if (left.isVariable) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if (right.isVariable) {
			CPBound rightbound(right._varid);
			result = translator()->translate(leftterm, _comparator, rightbound, context()._tseitin);
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			result = translator()->translate(leftterm, _comparator, rightbound, context()._tseitin);
		}
	} else {
		Assert(not left.isVariable);
		int leftvalue = left._domelement->value()._int;
		if (right.isVariable) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			result = translator()->translate(rightterm, invertComp(_comparator), leftbound, context()._tseitin);
		} else {
			Assert(not right.isVariable);
			int rightvalue = right._domelement->value()._int;
			result = compare(leftvalue,_comparator,rightvalue) ? _true : _false;
		}
	}
	if (verbosity() > 2) {
		if (_origform != NULL) { poptab(); }
		clog << tabs() << "Result is " << translator()->printLit(result) << "\n";
	}
	return result;
}

void ComparisonGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
	formula.literals.push_back(run()); // TODO can do better?
}

// TODO incorrect groundsize
AggGrounder::AggGrounder(AbstractGroundTheory* grounding, GroundingContext gc, AggFunction tp, SetGrounder* sg, TermGrounder* bg, CompType comp, SIGN sign)
		: FormulaGrounder(grounding, gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(comp), _sign(sign) {
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
Lit AggGrounder::handleDoubleNegation(double boundvalue, SetId setnr) const {
	TsType tp = context()._tseitin;
	Lit tseitin = translator()->translate(boundvalue, negateComp(_comp), _type, setnr, tp);
	return isPos(_sign) ? -tseitin : tseitin;
}

Lit AggGrounder::finishCard(double truevalue, double boundvalue, SetId setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	auto tsset = translator()->groundset(setnr);
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
		tp = invertImplication(tp);
	}
	if (simplify) {
		if (_doublenegtseitin) {
			if (negateset) {
				Lit tseitin = translator()->translate(tsset.literals(), !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			} else {
				litlist newsetlits(tsset.size());
				for (size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				Lit tseitin = translator()->translate(newsetlits, !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			}
		} else {
			if (negateset) {
				litlist newsetlits(tsset.size());
				for (size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				Lit tseitin = translator()->translate(newsetlits, conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			} else {
				Lit tseitin = translator()->translate(tsset.literals(), conj, tp);
				return isPos(_sign) ? tseitin : -tseitin;
			}
		}
	} else {
		if (_doublenegtseitin)
			return handleDoubleNegation(double(leftvalue), setnr);
		else {
			Lit tseitin = translator()->translate(double(leftvalue), _comp, AggFunction::CARD, setnr, tp);
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
Lit AggGrounder::splitproducts(double /*boundvalue*/, double newboundvalue, double /*minpossvalue*/, double /*maxpossvalue*/, int setnr) const {
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

	int possetnumber = translator()->translateSet(poslits, posweights, tsset.trueweights(), { });
	int negsetnumber = translator()->translateSet(neglits, negweights, { }, { });

	auto tp = context()._tseitin;
	if (isNeg(_sign)) {
		tp = invertImplication(tp);
	}
	Lit tseitin;
	if (newboundvalue == 0) {
		tseitin = translator()->translate(zerolits, false, tp);
	} else {
		Lit nozeros = -translator()->translate(zerolits, false, tp);
		Lit prodright = translator()->translate(abs(newboundvalue), _comp, AggFunction::PROD, possetnumber, tp);
		litlist possiblecards;
		if (newboundvalue > 0) {
			Lit nonegatives = -translator()->translate(neglits, false, tp); //solver cannot handle empty sets.
			possiblecards.push_back(nonegatives);
			for (size_t i = 2; i <= neglits.size(); i = i + 2) {
				Lit cardisi = translator()->translate(i, CompType::EQ, AggFunction::CARD, negsetnumber, tp);
				possiblecards.push_back(cardisi);
			}
		} else {
			for (size_t i = 1; i <= neglits.size(); i = i + 2) {
				Lit cardisi = translator()->translate(i, CompType::EQ, AggFunction::CARD, negsetnumber, tp);
				possiblecards.push_back(cardisi);
			}
		}
		Lit cardright = translator()->translate(possiblecards, false, tp);
		tseitin = translator()->translate( { nozeros, prodright, cardright }, true, tp);
	}
	return isPos(_sign) ? tseitin : -tseitin;

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
	if (_doublenegtseitin) {
		return handleDoubleNegation(newboundvalue, setnr);
	}
	else {
		Lit tseitin;
		TsType tp = context()._tseitin;
		if (isNeg(_sign)) {
			tp = invertImplication(tp);
		}
		tseitin = translator()->translate(newboundvalue, _comp, _type, setnr, tp);
		return isPos(_sign) ? tseitin : -tseitin;
	}
}

// TODO aggrounder estimate of fullgrounding is incorrect!
Lit AggGrounder::run() const {
	// Run subgrounders
	SetId setnr = _setgrounder->run();
	const GroundTerm& groundbound = _boundgrounder->run();
	Assert(not groundbound.isVariable);

	const DomainElement* bound = groundbound._domelement;

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
			if (_comp == CompType::EQ || _comp == CompType::LEQ) {
				tseitin = -translator()->translate(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
			} else if (_comp == CompType::NEQ || _comp == CompType::GT) {
				tseitin = translator()->translate(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
			} else if (_comp == CompType::GEQ) {
				return isPos(_sign) ? _true : _false;
			} else if (_comp == CompType::LT) {
				return isPos(_sign) ? _false : _true;
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
			if (_comp == CompType::EQ || _comp == CompType::GEQ) {
				tseitin = -translator()->translate(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
			} else if (_comp == CompType::NEQ || _comp == CompType::LT) {
				tseitin = translator()->translate(tsset.literals(), false, TsType::EQ);
				tseitin = isPos(_sign) ? tseitin : -tseitin;
			} else if (_comp == CompType::LEQ) {
				return isPos(_sign) ? _true : _false;
			} else if (_comp == CompType::GT) {
				return isPos(_sign) ? _false : _true;
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
	return conjunctive() == Conn::CONJ ? l == _true : l == _false;
}

Lit ClauseGrounder::redundantLiteral() const {
	return conjunctive() == Conn::CONJ ? _true : _false;
}

/**
 * conjunction: never true by a literal
 * disjunction: true if literal is true
 * negated conjunction: true if literal is false
 * negated disjunction: never true by a literal
 */
bool ClauseGrounder::makesFormulaTrue(Lit l) const {
	return (conjunctive() == Conn::DISJ && l == _true);
}

/**
 * conjunction: false if a literal is false
 * disjunction: never false by a literal
 * negated conjunction: never false by a literal
 * negated disjunction: false if a literal is true
 */
bool ClauseGrounder::makesFormulaFalse(Lit l) const {
	return (conjunctive() == Conn::CONJ && l == _false);
}

Lit ClauseGrounder::getEmtyFormulaValue() const {
	return conjunctive() == Conn::CONJ ? _true : _false;
}

bool ClauseGrounder::decidesFormula(Lit lit) const {
	return conjunctive() == Conn::CONJ ? lit == _false : lit == _true;
}

TsType ClauseGrounder::getTseitinType() const {
	return context()._tseitin;
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
	tseitin = translator()->translate(formula.literals, asConjunction, type);
	return tseitin;
}

void ClauseGrounder::run(ConjOrDisj& formula) const {
	internalRun(formula);
	if (isNegative()) {
		formula.literals = {-getReification(formula, getTseitinType())};
	}
}

FormStat ClauseGrounder::runSubGrounder(Grounder* subgrounder, bool conjFromRoot, ConjOrDisj& formula) const {
	Assert(formula.getType()==conjunctive());
	ConjOrDisj subformula;
	subgrounder->run(subformula);
	if (subformula.literals.size() == 0) {
		subformula.literals.push_back(subformula.getType() == Conn::CONJ ? _true : _false);
	}
	if (subformula.literals.size() == 1) {
		Lit l = subformula.literals[0];
		if (makesFormulaFalse(l)) {
			formula.literals = litlist { _false };
			return FormStat::DECIDED;
		} else if (makesFormulaTrue(l)) {
			formula.literals = litlist { _true };
			return FormStat::DECIDED;
		} else if (not isRedundantInFormula(l)) {
			formula.literals.push_back(l);
		}
		return FormStat::UNKNOWN;
	}
	// otherwise INVAR: subformula is not true nor false and does not contain true nor false literals
	if (conjFromRoot && conjunctiveWithSign()) {
		if (subformula.getType() == Conn::CONJ) {
			for (auto i = subformula.literals.cbegin(); i < subformula.literals.cend(); ++i) {
				getGrounding()->addUnitClause(*i);
			}
		} else {
			getGrounding()->add(subformula.literals);
		}
	} else {
		if (subformula.getType() == formula.getType()) {
			formula.literals.insert(formula.literals.begin(), subformula.literals.cbegin(), subformula.literals.cend());
		} else {
			formula.literals.push_back(getReification(subformula, subgrounder->context()._tseitin));
		}
	}
	return FormStat::UNKNOWN;
}

BoolGrounder::BoolGrounder(AbstractGroundTheory* grounding, const std::vector<Grounder*>& sub, SIGN sign, bool conj, const GroundingContext& ct)
		: ClauseGrounder(grounding, sign, conj, ct), _subgrounders(sub) {
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
	formula.setType(conjunctive());
	for (auto g = _subgrounders.cbegin(); g < _subgrounders.cend(); g++) {
		CHECKTERMINATION
		if (runSubGrounder(*g, context()._conjunctivePathFromRoot, formula) == FormStat::DECIDED) {
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
			}
			return;
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}

QuantGrounder::QuantGrounder(AbstractGroundTheory* grounding, FormulaGrounder* sub, SIGN sign, QUANT quant, InstGenerator* gen, InstChecker* checker,
		const GroundingContext& ct)
		: ClauseGrounder(grounding, sign, quant == QUANT::UNIV, ct), _subgrounder(sub), _generator(gen), _checker(checker) {
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
	printorig();
	formula.setType(conjunctive());
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
		CHECKTERMINATION
		if (_checker->check()) {
			formula.literals = litlist { context().gentype == GenType::CANMAKETRUE ? _true : _false };
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
				clog << tabs() << "Checker checked, hence formula decided. Result is " << translator()->printLit(formula.literals.front()) << "\n";
			}
			return;
		}
		if (runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, formula) == FormStat::DECIDED) {
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
			}
			return;
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}

EquivGrounder::EquivGrounder(AbstractGroundTheory* grounding, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct)
		: ClauseGrounder(grounding, sign, true, ct), _leftgrounder(lg), _rightgrounder(rg) {
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
	leftformula.setType(conjunctive());
	rightformula.setType(conjunctive());
	Assert(_leftgrounder->context()._monotone==Context::BOTH);
	Assert(_rightgrounder->context()._monotone==Context::BOTH);
	Assert(_leftgrounder->context()._tseitin==TsType::EQ|| _leftgrounder->context()._tseitin==TsType::RULE);
	Assert(_rightgrounder->context()._tseitin==TsType::EQ|| _rightgrounder->context()._tseitin==TsType::RULE);
	runSubGrounder(_leftgrounder, false, leftformula);
	runSubGrounder(_rightgrounder, false, rightformula);
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
			auto ts1 = translator()->translate(aornotb, false, context()._tseitin);
			auto ts2 = translator()->translate(notaorb, false, context()._tseitin);
			formula.literals = litlist { ts1, ts2 };
		}
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}
