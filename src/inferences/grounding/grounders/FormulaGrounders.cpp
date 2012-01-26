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

int verbosity() {
	return getOption(IntType::GROUNDVERBOSITY);
}

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
	clog << "\n" << tabs() << "Grounding formula " << toString(_origform);
	if (not _origform->freeVars().empty()) {
		pushtab();
		clog << "\n" << tabs() << "with instance ";
		for (auto it = _origform->freeVars().cbegin(); it != _origform->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << toString(e) << ' ';
		}
		poptab();
	}
	clog << "\n" << tabs();
}

AtomGrounder::AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol* s, const vector<TermGrounder*>& sg,
		const vector<const DomElemContainer*>& checkargs, InstChecker* pic, InstChecker* cic, PredInter* inter, const vector<SortTable*>& vst,
		const GroundingContext& ct)
		: FormulaGrounder(grounding, ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic), _symbol(translator()->addSymbol(s)), _tables(vst), _sign(sign),
			_checkargs(checkargs), _inter(inter),
			groundsubterms(_subtermgrounders.size()),
			args(_subtermgrounders.size()){
	gentype = ct.gentype;
}

AtomGrounder::~AtomGrounder() {
	deleteList(_subtermgrounders);
	delete (_pchecker);
	delete (_cchecker);
}

Lit AtomGrounder::run() const {
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}
	}

	// Run subterm grounders
	bool alldomelts = true;

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
					clog << "Term value out of predicate type\n" << tabs(); //TODO should be a warning
					clog << "Result is " << (isPos(_sign) ? "false" : "true") << "\n";
					if (_origform != NULL) {
						poptab();
					}
					clog << tabs();
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
			clog << "Possible checker failed\n" << tabs();
			//clog << "Result is " << (gentype == GenType::CANMAKETRUE ? "false" : "true");
			clog << "Result is false";
			if (_origform != NULL) {
				poptab();
			}
			clog << "\n" << tabs();
		}
		//return gentype == GenType::CANMAKETRUE ? _false : _false;
		return _false;
	}
	if (_cchecker->check()) { // Literal decides formula if checker succeeds
		if (verbosity() > 2) {
			clog << "Certain checker succeeded\n" << tabs();
			clog << "Result is " << translator()->printLit(gentype == GenType::CANMAKETRUE ? _true : _false);
			if (_origform != NULL) {
				poptab();
			}
			clog << "\n" << tabs();
		}
		return gentype == GenType::CANMAKETRUE ? _true : _false;
	}
	if (_inter->isTrue(args)) {
		if (verbosity() > 2) {
			clog << "Result is " << (isPos(_sign) ? "true" : "false");
			if (_origform != NULL) {
				poptab();
			}
			clog << "\n" << tabs();
		}
		return isPos(_sign) ? _true : _false;
	}
	if (_inter->isFalse(args)) {
		if (verbosity() > 2) {
			clog << "Result is " << (isPos(_sign) ? "false" : "true");
			if (_origform != NULL) {
				poptab();
			}
			clog << "\n" << tabs();
		}
		return isPos(_sign) ? _false : _true;
	}

	// Return grounding
	Lit lit = translator()->translate(_symbol, args);
	if (isNeg(_sign)) {
		lit = -lit;
	}
	if (verbosity() > 2) {
		clog << "Result is " << translator()->printLit(lit);
		if (_origform != NULL) {
			poptab();
		}
		clog << "\n" << tabs();
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
				result = leftvalue == rightvalue ? _true : _false;
				break;
			case CompType::NEQ:
				result = leftvalue != rightvalue ? _true : _false;
				break;
			case CompType::LEQ:
				result = leftvalue <= rightvalue ? _true : _false;
				break;
			case CompType::GEQ:
				result = leftvalue >= rightvalue ? _true : _false;
				break;
			case CompType::LT:
				result = leftvalue < rightvalue ? _true : _false;
				break;
			case CompType::GT:
				result = leftvalue > rightvalue ? _true : _false;
				break;
			}
		}
	}
	return result;
}

void ComparisonGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
	formula.literals.push_back(run()); // TODO can do better?
}

AggGrounder::~AggGrounder() {
	delete (_setgrounder);
	delete (_boundgrounder);
}

/**
 * Negate the comparator and invert the sign of the tseitin when the aggregate is in a doubly negated context.
 */
//TODO:why?
Lit AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	TsType tp = context()._tseitin;
	Lit tseitin = translator()->translate(boundvalue, negateComp(_comp), _type, setnr, tp);
	return isPos(_sign) ? -tseitin : tseitin;
}

Lit AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const {
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
		if (tp == TsType::IMPL)
			tp = TsType::RIMPL;
		else if (tp == TsType::RIMPL)
			tp = TsType::IMPL;
	}
	if (simplify) {
		if (_doublenegtseitin) {
			if (negateset) {
				Lit tseitin = translator()->translate(tsset.literals(), !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			} else {
				vector<Lit> newsetlits(tsset.size());
				for (size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				Lit tseitin = translator()->translate(newsetlits, !conj, tp);
				return isPos(_sign) ? -tseitin : tseitin;
			}
		} else {
			if (negateset) {
				vector<Lit> newsetlits(tsset.size());
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
 * TODO Can be optimized more (for special cases like in the "finish"-method, but won't be called often ayway.
 */
Lit AggGrounder::splitproducts(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const {
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

	int possetnumber = translator()->translateSet(poslits, posweights, tsset.trueweights());
	int negsetnumber = translator()->translateSet(neglits, negweights, { });

	auto tp = context()._tseitin;
	if (isNeg(_sign)) {
		if (tp == TsType::IMPL) {
			tp = TsType::RIMPL;
		} else if (tp == TsType::RIMPL) {
			tp = TsType::IMPL;
		}
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
	if (_doublenegtseitin)
		return handleDoubleNegation(newboundvalue, setnr);
	else {
		Lit tseitin;
		TsType tp = context()._tseitin;
		if (isNeg(_sign)) {
			tp = reverseImplication(tp);
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
	auto tsset = translator()->groundset(setnr);

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
	return conn_ == Conn::CONJ ? l == _true : l == _false;
}

/**
 * conjunction: never true by a literal
 * disjunction: true if literal is true
 * negated conjunction: true if literal is false
 * negated disjunction: never true by a literal
 */
bool ClauseGrounder::makesFormulaTrue(Lit l) const {
	return (conn_ == Conn::DISJ && l == _true);
}

/**
 * conjunction: false if a literal is false
 * disjunction: never false by a literal
 * negated conjunction: never false by a literal
 * negated disjunction: false if a literal is true
 */
bool ClauseGrounder::makesFormulaFalse(Lit l) const {
	return (conn_ == Conn::CONJ && l == _false);
}

Lit ClauseGrounder::getEmtyFormulaValue() const {
	return conn_ == Conn::CONJ ? _true : _false;
}

TsType ClauseGrounder::getTseitinType() const {
	return context()._tseitin;
}

// Takes context into account!
Lit ClauseGrounder::createTseitin(const ConjOrDisj& formula) const {
	TsType type = getTseitinType();
	/*if (isNegative()) {
	 type = reverseImplication(type);
	 }*/

	Lit tseitin;
	bool asConjunction = formula.getType() == Conn::CONJ;
	if (negativeDefinedContext()) {
		asConjunction = not asConjunction;
	}
	tseitin = translator()->translate(formula.literals, asConjunction, type);
	//return isNegative() ? -tseitin : tseitin;
	return tseitin;
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
	internalRun(formula);
	if (isNegative()) {
		//todo: Do this or use negate(formula)?
		Lit tseitin = getReification(formula);
		//formula.setType(Conn::CONJ);
		formula.literals = {-tseitin};
	}
}

FormStat ClauseGrounder::runSubGrounder(Grounder* subgrounder, bool conjFromRoot, ConjOrDisj& formula) const {
	Assert(formula.getType()==conn_);
	ConjOrDisj subformula;
	subgrounder->run(subformula);
	if (subformula.literals.size() == 0) {
		Lit value = subformula.getType() == Conn::CONJ ? _true : _false;
		if (makesFormulaTrue(value)) {
			formula.literals = litlist { _true };
			return FormStat::DECIDED;
		} else if (makesFormulaFalse(value)) {
			formula.literals = litlist { _false };
			return FormStat::DECIDED;
		} else if (subformula.getType() != formula.getType()) {
			formula.literals.push_back(value);
			return (FormStat::UNKNOWN);
		}
		return (FormStat::UNKNOWN);
	} else if (subformula.literals.size() == 1) {
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
	} // otherwise INVAR: subformula is not true nor false and does not contain true nor false literals
	//TODO: remove all the "negated"
	if (conjFromRoot && conjunctive()) {
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
			formula.literals.push_back(getReification(subformula));
		}
	}
	return FormStat::UNKNOWN;
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
	formula.setType(conn_);
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

	formula.setType(conn_);

	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
		CHECKTERMINATION
		if (_checker->check()) {
			std::cerr << toString(_checker);
			formula.literals = litlist { context().gentype == GenType::CANMAKETRUE ? _false : _true };
			if (verbosity() > 2 and _origform != NULL) {
				poptab();
				clog << "Checker checked, hence formula decided. Result is " << translator()->printLit(formula.literals.front()) << "\n" << tabs();
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

EquivGrounder::~EquivGrounder() {
	delete (_leftgrounder);
	delete (_rightgrounder);
}

Lit EquivGrounder::getLitEquivWith(const ConjOrDisj& form) const {
	if (form.literals.size() == 0) {
		if (form.getType() == Conn::CONJ) {
			return _true;
		} else {
			return _false;
		}
	} else if (form.literals.size() == 1) {
		return form.literals[0];
	} else {
		return getReification(form);
	}
}

void EquivGrounder::internalRun(ConjOrDisj& formula) const {
	//Assert(not negated); I think this is not needed.
	if (verbosity() > 2) {
		printorig();
		if (_origform != NULL) {
			pushtab();
		}

		clog << "Current formula: " << (isNegative() ? "~" : "");
		_leftgrounder->printorig();
		clog << " <=> ";
		_rightgrounder->printorig();
	}

	// Run subgrounders
	ConjOrDisj leftformula, rightformula;
	leftformula.setType(conn_);
	rightformula.setType(conn_);
	runSubGrounder(_leftgrounder, false, leftformula);
	runSubGrounder(_rightgrounder, false, rightformula);
	auto left = getLitEquivWith(leftformula);
	auto right = getLitEquivWith(rightformula);

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
		TsType tp = context()._tseitin;
		Lit ts1 = translator()->translate(aornotb, false, tp);
		Lit ts2 = translator()->translate(notaorb, false, tp);
		formula.literals = litlist { ts1, ts2 };
		formula.setType(Conn::CONJ);
	}
	if (verbosity() > 2 and _origform != NULL) {
		poptab();
	}
}
