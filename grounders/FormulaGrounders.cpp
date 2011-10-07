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

void FormulaGrounder::setorig(const Formula* f, const map<Variable*, const DomElemContainer*>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(auto it = f->freevars().begin(); it != f->freevars().end(); ++it) {
		_varmap[*it] = mvd.find(*it)->second;

		// Clone variable to store original
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_origvarmap[v] = mvd.find(*it)->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	clog << "Grounding formula " << _origform->to_string();
	if(not _origform->freevars().empty()) {
		clog << " with instance ";
		for(auto it = _origform->freevars().begin(); it != _origform->freevars().end(); ++it) {
			clog << (*it)->to_string() << " = ";
			const DomainElement* e = _origvarmap.find(*it)->second->get();
			clog << e->to_string() << ' ';
		}
	}
	clog << "\n";
}

AtomGrounder::AtomGrounder(GroundTranslator* gt, SIGN sign, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
		FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic),
		_symbol(gt->addSymbol(s)), _tables(vst), _sign(sign){
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
		if(not _pchecker->isInInterpretation(args)) {
			if(verbosity() > 2) {
				clog << "Possible checker failed\n";
				clog << "Result is " << (_certainvalue ? "false" : "true") << "\n";
			}
			return _certainvalue ? _false : _true;	// TODO: dit is lelijk
		}
		if(_cchecker->isInInterpretation(args)) {
			if(verbosity() > 2) {
				clog << "Certain checker succeeded\n";
 				clog << "Result is " << translator()->printAtom(_certainvalue, false) << "\n"; //TODO longnames?
			}
			return _certainvalue;
		}
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

//CPAtomGrounder::CPAtomGrounder(GroundTranslator* gt, GroundTermTranslator* tt, SIGN sign, Function* func,
//							const vector<TermGrounder*> vtg, InstanceChecker* pic, InstanceChecker* cic,
//							const vector<SortTable*>& vst, const GroundingContext& ct) :
//	AtomGrounder(gt,sign,func,vtg,pic,cic,vst,ct), _termtranslator(tt) { }

//int CPAtomGrounder::run() const {
//	if(verbosity() > 2) printorig();
//	// Run subterm grounders
//	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
//		_args[n] = _subtermgrounders[n]->run();
//	}
//
//	// Checking partial functions
//	for(unsigned int n = 0; n < _args.size(); ++n) {
//		//TODO: only check positions that can be out of bounds!
//		if(!_args[n]) {
//			//TODO: produce a warning!
//			if(context()._funccontext == Context::BOTH) {
//				// TODO: produce an error
//			}
//			if(verbosity() > 2) {
//				clog << "Partial function went out of bounds\n";
//				clog << "Result is " << (context()._funccontext != Context::NEGATIVE  ? "true" : "false") << "\n";
//			}
//			return context()._funccontext != Context::NEGATIVE  ? _true : _false;
//		}
//	}
//
//	// Checking out-of-bounds
//	for(unsigned int n = 0; n < _args.size(); ++n) {
//		if(!_tables[n]->contains(_args[n])) {
//			if(verbosity() > 2) {
//				clog << "Term value out of predicate type\n";
//				clog << "Result is " << (_sign  ? "false" : "true") << "\n";
//			}
//			return isPos(_sign)? _false : _true;
//		}
//	}
//
//	// Run instance checkers
//	if(!(_pchecker->run(_args))) {
//		if(verbosity() > 2) {
//			clog << "Possible checker failed\n";
//			clog << "Result is " << (_certainvalue ? "false" : "true") << "\n";
//		}
//		return _certainvalue ? _false : _true;	// TODO: dit is lelijk
//	}
//	if(_cchecker->run(_args)) {
//		if(verbosity() > 2) {
//			clog << "Certain checker succeeded\n";
//			clog << "Result is " << translator()->printAtom(_certainvalue) << "\n";
//		}
//		return _certainvalue;
//	}
//
//	// Return grounding
//	assert(typeid(*(translator()->getSymbol(_symbol))) == typeid(Function)); // by definition...
//	Function* func = static_cast<Function*>(translator()->getSymbol(_symbol));
//	ElementTuple args = _args; args.pop_back();
//	int value = _args.back()->value()._int;
//
//	unsigned int varid = _termtranslator->translate(func,args); //FIXME conversion is nasty...
//	CPTerm* leftterm = new CPVarTerm(varid);
//	CPBound rightbound(value);
//	int atom = translator()->translate(leftterm,CompType::EQ,rightbound,TsType::EQ);
//	if(!_sign) atom = -atom;
//	return atom;
//}

int ComparisonGrounder::run() const {
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();

	//XXX Is following check necessary??
	if((not left._domelement && not left._varid) || (not right._domelement && not right._varid)) {
		return context()._funccontext != Context::NEGATIVE  ? _true : _false;
	}

	//TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???

	if(left._isvarid) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if(right._isvarid) {
			CPBound rightbound(right._varid);
//			return translator()->translate(leftterm,_comparator,rightbound,context()._tseitin);
			return translator()->translate(leftterm,_comparator,rightbound,TsType::EQ);
		}
		else {
			assert(not right._isvarid);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			return translator()->translate(leftterm,_comparator,rightbound,TsType::EQ);
		}
	}
	else {
		assert(not left._isvarid);
		int leftvalue = left._domelement->value()._int;
		if(right._isvarid) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			return translator()->translate(rightterm,invertcomp(_comparator),leftbound,TsType::EQ);
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
 * int AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const
 * DESCRIPTION
 * 		Invert the comparator and the sign of the tseitin when the aggregate is in a doubly negated context.
 */
int AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	bool newcomp;
	switch(_comp) {
		case AGG_LT : newcomp = AGG_GT; break;
		case AGG_GT : newcomp = AGG_LT; break;
		case AGG_EQ : assert(false); break;
	}
	TsType tp = context()._tseitin;
	int tseitin = translator()->translate(boundvalue,newcomp,false,_type,setnr,tp);
	return isPos(_sign)? -tseitin : tseitin;
}

/**
 * int AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const
 */
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
			int tseitin = translator()->translate(double(leftvalue),_comp,true,AggFunction::CARD,setnr,tp);
			return isPos(_sign)? tseitin : -tseitin;
		}
	}
}

/**
 * int AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const
 * DESCRIPTION
 * 		General finish method for grounding of sum, product, minimum and maximum aggregates.
 * 		Checks whether the aggregate will be certainly true or false, based on minimum and maximum possible values and the given bound;
 * 		and creates a tseitin, handling double negation when necessary;
 */
int AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const {
	// Check minimum and maximum possible values against the given bound
	switch(_comp) { //TODO more complicated propagation is possible!
		case AGG_EQ:
			if(minpossvalue > boundvalue || maxpossvalue < boundvalue)
				return isPos(_sign)? _false : _true;
			break;
		case AGG_LT:
			if(boundvalue < minpossvalue)
				return isPos(_sign)? _true : _false;
			else if(boundvalue >= maxpossvalue)
				return isPos(_sign)? _false : _true;
			break;
		case AGG_GT:
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
		tseitin = translator()->translate(newboundvalue,_comp,true,_type,setnr,tp);
		return isPos(_sign)? tseitin : -tseitin;
	}
}

/**
 * int AggGrounder::run() const
 * DESCRIPTION
 * 		Run the aggregate grounder.
 */
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

inline bool ClauseGrounder::isNotRedundantInClause(Lit l) const {
	return conn_==CONJ? l!=_true : l!=_false;
}
// True of the value of the literal immediately makes the tseitin formula true
inline bool ClauseGrounder::decidesClause(Lit l) const {
	return conn_==CONJ ? l == _false : l == _true;
}

// Get the value if one literal has decided the value of the tseitin formula (so false if it is a conjunction, true if it is a disjunction)
inline Lit ClauseGrounder::getDecidedValue() const {
	return conjunctive() ? _false : _true;
}

inline Lit ClauseGrounder::getEmtyFormulaValue() const {
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

void QuantGrounder::run(litlist& clause, bool negateclause) const {
	if(verbosity() > 2) printorig();

	if(not _generator->first()) {
		return;
	}
	do{
		Lit l = _subgrounder->run();
		if(decidesClause(l)) {
			Lit valuelit = getDecidedValue();
			clause = litlist{negateclause?-valuelit:valuelit};
			break;
		}else if(isNotRedundantInClause(l)){
			clause.push_back(negateclause ? -l : l);
		}
	}while(_generator->next()); // TODO generators are not intuitive to understand (changing the underlying structure according to which grounding occurs?
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
		clause.push_back(ts1); clause.push_back(ts2);
	}
}
