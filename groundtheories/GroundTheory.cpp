#include "GroundTheory.hpp"

#include "GroundPolicy.hpp"
#include "SolverPolicy.hpp"
#include "PrintGroundPolicy.hpp"

template class GroundTheory<GroundPolicy>;
template class GroundTheory<PrintGroundPolicy>;
template class GroundTheory<SolverPolicy>;

template<typename Policy>
void GroundTheory<Policy>::accept(TheoryVisitor* v) const {
	v->visit(this);
}
template<typename Policy>
AbstractTheory* GroundTheory<Policy>::accept(TheoryMutatingVisitor* v) {
	return v->visit(this);
}

template<typename Policy>
void GroundTheory<Policy>::transformForAdd(const std::vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst) {
	size_t n = 0;
	if (skipfirst) {
		++n;
	}
	for (; n < vi.size(); ++n) {
		int atom = abs(vi[n]);
		if (translator()->isTseitinWithSubformula(atom) && _printedtseitins.find(atom) == _printedtseitins.end()) {
			_printedtseitins.insert(atom);
			TsBody* tsbody = translator()->getTsBody(atom);
			if (typeid(*tsbody) == typeid(PCTsBody)){
				PCTsBody * body = dynamic_cast<PCTsBody*>(tsbody);
				if (body->type() == TsType::IMPL || body->type() == TsType::EQ) {
					if (body->conj()) {
						for (unsigned int m = 0; m < body->size(); ++m) {
							std::vector<int> cl { -atom, body->literal(m) };
							add(cl, true);
						}
					} else {
						std::vector<int> cl(body->size() + 1, -atom);
						for (size_t m = 0; m < body->size(); ++m) {
							cl[m + 1] = body->literal(m);
						}
						add(cl, true);
					}
				}
				if (body->type() == TsType::RIMPL || body->type() == TsType::EQ) {
					if (body->conj()) {
						std::vector<int> cl(body->size() + 1, atom);
						for (size_t m = 0; m < body->size(); ++m) {
							cl[m + 1] = -body->literal(m);
						}
						add(cl, true);
					} else {
						for (size_t m = 0; m < body->size(); ++m) {
							std::vector<int> cl(2, atom);
							cl[1] = -body->literal(m);
							add(cl, true);
						}
					}
				}
				if (body->type() == TsType::RULE) {
					// FIXME when doing this lazily, the rule should not be here until the tseitin has a value!
					Assert(defnr != ID_FOR_UNDEFINED);
					Policy::polAdd(defnr, new PCGroundRule(atom, body, true)); //TODO true (recursive) might not always be the case?
				}
			} else if (typeid(*tsbody) == typeid(AggTsBody)) {
				AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
				if (body->type() == TsType::RULE) {
					Assert(defnr != ID_FOR_UNDEFINED);
					add(body->setnr(), ID_FOR_UNDEFINED, (body->aggtype() != AggFunction::CARD));
					Policy::polAdd(defnr, new AggGroundRule(atom, body, true)); //TODO true (recursive) might not always be the case?
				} else {
					add(atom, body);
				}
			} else if (typeid(*tsbody) == typeid(CPTsBody)) {
				CPTsBody* body = dynamic_cast<CPTsBody*>(tsbody);
				if (body->type() == TsType::RULE) {
					Assert(false);
					//TODO Does this ever happen?
				} else {
					add(atom, body);
				}
			} else {
				Assert(typeid(*tsbody) == typeid(LazyTsBody));
				LazyTsBody* body = dynamic_cast<LazyTsBody*>(tsbody);
				body->notifyTheoryOccurence();
			}
		}
	}
}

template<typename Policy>
CPTerm* GroundTheory<Policy>::foldCPTerm(CPTerm* cpterm) {
	if (_foldedterms.find(cpterm) == _foldedterms.end()) {
		_foldedterms.insert(cpterm);
		if (typeid(*cpterm) == typeid(CPVarTerm)) {
			CPVarTerm* varterm = static_cast<CPVarTerm*>(cpterm);
			if (not termtranslator()->function(varterm->varid())) {
				CPTsBody* cprelation = termtranslator()->cprelation(varterm->varid());
				CPTerm* left = foldCPTerm(cprelation->left());
				if ((typeid(*left) == typeid(CPSumTerm) || typeid(*left) == typeid(CPWSumTerm)) && cprelation->comp() == CompType::EQ) {
					Assert(cprelation->right()._isvarid && cprelation->right()._varid == varterm->varid());
					return left;
				}
			}
		} else if (typeid(*cpterm) == typeid(CPSumTerm)) {
			CPSumTerm* sumterm = static_cast<CPSumTerm*>(cpterm);
			std::vector<VarId> newvarids;
			for (auto it = sumterm->varids().begin(); it != sumterm->varids().end(); ++it) {
				if (not termtranslator()->function(*it)) {
					CPTsBody* cprelation = termtranslator()->cprelation(*it);
					CPTerm* left = foldCPTerm(cprelation->left());
					if (typeid(*left) == typeid(CPSumTerm) && cprelation->comp() == CompType::EQ) {
						CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
						Assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
						newvarids.insert(newvarids.end(), subterm->varids().begin(), subterm->varids().end());
					}
					//TODO Need to do something special in other cases?
					else
						newvarids.push_back(*it);
				} else
					newvarids.push_back(*it);
			}
			sumterm->varids(newvarids);
		} else if (typeid(*cpterm) == typeid(CPWSumTerm)) {
			//CPWSumTerm* wsumterm = static_cast<CPWSumTerm*>(cpterm);
			//TODO
		}
	}
	return cpterm;
}

/**
 *	Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
 *	This method should be called before running the SAT solver and after grounding.
 */
template<typename Policy>
void GroundTheory<Policy>::addFuncConstraints() {
	for (unsigned int n = 0; n < translator()->nbManagedSymbols(); ++n) {
		if(GlobalData::instance()->terminateRequested()){
			throw IdpException("Terminate requested");
		}
		auto pfs = translator()->getManagedSymbol(n);
		if (typeid(*pfs) != typeid(Function)) {
			continue;
		}
		auto f = dynamic_cast<Function*>(pfs); //TODO: use the template-version check?

		//TODO: optimization: if tuples.size = domain.size, we can use tuples instead of pt... This avoids calling the translator every time...
		auto tuples = translator()->getTuples(n);

		if (tuples.empty()) {
			continue;
		}

		StrictWeakNTupleEquality tuplesFirstNEqual(f->arity());
		StrictWeakNTupleOrdering tuplesFirstNSmaller(f->arity());

		const PredTable* ct = structure()->inter(f)->graphInter()->ct();
		const PredTable* pt = structure()->inter(f)->graphInter()->pt();
		SortTable* outSortTable = structure()->inter(f->outsort());
		if (not pt->finite()) {
			thrownotyetimplemented("Functions using infinite types");
			//FIXME make this work for functions with infintie domains and/or infinite out-sorts.  Take a look at lower code...
		}

		ElementTuple domainElement(f->arity(), 0);
		TableIterator ctIterator = ct->begin();
		SortIterator outSortIterator = outSortTable->sortBegin();
		std::vector<litlist> sets; //NOTE: for every domain element (x1,x2): one set containing all (x1,x2,y).  The cardinality of this set should be 1

		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			if(GlobalData::instance()->terminateRequested()){
				throw IdpException("Terminate requested");
			}
			ElementTuple current((*ptIterator));
			if (not tuplesFirstNEqual(current, domainElement) || sets.empty()) {
				if (not ctIterator.isAtEnd()) {
					const ElementTuple& certainly(*ctIterator);
					if (tuplesFirstNEqual(certainly, current)) {
						if (current != certainly) {
							Lit translation = translator()->translate(f, current);
							addUnitClause(-translation);
						}
						continue;
					} else if (tuplesFirstNSmaller(certainly, current)) {
						do {
							if(GlobalData::instance()->terminateRequested()){
								throw IdpException("Terminate requested");
							}
							++ctIterator;
						} while (not ctIterator.isAtEnd() && tuplesFirstNSmaller(*ctIterator, current));
						continue;
					}
				}
				sets.push_back(std::vector<int>(0));
				domainElement = current;
				domainElement.pop_back();
				outSortIterator = outSortTable->sortBegin();
			}

			Lit translation = translator()->translate(f, (*ptIterator));
			sets.back().push_back(translation);
		}

		//OLD CODE that might work for infintite domains... First we should find out what exactly is the meaning of the grounding in case of infinite domains...
		//FIXME implement for infinite domains
		/*std::vector<bool> weak;
		 for (auto it = tuples.begin(); it != tuples.end();) {
		 //NOTE: DE checks whether or not a tuple (x1,x2,x3,x4,y) starts with (x1,x2,x3,x4)
		 //IMPORTANT: tableiterator respects lexicographic ordering!!!

		 if (tuplesFirstNEqual(it->first, domainElement) && !sets.empty()) {
		 sets.back().push_back(it->second);
		 while (*outSortIterator != it->first.back()) {
		 ElementTuple temp = domainElement;
		 temp.push_back(*outSortIterator);
		 if (pt->contains(temp)) {
		 weak.back() = true;
		 break;
		 }
		 ++outSortIterator;
		 }
		 ++it;
		 if (not outSortIterator.isAtEnd()) {
		 ++outSortIterator;
		 }
		 } else {
		 if (not sets.empty() && not outSortIterator.isAtEnd()) {
		 weak.back() = true;
		 }
		 if (not ctIterator.isAtEnd()) {
		 const ElementTuple& tuple = *ctIterator;
		 if (tuplesFirstNEqual(tuple, it->first)) {
		 do {
		 if (it->first != tuple) {
		 addUnitClause(-(it->second));
		 std::cerr << "add unit clause " << -(it->second) << "\n";
		 }
		 ++it;
		 } while (it != tuples.end() && tuplesFirstNEqual(tuple, it->first));
		 continue;
		 } else if (tuplesFirstNSmaller(tuple, it->first)) {
		 do {
		 ++ctIterator;
		 } while (not ctIterator.isAtEnd() && tuplesFirstNSmaller(*ctIterator, it->first));
		 continue;
		 }
		 }
		 sets.push_back(std::vector<int>(0));
		 weak.push_back(false);
		 domainElement = it->first;
		 domainElement.pop_back();
		 outSortIterator = outSortTable->sortBegin();
		 }
		 }*/
		for (size_t s = 0; s < sets.size(); ++s) {
			if(GlobalData::instance()->terminateRequested()){
				throw IdpException("Terminate requested");
			}
			std::vector<double> lw(sets[s].size(), 1);
			int setnr = translator()->translateSet(sets[s], lw, { });
			int tseitin;
			if (f->partial() || (not outSortTable->finite())) {
				tseitin = translator()->translate(1, CompType::GT, AggFunction::CARD, setnr, TsType::IMPL);
			} else {
				tseitin = translator()->translate(1, CompType::EQ, AggFunction::CARD, setnr, TsType::IMPL);
			}
			addUnitClause(tseitin);
		}
	}
}

template<typename Policy>
void GroundTheory<Policy>::addFalseDefineds() {
	for (size_t n = 0; n < translator()->nbManagedSymbols(); ++n) {
		if(GlobalData::instance()->terminateRequested()){
			throw IdpException("Terminate requested");
		}
		PFSymbol* s = translator()->getManagedSymbol(n);
		auto it = _defined.find(s);
		if (it == _defined.end()) {
			continue;
		}
		const PredTable* pt = structure()->inter(s)->pt();
		for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
			if(GlobalData::instance()->terminateRequested()){
				throw IdpException("Terminate requested");
			}
			Lit translation = translator()->translate(s, (*ptIterator));
			if (it->second.find(translation) == it->second.end()) {
				addUnitClause(-translation);
				// TODO if not in translator, should make the structure more precise (do not add it to the grounding, that is useless)
			}
		}

	}
}
