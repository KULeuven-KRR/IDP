/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef GROUDING_GROUNDTHEORY_HPP_
#define GROUDING_GROUNDTHEORY_HPP_

#include "commontypes.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "vocabulary.hpp"
#include "structure.hpp"
#include "ecnf.hpp"

#include "inferences/grounding/GroundTermTranslator.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "visitors/TheoryVisitor.hpp"
#include "visitors/VisitorFriends.hpp"

#include <iostream>

template<class Policy>
class GroundTheory: public AbstractGroundTheory, public Policy {
	//ACCEPTBOTH(AbstractTheory)

	std::set<int> _printedtseitins; //!< Tseitin atoms produced by the translator that occur in the theory.
	std::set<int> _printedsets; //!< Set numbers produced by the translator that occur in the theory.
	std::set<int> _printedconstraints; //!< Atoms for which a connection to CP constraints are added.
	std::set<CPTerm*> _foldedterms;
	std::map<PFSymbol*, std::set<int> > _defined; //!< List of defined symbols and the heads which have a rule.

public:
	const int ID_FOR_UNDEFINED;

	// Constructors
	GroundTheory(AbstractStructure* str) :
			AbstractGroundTheory(str), ID_FOR_UNDEFINED(-1) {
		Policy::polStartTheory(translator());
	}
	GroundTheory(Vocabulary* voc, AbstractStructure* str) :
			AbstractGroundTheory(voc, str), ID_FOR_UNDEFINED(-1) {
		Policy::polStartTheory(translator());
	}

	virtual ~GroundTheory() {
	}

	// Mutators
	void add(Formula*) {
		Assert(false);
	}
	void add(Definition*) {
		Assert(false);
	}
	void add(FixpDef*) {
		Assert(false);
	}

	virtual void recursiveDelete() {
		Policy::polRecursiveDelete();
	}

	void closeTheory() {
		// TODO arbitrary values?
		// FIXME problem if a function does not occur in the theory/grounding! It might be arbitrary, but should still be a function?
		addFalseDefineds();
		addFuncConstraints();
		Policy::polEndTheory();
	}

	void add(const GroundClause& cl, bool skipfirst = false) {
		transformForAdd(cl, VIT_DISJ, ID_FOR_UNDEFINED, skipfirst);
		Policy::polAdd(cl);
	}

	void add(GroundDefinition* def) {
		for (auto i = def->begin(); i != def->end(); ++i) {
			if (sametypeid<PCGroundRule>(*(*i).second)) {
				auto rule = dynamic_cast<PCGroundRule*>((*i).second);
				transformForAdd(rule->body(), (rule->type() == RT_CONJ ? VIT_CONJ : VIT_DISJ), def->id());
				Policy::polAdd(def->id(), rule);
				notifyDefined(rule->head());
			} else {
				Assert(sametypeid<AggGroundRule>(*(*i).second));
				auto rule = dynamic_cast<AggGroundRule*>((*i).second);
				add(rule->setnr(), def->id(), (rule->aggtype() != AggFunction::CARD));
				Policy::polAdd(def->id(), rule);
				notifyDefined(rule->head());
			}
		}
	}

private:
	void notifyDefined(int inputatom) {
		if (not translator()->isInputAtom(inputatom)) {
			return;
		}
		PFSymbol* symbol = translator()->getSymbol(inputatom);
		auto it = _defined.find(symbol);
		if (it == _defined.end()) {
			it = _defined.insert(std::pair<PFSymbol*, std::set<int> >(symbol, std::set<int>())).first;
		}
		(*it).second.insert(inputatom);
	}

public:
	void add(GroundFixpDef*) {
		Assert(false);
		//TODO
	}

	void add(int tseitin, CPTsBody* body) {
		//TODO also add variables (in a separate container?)

		CPTsBody* foldedbody = new CPTsBody(body->type(), foldCPTerm(body->left()), body->comp(), body->right());
		//FIXME possible leaks!!

		Policy::polAdd(tseitin, foldedbody);
	}

	void add(int setnr, unsigned int defnr, bool weighted) {
		if (_printedsets.find(setnr) == _printedsets.end()) {
			_printedsets.insert(setnr);
			TsSet& tsset = translator()->groundset(setnr);
			transformForAdd(tsset.literals(), VIT_SET, defnr);
			std::vector<double> weights;
			if (weighted) {
				weights = tsset.weights();
			}
			Policy::polAdd(tsset, setnr, weighted);
		}
	}

	// FIXME very unclear invariants!
	void add(const Lit& tseitin, TsType type, const GroundClause& clause) {
		//std::vector<Lit> temp{clause[0]};
		// FIXME there might be an ID if it is a rule!
		transformForAdd(clause, VIT_DISJ, ID_FOR_UNDEFINED);
		Policy::polAdd(tseitin, type, clause);
	}

	void add(int head, AggTsBody* body) {
		add(body->setnr(), ID_FOR_UNDEFINED, (body->aggtype() != AggFunction::CARD));
		Policy::polAdd(head, body);
	}

	std::ostream& put(std::ostream& s) const {
		return Policy::polPut(s, translator(), termtranslator());
	}

	void accept(TheoryVisitor* v) const {
		v->visit(this);
	}

	AbstractTheory* accept(TheoryMutatingVisitor* v) {
		return v->visit(this);
	}

	void transformForAdd(const std::vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst = false) {
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

	CPTerm* foldCPTerm(CPTerm* cpterm) {
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
	void addFuncConstraints() {
		for (unsigned int n = 0; n < translator()->nbManagedSymbols(); ++n) {
			if(GlobalData::instance()->terminateRequested()){
				throw IdpException("Terminate requested");
			}
			auto pfs = translator()->getManagedSymbol(n);
			if (not sametypeid<Function>(*pfs)) {
				continue;
			}
			auto f = dynamic_cast<Function*>(pfs);

			if (translator()->getTuples(n).empty()) {
				continue;
			}

			FirstNElementsEqual equalDomain(f->arity());
			StrictWeakNTupleOrdering tuplesFirstNSmaller(f->arity());

			auto ct = structure()->inter(f)->graphInter()->ct();
			auto pt = structure()->inter(f)->graphInter()->pt();
			auto outSortTable = structure()->inter(f->outsort());
			if (not pt->finite()) {
				throw notyetimplemented("Cannot ground functions with infinite sorts.");
				//FIXME make this work for functions with infintie domains and/or infinite out-sorts.  Take a look at lower code...
			}

			ElementTuple domainElement, certainly;
			TableIterator ctIterator = ct->begin();
			bool begin = true, newdomain = true, testcertainly = false;
			litlist tupleset; // Set of tuples with same domain but different range, for which the cardinality should be 1
			for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
				CHECKTERMINATION
				ElementTuple current((*ptIterator));

				if(begin){
					domainElement = current;
					begin = false;
				}

				if(not equalDomain(current, domainElement)){
					newdomain = true;
					if(not testcertainly){
						addRangeConstraint(f, tupleset, outSortTable);
					}
					tupleset.clear();
				}

				if(newdomain){
					domainElement = current;
					newdomain = false;

					// CERTAINLY TRUE OPTIMIZATION / PROPAGATION: if some in the domain is certainly true, assert all other ones false
					while(not ctIterator.isAtEnd() && tuplesFirstNSmaller(*ctIterator, current)){
						CHECKTERMINATION
						++ctIterator;
					}
					if(not ctIterator.isAtEnd() && equalDomain(*ctIterator, current)){
						certainly = *ctIterator;
						testcertainly = true;
					}else{
						testcertainly = false;
					}
				}

				if (testcertainly && equalDomain(certainly, current)) {
					if (current != certainly) { // Assert current false
						Lit translation = translator()->translate(f, current);
						addUnitClause(-translation);
					}
					continue;
				}else{
					Lit translation = translator()->translate(f, current);
					tupleset.push_back(translation);
				}
			}
			if(not testcertainly){
				addRangeConstraint(f, tupleset, outSortTable);
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
			 std::clog << "add unit clause " << -(it->second) << "\n";
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
		}
	}

	void addFalseDefineds() {
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

private:
	void addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable){
		CHECKTERMINATION
		std::vector<double> lw(set.size(), 1);
		int setnr = translator()->translateSet(set, lw, { });
		int tseitin;
		if (f->partial() || (not outSortTable->finite())) {
			tseitin = translator()->translate(1, CompType::GEQ, AggFunction::CARD, setnr, TsType::IMPL);
		} else {
			tseitin = translator()->translate(1, CompType::EQ, AggFunction::CARD, setnr, TsType::IMPL);
		}
		addUnitClause(tseitin);
	}
};

#endif /* GROUDING_GROUNDTHEORY_HPP_ */
