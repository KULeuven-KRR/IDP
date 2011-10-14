/************************************
 FormulaGrounders.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/FormulaGrounders.hpp"

#include <iostream>
#include "vocabulary.hpp"
#include "ecnf.hpp"
#include "grounders/TermGrounders.hpp"
#include "grounders/SetGrounders.hpp"
#include "common.hpp"
#include "generator.hpp"
#include "checker.hpp"
#include <cmath>

using namespace std;

void FormulaGrounder::setOrig(const Formula* f, const map<Variable*, const DomElemContainer*>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(auto it = f->freeVars().begin(); it != f->freeVars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[*it] = mvd.find(*it)->second;
		_origvarmap[v] = mvd.find(*it)->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	clog << "Grounding formula " << _origform->toString();
	if(not _origform->freeVars().empty()) {
		clog << " with instance ";
		for(auto it = _origform->freeVars().begin(); it != _origform->freeVars().end(); ++it) {
			clog << (*it)->toString() << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << e->toString() << ' ';
		}
	}
	clog << "\n";
}

AtomGrounder::AtomGrounder(
		GroundTranslator* gt,
		SIGN sign,
		PFSymbol* s,
		const vector<TermGrounder*>& sg,
		const vector<const DomElemContainer*>& checkargs,
		InstGenerator* pic, InstGenerator* cic,
		PredInter* inter,
		const vector<SortTable*>& vst,
		const GroundingContext& ct)
		: FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic),
		_symbol(gt->addSymbol(s)), _tables(vst), _sign(sign), _checkargs(checkargs), _inter(inter){
	_certainvalue = ct._truegen ? _true : _false;
}

Lit AtomGrounder::run() const {
	if(verbosity() > 2) printorig();

	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}

	// Checking partial functions
	for(unsigned int n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(not groundsubterms[n]._isvarid && not args[n]) {
			//TODO: produce a warning!
			if(context()._funccontext == Context::BOTH) {
				// TODO: produce an error
			}
			if(verbosity() > 2) {
				clog << "Partial function went out of bounds\n";
				clog << "Result is " << (context()._funccontext != Context::NEGATIVE  ? "true" : "false") << "\n";
			}
			return context()._funccontext != Context::NEGATIVE  ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < args.size(); ++n) {
		if(not groundsubterms[n]._isvarid && not _tables[n]->contains(args[n])) {
			if(verbosity() > 2) {
				clog << "Term value out of predicate type\n";
				clog << "Result is " << (isPos(_sign)? "false" : "true") << "\n";
			}
			return isPos(_sign)? _false : _true;
		}
	}

	// Run instance checkers
	if(alldomelts) {
		for(size_t n = 0; n < args.size(); ++n) {
        	*(_checkargs[n]) = args[n];
		}
		if(not _pchecker->first()) {
			if(verbosity() > 2) {
				clog << "Possible checker failed\n";
				clog << "Result is " << (_certainvalue ? "false" : "true") << "\n";
			}
			return _certainvalue ? _false : _true;	// TODO: dit is lelijk
		}
		if(_cchecker->first()) {
			if(verbosity() > 2) {
				clog << "Certain checker succeeded\n";
 				clog << "Result is " << translator()->printAtom(_certainvalue, false) << "\n"; //TODO longnames?
			}
			return _certainvalue;
		}
		if(_inter->isTrue(args)) { return isPos(_sign)?_true:_false; }
		if(_inter->isFalse(args)) { return isPos(_sign)?_false:_true; }
	}

	// Return grounding
	if(alldomelts) {
		int atom = translator()->translate(_symbol,args);
		if(isNeg(_sign)) atom = -atom;
		if(verbosity() > 2) {
			clog << "Result is " << translator()->printAtom(atom, false) << "\n"; // TODO longnames?
		}
		return atom;
	}
	else {
		//TODO Should we handle CPSymbols (that are not comparisons) here? No!
		//TODO Should we assert(alldomelts)? Maybe yes, if P(t) and (not isCPSymbol(P)) and isCPSymbol(t) then it should have been rewritten, right?
		//TODO If not previous... Do we need a GroundTranslator::translate method that takes GroundTerms as args??
		assert(false);
	}
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
}

int ComparisonGrounder::run() const {
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();

	//TODO Is following check necessary??
	if((not left._domelement && not left._varid) || (not right._domelement && not right._varid)) {
		return context()._funccontext != Context::NEGATIVE  ? _true : _false;
	}

	//TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???

	if(left._isvarid) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if(right._isvarid) {
			CPBound rightbound(right._varid);
			return translator()->translate(leftterm,_comparator,rightbound,TsType::EQ); //TODO use _context._tseitin?
		}
		else {
			assert(not right._isvarid);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			return translator()->translate(leftterm,_comparator,rightbound,TsType::EQ); //TODO use _context._tseitin?
		}
	}
	else {
		assert(not left._isvarid);
		int leftvalue = left._domelement->value()._int;
		if(right._isvarid) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			return translator()->translate(rightterm,invertComp(_comparator),leftbound,TsType::EQ); //TODO use _context._tseitin?
		}
		else {
			assert(not right._isvarid);
			int rightvalue = right._domelement->value()._int;
			switch(_comparator) {
				case CompType::EQ: return leftvalue == rightvalue ? _true : _false;
				case CompType::NEQ: return leftvalue != rightvalue ? _true : _false;
				case CompType::LEQ: return leftvalue <= rightvalue ? _true : _false;
				case CompType::GEQ: return leftvalue >= rightvalue ? _true : _false;
				case CompType::LT: return leftvalue < rightvalue ? _true : _false;
				case CompType::GT: return leftvalue > rightvalue ? _true : _false;
			}
		}
	}
	assert(false);
	return 0;
}

void ComparisonGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
}

/**
 * Invert the comparator and the sign of the tseitin when the aggregate is in a doubly negated context.
 */
int AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	CompType newcomp;
	switch(_comp) {
		case AGG_LT : newcomp = CompType::GT; break;
		case AGG_GT : newcomp = CompType::LT; break;
		case AGG_EQ : assert(false); break;
	}
	TsType tp = context()._tseitin;
	int tseitin = translator()->translate(boundvalue,newcomp,false,_type,setnr,tp);
	return isPos(_sign)? -tseitin : tseitin;
}

int AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	const TsSet& tsset = translator()->groundset(setnr);
	int maxposscard = tsset.size();
	TsType tp = context()._tseitin;
	bool simplify = false;
	bool conj;
	bool negateset;
	switch(_comp) {
		case AGG_EQ:
			if(leftvalue < 0 || leftvalue > maxposscard) {
				return isPos(_sign)? _false : _true;
			}
			else if(leftvalue == 0) {
				simplify = true;
				conj = true;
				negateset = true;
			}
			else if(leftvalue == maxposscard) {
				simplify = true;
				conj = true;
				negateset = false;
			}
			break;
		case AGG_LT:
			if(leftvalue < 0) {
				return isPos(_sign)? _true : _false;
			}
			else if(leftvalue == 0) {
				simplify = true;
				conj = false;
				negateset = false;
			}
			else if(leftvalue == maxposscard-1) {
				simplify = true;
				conj = true;
				negateset = false;
			}
			else if(leftvalue >= maxposscard) {
				return isPos(_sign)? _false : _true;
			}
			break;
		case AGG_GT:
			if(leftvalue <= 0) {
				return isPos(_sign)? _false : _true;
			}
			else if(leftvalue == 1) {
				simplify = true;
				conj = true;
				negateset = true;
			}
			else if(leftvalue == maxposscard) {
				simplify = true;
				conj = false;
				negateset = true;
			}
			else if(leftvalue > maxposscard) {
				return isPos(_sign)? _true : _false;
			}
			break;
	}
	if(isNeg(_sign)) {
		if(tp == TsType::IMPL) tp = TsType::RIMPL;
		else if(tp == TsType::RIMPL) tp = TsType::IMPL;
	}
	if(simplify) {
		if(_doublenegtseitin) {
			if(negateset) {
				int tseitin = translator()->translate(tsset.literals(),!conj,tp);
				return isPos(_sign)? -tseitin : tseitin;
			}
			else {
				vector<int> newsetlits(tsset.size());
				for(unsigned int n = 0; n < tsset.size(); ++n) newsetlits[n] = -tsset.literal(n);
				int tseitin = translator()->translate(newsetlits,!conj,tp);
				return isPos(_sign)? -tseitin : tseitin;
			}
		}
		else {
			if(negateset) {
				vector<int> newsetlits(tsset.size());
				for(unsigned int n = 0; n < tsset.size(); ++n) newsetlits[n] = -tsset.literal(n);
				int tseitin = translator()->translate(newsetlits,conj,tp);
				return isPos(_sign)? tseitin : -tseitin;
			}
			else {
				int tseitin = translator()->translate(tsset.literals(),conj,tp);
				return isPos(_sign)? tseitin : -tseitin;
			}
		}
	}
	else {
		if(_doublenegtseitin) return handleDoubleNegation(double(leftvalue),setnr);
		else {
			// TODO define cast from AggComp to CompType
			CompType comp = CompType::EQ;
			switch(_comp){
			case AGG_EQ: comp = CompType::EQ; break;
			case AGG_LT: comp = CompType::LT; break;
			case AGG_GT: comp = CompType::GT; break;
			}
			int tseitin = translator()->translate(double(leftvalue),comp,true,AggFunction::CARD,setnr,tp);
			return isPos(_sign)? tseitin : -tseitin;
		}
	}
}

/**
 * General finish method for grounding of sum, product, minimum and maximum aggregates.
 * Checks whether the aggregate will be certainly true or false, based on minimum and maximum possible values and the given bound;
 * and creates a tseitin, handling double negation when necessary;
 */
int AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const {
	// Check minimum and maximum possible values against the given bound
	CompType comp = CompType::EQ;
	switch(_comp) { //TODO more complicated propagation is possible!
		case AGG_EQ:
			comp = CompType::EQ;
			if(minpossvalue > boundvalue || maxpossvalue < boundvalue)
				return isPos(_sign)? _false : _true;
			break;
		case AGG_LT:
			comp = CompType::LT;
			if(boundvalue < minpossvalue)
				return isPos(_sign)? _true : _false;
			else if(boundvalue >= maxpossvalue)
				return isPos(_sign)? _false : _true;
			break;
		case AGG_GT:
			comp = CompType::GT;
			if(boundvalue > maxpossvalue)
				return isPos(_sign)? _true : _false;
			else if(boundvalue <= minpossvalue)
				return isPos(_sign)? _false : _true;
			break;
	}
	if(_doublenegtseitin)
		return handleDoubleNegation(newboundvalue,setnr);
	else {
		int tseitin;
		TsType tp = context()._tseitin;
		if(isNeg(_sign)) {
			if(tp == TsType::IMPL) tp = TsType::RIMPL;
			else if(tp == TsType::RIMPL) tp = TsType::IMPL;
		}
		tseitin = translator()->translate(newboundvalue,comp,true,_type,setnr,tp);
		return isPos(_sign)? tseitin : -tseitin;
	}
}

int AggGrounder::run() const {
	// Run subgrounders
	int setnr = _setgrounder->run();
	const GroundTerm& groundbound = _boundgrounder->run();
	assert(not groundbound._isvarid); //TODO
	const DomainElement* bound = groundbound._domelement;

	// Retrieve the set, note that weights might be changed when handling min and max aggregates.
	TsSet& tsset = translator()->groundset(setnr);

	// Retrieve the value of the bound
	double boundvalue = bound->type() == DET_INT ? (double) bound->value()._int : bound->value()._double;

	// Compute the value of the aggregate based on weights of literals that are certainly true.
	double truevalue = applyAgg(_type,tsset.trueweights());

	// When the set is empty, return an answer based on the current value of the aggregate.
	if(tsset.literals().empty()) {
		bool returnvalue;
		switch(_comp) {
			case AGG_LT : returnvalue = boundvalue < truevalue; break;
			case AGG_GT : returnvalue = boundvalue > truevalue; break;
			case AGG_EQ : returnvalue = boundvalue == truevalue; break;
		}
		return isPos(_sign) == returnvalue ? _true : _false;
	}

	// Handle specific aggregates.
	int tseitin;
	double minpossvalue = truevalue;
	double maxpossvalue = truevalue;
	switch(_type) {
		case AggFunction::CARD:
			tseitin = finishCard(truevalue,boundvalue,setnr);
			break;
		case AggFunction::SUM:
			// Compute the minimum and maximum possible value of the sum.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				if(tsset.weight(n) > 0) maxpossvalue += tsset.weight(n);
				else if(tsset.weight(n) < 0) minpossvalue += tsset.weight(n);
			}
			// Finish
			tseitin = finish(boundvalue,(boundvalue-truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		case AggFunction::PROD: {
			// Compute the minimum and maximum possible value of the product.
			bool containsneg = false;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				maxpossvalue *= abs(tsset.weight(n));
				if(tsset.weight(n) < 0) containsneg = true;
			}
			if(containsneg) minpossvalue = -maxpossvalue;
			// Finish
			tseitin = finish(boundvalue,(boundvalue/truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AggFunction::MIN:
			// Compute the minimum possible value of the set.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
				// Decrease all weights greater than truevalue to truevalue. // TODO why not just drop all those?
				if(tsset.weight(n) > truevalue) tsset.setWeight(n,truevalue);
			}
			// Finish
			tseitin = finish(boundvalue,boundvalue,minpossvalue,maxpossvalue,setnr);
			break;
		case AggFunction::MAX:
			// Compute the maximum possible value of the set.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
				// Increase all weights less than truevalue to truevalue. // TODO why not just drop all those?
				if(tsset.weight(n) < truevalue) tsset.setWeight(n,truevalue);
			}
			// Finish
			tseitin = finish(boundvalue,boundvalue,minpossvalue,maxpossvalue,setnr);
			break;
	}
	return tseitin;
}

void AggGrounder::run(litlist& clause) const {
	clause.push_back(run());
}

bool ClauseGrounder::isNotRedundantInClause(Lit l) const {
	return conn_==CONJ? l!=_true : l!=_false;
}

// True of the value of the literal immediately makes the tseitin formula true
bool ClauseGrounder::decidesClause(Lit l) const {
	return conn_==CONJ ? l == _false : l == _true;
}

// Get the value if one literal has decided the value of the tseitin formula (so false if it is a conjunction, true if it is a disjunction)
Lit ClauseGrounder::getDecidedValue() const {
	return conjunctive() ? _false : _true;
}

Lit ClauseGrounder::getEmtyFormulaValue() const {
	return conjunctive() ? _true : _false;
}

TsType ClauseGrounder::getTseitinType() const{
	return context()._tseitin;
}

Lit ClauseGrounder::createTseitin(const litlist& clause) const{
	TsType type = getTseitinType();
	if(isNegative()) {
		type = reverseImplication(type);
	}
	Lit tseitin;
	if(negativeDefinedContext()) {
		tseitin = translator()->translate(clause,conn_==DISJ,type);
	}else{
		tseitin = translator()->translate(clause,conn_==CONJ,type);
	}
	return isNegative()?-tseitin:tseitin;
}

Lit ClauseGrounder::getReification(litlist& clause) const {
	if(clause.empty()) {
		return getEmtyFormulaValue();
	}

	if(clause.size() == 1) {
		return clause[0];
	}

	return createTseitin(clause);
}

// FIXME unittests
// FIXME add printing code again
Lit ClauseGrounder::run() const {
	litlist clause;
	bool negateclause = isNegative() && not negativeDefinedContext();
	negateclause &= isPositive() && negativeDefinedContext();
	run(clause, negateclause);
	return getReification(clause);
}

void ClauseGrounder::run(litlist& clause) const {
	run(clause, isNegative());
}

// NOTE: Optimized to avoid looping over the clause after construction
void BoolGrounder::run(litlist& clause, bool negateclause) const {
	if(verbosity() > 2) printorig();

	for(auto g=_subgrounders.begin(); g<_subgrounders.end(); g++){
		Lit lit = (*g)->run();
		if(decidesClause(lit)) {
			Lit valuelit = getDecidedValue();
			clause = litlist{negateclause?-valuelit:valuelit};
			break;
		}else if(isNotRedundantInClause(lit)){
			clause.push_back(negateclause?-lit:lit);
		}
	}
}

// FIXME what are the "clause" semantics here? => apperently, these are unimportant, as the sentencegrounder currently magically "knows" what it will get back
void QuantGrounder::run(litlist& clause, bool negateclause) const {
	if(verbosity() > 2) printorig();

	if(not _generator->first()) {
		return;
	}

	do{
		if(_checker->first()) {
			Lit valuelit = getDecidedValue();
			clause = litlist{negateclause?-valuelit:valuelit};
			break;
		}
		Lit l = _subgrounder->run();
		if(decidesClause(l)) {
			Lit valuelit = getDecidedValue();
			clause = litlist{negateclause?-valuelit:valuelit};
			break;
		}else if(isNotRedundantInClause(l)){
			clause.push_back(negateclause ? -l : l);
		}
	}while(_generator->next());
}

Lit EquivGrounder::run() const {
	litlist clause;

	run(clause);

	if(clause.size()>1){
		return translator()->translate(clause,true,context()._tseitin);
	}else{
		assert(clause.size()>0);
		return clause[0];
	}
}

void EquivGrounder::run(litlist& clause) const {
	if(verbosity() > 2) printorig();

	// Run subgrounders
	Lit left = _leftgrounder->run();
	Lit right = _rightgrounder->run();

	if(left == right) {
		clause.push_back(isPositive() ? _true : _false);
	}else if(left == _true) {
		if(right == _false) {
			clause.push_back(isPositive() ? _false : _true);
		} else {
			clause.push_back(isPositive() ? right : -right);
		}
	} else if(left == _false) {
		if(right == _true) {
			clause.push_back(isPositive() ? _false : _true);
		} else {
			clause.push_back(isPositive() ? -right : right);
		}
	} else if(right == _true) {  // left is not true or false
		clause.push_back(isPositive() ? left : -left);
	} else if(right == _false) {
		clause.push_back(isPositive() ? -left : left);
	} else {
		litlist cl1 = {left, isPositive()?-right:right};
		litlist cl2 = {-left, isPositive()?right:-right};
		TsType tp = context()._tseitin;
		Lit ts1 = translator()->translate(cl1,false,tp);
		Lit ts2 = translator()->translate(cl2,false,tp);
		clause = litlist{ts1, ts2};
/*		litlist cl3 = {ts1, ts2};
		Lit head = translator()->translate(cl3,true,tp);
		clause.push_back(head);*/
		// FIXME, we should not introduce the third tseitin, if the semantics of the returned datastructure are a conjunction instead of a clause
	}
}
