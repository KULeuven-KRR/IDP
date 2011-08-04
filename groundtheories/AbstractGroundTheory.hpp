/************************************
	AbstractGroundTheory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ABSTRACTGROUNDTHEORY_HPP_
#define ABSTRACTGROUNDTHEORY_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <ostream>
#include <iostream>

#include "ecnf.hpp"
#include "ground.hpp"
#include "commontypes.hpp"

//FIXME definition numbers are passed directly to the solver. In future, solver input change might render this invalid

// Enumeration used in GroundTheory::transformForAdd
enum VIType { VIT_DISJ, VIT_CONJ, VIT_SET };

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory : public AbstractTheory {
private:
	AbstractStructure*		_structure;			//!< The ground theory may be partially reduced with respect to this structure.
	GroundTranslator*		_translator;		//!< Link between ground atoms and SAT-solver literals.
	GroundTermTranslator*	_termtranslator;	//!< Link between ground terms and CP-solver variables.
public:
	// Constructors
	AbstractGroundTheory(AbstractStructure* str) :
		AbstractTheory("",ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) { }

	AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) :
		AbstractTheory("",voc,ParseInfo()), _structure(str), _translator(new GroundTranslator()), _termtranslator(new GroundTermTranslator(str)) { }

	~AbstractGroundTheory() {
		delete(_structure);
		delete(_translator);
		delete(_termtranslator);
	}

	void 	addEmptyClause()	{ GroundClause c(0); add(c);	}
	void 	addUnitClause(int l){ GroundClause c(1,l); add(c);	}
	virtual void add(GroundClause& cl, bool skipfirst=false) = 0;
	virtual void add(GroundDefinition* d) = 0;
	virtual void add(GroundFixpDef*) = 0;
	virtual void add(int head, AggTsBody* body) = 0;
	virtual void add(int tseitin, CPTsBody* body) = 0;
	virtual void add(int setnr, unsigned int defnr, bool weighted) = 0;
	//virtual void add(unsigned int defnr, int tseitin, PCTsBody* body, bool recursive) = 0;
	//virtual void add(unsigned int defnr, int tseitin, AggTsBody* body, bool recursive) = 0;

	//NOTE: have to call these!
	//TODO check whether they are called correctly (currently in theorygrounder->run), but probably missing several usecases
	virtual void startTheory() = 0;
	virtual void closeTheory() = 0;

	// Inspectors
	GroundTranslator*		translator()		const { return _translator;			}
	GroundTermTranslator*	termtranslator()	const { return _termtranslator; 	}
	AbstractStructure*		structure()			const { return _structure;			}
	AbstractGroundTheory*	clone()				const { assert(false); return NULL;/* TODO */	}
};
template<class Policy>
class GroundTheory : public AbstractGroundTheory, public Policy {
	std::set<int>			_printedtseitins;		//!< Tseitin atoms produced by the translator that occur in the theory.
	std::set<int>			_printedsets;			//!< Set numbers produced by the translator that occur in the theory.
	std::set<int>			_printedconstraints;	//!< Atoms for which a connection to CP constraints are added.
	std::set<CPTerm*>		_foldedterms;
	std::map<PFSymbol*, std::set<int> >		_defined;	//list of defined symbols and the heads which have a rule

	/**
	 * GroundTheory<Policy>::transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst)
	 * DESCRIPTION
	 *		Adds defining rules for tseitin literals in a given vector of literals to the ground theory.
	 *		This method may apply unfolding which changes the given vector of literals.
	 * PARAMETERS
	 *		vi			- given vector of literals
	 *		vit			- indicates whether vi represents a disjunction, conjunction or set of literals
	 *		defnr		- number of the definition vi belongs to. Is NODEF when vi does not belong to a definition
	 *		skipfirst	- if true, the defining rule for the first literal is not added to the ground theory
	 *			is an OPTIMIZATION
	 * TODO
	 *		implement unfolding
	 */
	void transformForAdd(const std::vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst = false) {
		unsigned int n = 0;
		if(skipfirst) ++n;
		for(; n < vi.size(); ++n) {
			int atom = abs(vi[n]);
			if(translator()->isTseitin(atom) && _printedtseitins.find(atom) == _printedtseitins.end()) {
				_printedtseitins.insert(atom);
				TsBody* tsbody = translator()->tsbody(atom);
				if(typeid(*tsbody) == typeid(PCTsBody)) {
					PCTsBody* body = dynamic_cast<PCTsBody*>(tsbody);
					if(body->type() == TS_IMPL || body->type() == TS_EQ) {
						if(body->conj()) {
							for(unsigned int m = 0; m < body->size(); ++m) {
								std::vector<int> cl(2,-atom);
								cl[1] = body->literal(m);
								add(cl,true);
							}
						}
						else {
							std::vector<int> cl(body->size()+1,-atom);
							for(unsigned int m = 0; m < body->size(); ++m){
								cl[m+1] = body->literal(m);
							}
							add(cl,true);
						}
					}
					if(body->type() == TS_RIMPL || body->type() == TS_EQ) {
						if(body->conj()) {
							std::vector<int> cl(body->size()+1,atom);
							for(unsigned int m = 0; m < body->size(); ++m){
								cl[m+1] = -body->literal(m);
							}
							add(cl,true);
						}
						else {
							for(unsigned int m = 0; m < body->size(); ++m) {
								std::vector<int> cl(2,atom);
								cl[1] = -body->literal(m);
								add(cl,true);
							}
						}
					}
					if(body->type() == TS_RULE) {
						assert(defnr != ID_FOR_UNDEFINED);
						Policy::polAdd(defnr,new PCGroundRule(atom,body,true)); //TODO true (recursive) might not always be the case?
					}
				}
				else if(typeid(*tsbody) == typeid(AggTsBody)) {
					AggTsBody* body = dynamic_cast<AggTsBody*>(tsbody);
					if(body->type() == TS_RULE) {
						assert(defnr != ID_FOR_UNDEFINED);
						Policy::polAdd(defnr,new AggGroundRule(atom,body,true)); //TODO true (recursive) might not always be the case?
					}
					else {
						add(atom,body);
					}
				}
				else {
					assert(typeid(*tsbody) == typeid(CPTsBody));
					CPTsBody* body = dynamic_cast<CPTsBody*>(tsbody);
					if(body->type() == TS_RULE) {
						assert(false);
						//TODO Does this ever happen?
					}
					else {
						add(atom,body);
					}
				}
			}
		}
	}

	CPTerm* foldCPTerm(CPTerm* cpterm) {
		if(_foldedterms.find(cpterm) == _foldedterms.end()) {
			_foldedterms.insert(cpterm);
			if(typeid(*cpterm) == typeid(CPVarTerm)) {
				CPVarTerm* varterm = static_cast<CPVarTerm*>(cpterm);
				if(not termtranslator()->function(varterm->_varid)) {
					CPTsBody* cprelation = termtranslator()->cprelation(varterm->_varid);
					CPTerm* left = foldCPTerm(cprelation->left());
					if((typeid(*left) == typeid(CPSumTerm) || typeid(*left) == typeid(CPWSumTerm)) && cprelation->comp() == CT_EQ) {
						assert(cprelation->right()._isvarid && cprelation->right()._varid == varterm->_varid);
						return left;
					}
				}
			}
			else if(typeid(*cpterm) == typeid(CPSumTerm)) {
				CPSumTerm* sumterm = static_cast<CPSumTerm*>(cpterm);
				std::vector<VarId> newvarids;
				for(auto it = sumterm->_varids.begin(); it != sumterm->_varids.end(); ++it) {
					if(not termtranslator()->function(*it)) {
						CPTsBody* cprelation = termtranslator()->cprelation(*it);
						CPTerm* left = foldCPTerm(cprelation->left());
						if(typeid(*left) == typeid(CPSumTerm) && cprelation->comp() == CT_EQ) {
							CPSumTerm* subterm = static_cast<CPSumTerm*>(left);
							assert(cprelation->right()._isvarid && cprelation->right()._varid == *it);
							newvarids.insert(newvarids.end(),subterm->_varids.begin(),subterm->_varids.end());
						}
						//TODO Need to do something special in other cases?
						else newvarids.push_back(*it);
					}
					else newvarids.push_back(*it);
				}
				sumterm->_varids = newvarids;
			}
			else if(typeid(*cpterm) == typeid(CPWSumTerm)) {
				//CPWSumTerm* wsumterm = static_cast<CPWSumTerm*>(cpterm);
				//TODO
			}
		}
		return cpterm;
	}

public:
	const int ID_FOR_UNDEFINED;

	// Constructors
	GroundTheory(AbstractStructure* str): AbstractGroundTheory(str), ID_FOR_UNDEFINED(-1){}
	GroundTheory(Vocabulary* voc, AbstractStructure* str): AbstractGroundTheory(voc, str), ID_FOR_UNDEFINED(-1){}

	virtual	~GroundTheory(){}

	// Mutators
	void 	add(Formula* )		{ assert(false);	}
	void 	add(Definition* )	{ assert(false);	}
	void 	add(FixpDef* )		{ assert(false);	}

	virtual void recursiveDelete(){ Policy::polRecursiveDelete(); }

	void startTheory(){
		Policy::polStartTheory(translator());
	}
	void closeTheory(){
		// TODO arbitrary values?
		addFalseDefineds();
		addFuncConstraints();
		Policy::polEndTheory();
	}

	void add(GroundClause& cl, bool skipfirst) {
		transformForAdd(cl,VIT_DISJ,ID_FOR_UNDEFINED,skipfirst);
		Policy::polAdd(cl);
	}

	void add(GroundDefinition* def) {
		for(auto i=def->begin(); i!=def->end(); ++i){
			if(typeid(PCGroundRule*)==typeid((*i).second)){
				PCGroundRule* rule = dynamic_cast<PCGroundRule*>((*i).second);
				transformForAdd(rule->body(),(rule->type()==RT_CONJ ? VIT_CONJ : VIT_DISJ), def->id());
				notifyDefined(rule->head());
			}else{
				AggGroundRule* rule = dynamic_cast<AggGroundRule*>((*i).second);
				add(rule->setnr(),def->id(),(rule->aggtype() != AGG_CARD));
				notifyDefined(rule->head());
			}
		}
		Policy::polAdd(def);
	}

private:
	void notifyDefined(int tseitin){
		if(!translator()->hasSymbolFor(tseitin)){
			return;
		}
		PFSymbol* symbol = translator()->atom2symbol(tseitin);
		auto it = _defined.find(symbol);
		if(it==_defined.end()){
			it = _defined.insert(std::pair<PFSymbol*, std::set<int> >(symbol, std::set<int>())).first;
		}
		(*it).second.insert(tseitin);
	}

public:
	void add(GroundFixpDef*) {
		assert(false);
		//TODO
	}

	void add(int tseitin, CPTsBody* body) {
		//TODO also add variables (in a separate container?)

		CPTsBody* foldedbody = new CPTsBody(body->type(),foldCPTerm(body->left()),body->comp(),body->right());
		//FIXME possible leaks!!

		Policy::polAdd(tseitin, foldedbody);
	}

	void add(int setnr, unsigned int defnr, bool weighted) {
		if(_printedsets.find(setnr) == _printedsets.end()) {
			_printedsets.insert(setnr);
			TsSet& tsset = translator()->groundset(setnr);
			transformForAdd(tsset.literals(),VIT_SET,defnr);
			std::vector<double> weights;
			if(weighted) weights = tsset.weights();
			Policy::polAdd(tsset,setnr, weighted);
		}
	}

	void add(int head, AggTsBody* body) {
		add(body->setnr(),ID_FOR_UNDEFINED,(body->aggtype() != AGG_CARD));
		Policy::polAdd(head, body);
	}

	/**
	 *		Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
	 *		This method should be called before running the SAT solver and after grounding.
	 */
	void addFuncConstraints() {
		for(unsigned int n = 0; n < translator()->nbSymbols(); ++n) {
			PFSymbol* pfs = translator()->getSymbol(n);
			auto tuples = translator()->getTuples(n);
			if((typeid(*pfs)!=typeid(Function)) || tuples.empty()) {
				continue;
			}
			Function* f = dynamic_cast<Function*>(pfs);
			StrictWeakNTupleEquality de(f->arity());
			StrictWeakNTupleOrdering ds(f->arity());

			const PredTable* ct = structure()->inter(f)->graphinter()->ct();
			const PredTable* pt = structure()->inter(f)->graphinter()->pt();
			SortTable* st = structure()->inter(f->outsort());

			ElementTuple input(f->arity(),0);
			TableIterator tit = ct->begin();
			SortIterator sit = st->sortbegin();
			std::vector<std::vector<int> > sets;
			std::vector<bool> weak;
			for(auto it = tuples.begin(); it != tuples.end(); ) {
				if(de(it->first,input) && !sets.empty()) {
					sets.back().push_back(it->second);
					while(*sit != it->first.back()) {
						ElementTuple temp = input; temp.push_back(*sit);
						if(pt->contains(temp)) {
							weak.back() = true;
							break;
						}
						++sit;
					}
					++it;
					if(sit.hasNext()) ++sit;
				}
				else {
					if(!sets.empty() && sit.hasNext()) weak.back() = true;
					if(tit.hasNext()) {
						const ElementTuple& tuple = *tit;
						if(de(tuple,it->first)) {
							do {
								if(it->first != tuple) addUnitClause(-(it->second));
								++it;
							} while(it != tuples.end() && de(tuple,it->first));
							continue;
						}
						else if(ds(tuple,it->first)) {
							do { ++tit; } while(tit.hasNext() && ds(*tit,it->first));
							continue;
						}
					}
					sets.push_back(std::vector<int>(0));
					weak.push_back(false);
					input = it->first; input.pop_back();
					sit = st->sortbegin();
				}
			}
			for(unsigned int s = 0; s < sets.size(); ++s) {
				std::vector<double> lw(sets[s].size(),1);
				std::vector<double> tw(0);
				int setnr = translator()->translateSet(sets[s],lw,tw);
				int tseitin;
				if(f->partial() || !(st->finite()) || weak[s]) {
					tseitin = translator()->translate(1,'>',false,AGG_CARD,setnr,TS_IMPL);
				}
				else {
					tseitin = translator()->translate(1,'=',true,AGG_CARD,setnr,TS_IMPL);
				}
				addUnitClause(tseitin);
			}
		}
	}

	void addFalseDefineds() {
		for(unsigned int n = 0; n < translator()->nbSymbols(); ++n) {
			PFSymbol* s = translator()->getSymbol(n);
			auto it = _defined.find(s);
			if(it!=_defined.end()) {
				auto tuples = translator()->getTuples(n);
				for(auto jt = tuples.begin(); jt != tuples.end(); ++jt) {
					if(it->second.find(jt->second) == it->second.end()){
						addUnitClause(-jt->second);
					}
				}
			}
		}
	}

	std::ostream& put(std::ostream& s, unsigned int) const{
		return Policy::polPut(s, translator(), termtranslator());
	}

	std::string to_string() const{
		return Policy::polTo_string(translator(), termtranslator());
	}

	virtual void			accept(TheoryVisitor* v) const		{ v->visit(this);			}
	virtual AbstractTheory*	accept(TheoryMutatingVisitor* v)	{ return v->visit(this);	}

};

#endif /* ABSTRACTGROUNDTHEORY_HPP_ */
