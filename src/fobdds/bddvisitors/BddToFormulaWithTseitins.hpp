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

#ifndef BDDTOTSEITINFORMULA554578_HPP_
#define BDDTOTSEITINFORMULA554578_HPP_
#include "BddToFormula.hpp"
#include "CountOccurences.hpp"
#include "IndexCollector.hpp"
//TODO: fix the parseinfo!
/**
 * Given a bdd or a kernel, creates the associated formula.
 * Makes use of _counter to count how many times certain bdds occur in the theory.
 * If the count is high, a Tseitin is created for this formula.
 * Finally, adds all tseitin-defining equivalences to the theory.
 */
class BDDToFOWithTseitins: public BDDToFO {
private:
	Vocabulary* _vocabulary;
	CountOccurences* _counter;
	std::map<const FOBDD*, Predicate*> _bddtseitins;
	std::map<const FOBDDKernel*, Predicate*> _kerneltseitins;
	std::set<const FOBDD*> _bddtseitinsWithoutConstraints;
	std::set<const FOBDDKernel*> _kerneltseitinsWithoutConstraints;
	std::set<const FOBDD*> _definedbddtseitins;
	std::set<const FOBDDKernel*> _definedkerneltseitins;
	bool _inDefinition;
	int _boundary; //Only formulas that occur more than this number are tseitinified. Initially, this is 1

public:

	BDDToFOWithTseitins(std::shared_ptr<FOBDDManager> m, CountOccurences* counter, Vocabulary* voc = NULL)
			: 	BDDToFO(m),
				_vocabulary(voc),
				_counter(counter),
				_inDefinition(false),
				_boundary(1) {
	}
	template<typename BddNode>
	Formula* createFormulaWithFreeVars(const BddNode* object, const fobddvarset& freebddvars) {
		reset();
		setDBRMappingToMatch(freebddvars);
		object->accept(this);
		return _currformula;
	}
	/**
	 * Forget all tseitin-symbols.
	 */
	void tseitinReset() {
		_bddtseitins.clear();
		_kerneltseitins.clear();
		_bddtseitinsWithoutConstraints.clear();
		_kerneltseitinsWithoutConstraints.clear();
	}

	void setVocabulary(Vocabulary* v) {
		_vocabulary = v;
	}

	Theory* addTseitinConstraints(Theory* t) {
		bool changed = true;
		while (changed) {
			changed = false;
			while (not _bddtseitinsWithoutConstraints.empty()) {
				changed = true;
				auto bdd = *(_bddtseitinsWithoutConstraints.cbegin());
				_bddtseitinsWithoutConstraints.erase(bdd);
				Assert(_bddtseitins.find(bdd) != _bddtseitins.cend());
				auto pred = _bddtseitins[bdd];
				auto newForm = tseitinFormula(pred, bdd);
				t->add(newForm);
			}
			while (not _kerneltseitinsWithoutConstraints.empty()) {
				changed = true;
				auto kernel = *(_kerneltseitinsWithoutConstraints.cbegin());
				_kerneltseitinsWithoutConstraints.erase(kernel);
				Assert(_kerneltseitins.find(kernel) != _kerneltseitins.cend());
				auto pred = _kerneltseitins[kernel];
				auto newForm = tseitinFormula(pred, kernel);
				t->add(newForm);
			}
		}
		return t;
	}

	void startDefinition() {
		_definedbddtseitins.clear();
		_definedkerneltseitins.clear();
		_inDefinition = true;
	}

	void finishDefinitionAndAddConstraints(Definition* def) {
		bool changed = true;
		while (changed) {
			changed = false;
			while (not _definedbddtseitins.empty()) {
				changed = true;
				auto bdd = *(_definedbddtseitins.cbegin());
				_definedbddtseitins.erase(bdd);
				_bddtseitinsWithoutConstraints.erase(bdd);
				Assert(_bddtseitins.find(bdd) != _bddtseitins.cend());
				auto pred = _bddtseitins[bdd];
				auto newRule = tseitinRule(pred, bdd);
				def->add(newRule);
			}
			while (not _definedkerneltseitins.empty()) {
				changed = true;
				auto kernel = *(_definedkerneltseitins.cbegin());
				_definedkerneltseitins.erase(kernel);
				_kerneltseitinsWithoutConstraints.erase(kernel);
				Assert(_kerneltseitins.find(kernel) != _kerneltseitins.cend());
				auto pred = _kerneltseitins[kernel];
				auto newRule = tseitinRule(pred, kernel);
				def->add(newRule);
			}
		}
		_inDefinition = false;
	}

private:
	template<typename BDDConstruct>
	Formula* tseitinFormula(Predicate* pred, const BDDConstruct* arg) {
		auto vars = VarUtils::makeNewVariables(pred->sorts());
		varset varsset(vars.cbegin(), vars.cend());
		setDBRMappingToMatch(vars, arg);
		createTseitinAtom(pred, arg);
		auto tseitinAtom = _currformula;
		auto backup = _boundary;
		_boundary = _counter->getCount(arg);
		arg->accept(this);
		_boundary = backup;
		auto tseitinDefinition = _currformula;
		auto tseitinEquiv = new EquivForm(SIGN::POS, tseitinAtom, tseitinDefinition, FormulaParseInfo());
		return new QuantForm(SIGN::POS, QUANT::UNIV, varsset, tseitinEquiv, FormulaParseInfo());
	}

	template<typename BDDConstruct>
	Rule* tseitinRule(Predicate* pred, const BDDConstruct* arg) {
		auto vars = VarUtils::makeNewVariables(pred->sorts());
		varset varsset(vars.cbegin(), vars.cend());
		setDBRMappingToMatch(vars, arg);
		createTseitinAtom(pred, arg);
		Assert(isa<PredForm>(*_currformula));
		auto tseitinAtom = dynamic_cast<PredForm*>(_currformula);
		auto backup = _boundary;
		_boundary = _counter->getCount(arg);
		arg->accept(this);
		_boundary = backup;
		auto tseitinDefinition = _currformula;
		return new Rule(varsset, tseitinAtom, tseitinDefinition, ParseInfo());
	}

	template<typename BDDConstruct>
	void setDBRMappingToMatch(std::vector<Variable*> vars, const BDDConstruct* arg) {
		_dbrmapping.clear();
		auto ic = IndexCollector(_manager);
		auto freeindices = ic.getVariables(arg);
		std::map<unsigned int, const FOBDDDeBruijnIndex*> inttoindex;
		for (auto it = freeindices.cbegin(); it != freeindices.cend(); it++) {
			inttoindex[(*it)->index()] = (*it);
		}
		size_t i = 0;
		for (auto it = inttoindex.cbegin(); it != inttoindex.cend(); it++, i++) {
			_dbrmapping[it->second] = vars[i];
		}
		Assert(vars.size() == _dbrmapping.size());

	}

	void setDBRMappingToMatch(const fobddvarset& bddvars) {
		_dbrmapping.clear();
		size_t i = 0;
		for (auto it = bddvars.cbegin(); it != bddvars.cend(); ++it, ++i) {
			auto dbrindex = _manager->getDeBruijnIndex((*it)->sort(), i);
			_dbrmapping[dbrindex] = (*it)->variable();
		}
	}
	template<typename FOBDDConstruct>
	void createTseitinAtom(Predicate* p, FOBDDConstruct bdd, bool negate = false) {
		auto ic = IndexCollector(_manager);
		auto freeindices = ic.getVariables(bdd);
		std::map<unsigned int, Variable*> inttovar;
		for (auto it = freeindices.cbegin(); it != freeindices.cend(); it++) {
			Assert(_dbrmapping.find(*it) != _dbrmapping.cend());
			inttovar[(*it)->index()] = _dbrmapping[*it];
		}

		Assert(p->arity() == inttovar.size());
		std::vector<Term*> terms(inttovar.size(), NULL);
		int i = 0;
		for (auto it = inttovar.cbegin(); it != inttovar.cend(); ++it, ++i) {
			//terms is an dbr-index sorted list of all relevant varterms
			terms[i] = new VarTerm(it->second, TermParseInfo());
		}

		_currformula = new PredForm(negate ? SIGN::NEG : SIGN::POS, p, terms, FormulaParseInfo());
	}

	template<typename FOBDDConstruct>
	std::vector<Sort*> getRelevantDbrMappingSorts(const FOBDDConstruct* bdd) {
		auto ic = IndexCollector(_manager);
		auto freeindices = ic.getVariables(bdd);
		std::map<unsigned int, const FOBDDDeBruijnIndex*> inttoindex;
		for (auto it = freeindices.cbegin(); it != freeindices.cend(); it++) {
			inttoindex[(*it)->index()] = (*it);
			Assert(_dbrmapping.find(*it) != _dbrmapping.cend());
		}

		std::vector<Sort*> sorts(freeindices.size(), NULL);

		int i = 0;
		for (auto it = inttoindex.cbegin(); it != inttoindex.cend(); ++it, ++i) {
			sorts[i] = it->second->sort();
		}
		//sorts is an dbr-index sorted list of all relevant sorts
		return sorts;
	}

	bool shouldTseitinifyFormula(const FOBDD* bdd) {
		if (_manager->isTruebdd(bdd) || _manager->isFalsebdd(bdd)) {
			return false;
		}
		return _manager->longestbranch(bdd) > 2 && _counter->getCount(bdd) > _boundary;
	}
	bool shouldTseitinifyFormula(const FOBDDKernel* kernel) {
		//NOTE: truekernels should not be appear here...
		return _manager->longestbranch(kernel) > 2 && _counter->getCount(kernel) > _boundary;
	}

	void tseitinifyFormula(const FOBDD* bdd) {
		auto negated = _manager->negation(bdd);
		auto res = _bddtseitins.find(negated);
		if (res != _bddtseitins.cend()) {
			createTseitinAtom((*res).second, bdd, true);
			if (_inDefinition) {
				_definedbddtseitins.insert(negated);
			}
			return;
		}
		if (_inDefinition) {
			_definedbddtseitins.insert(bdd);
		}
		res = _bddtseitins.find(bdd);
		if (res != _bddtseitins.cend()) {
			createTseitinAtom((*res).second, bdd);
			return;
		}
		//In this case, we need to create a new tseitin symbol.
		auto sorts = getRelevantDbrMappingSorts(bdd);

		Assert(_vocabulary !=NULL);
		auto tseitinsymbol = new Predicate(sorts);
		_vocabulary->add(tseitinsymbol);
		addTseitin(bdd, tseitinsymbol);
		createTseitinAtom(tseitinsymbol, bdd);
	}

	void tseitinifyFormula(const FOBDDKernel* kernel) {
		if (_inDefinition) {
			_definedkerneltseitins.insert(kernel);
		}
		auto res = _kerneltseitins.find(kernel);
		if (res != _kerneltseitins.cend()) {
			createTseitinAtom((*res).second, kernel);
			return;
		}

		//In this case, we need to create a new tseitin symbol.
		auto sorts = getRelevantDbrMappingSorts(kernel);

		Assert(_vocabulary !=NULL);
		auto tseitinsymbol = new Predicate(sorts);
		_vocabulary->add(tseitinsymbol);
		addTseitin(kernel, tseitinsymbol);
		createTseitinAtom(tseitinsymbol, kernel);
	}

	void addTseitin(const FOBDD* bdd, Predicate* tseitinsymbol) {
		_bddtseitins[bdd] = tseitinsymbol;
		_bddtseitinsWithoutConstraints.insert(bdd);
	}
	void addTseitin(const FOBDDKernel* kernel, Predicate* tseitinsymbol) {
		_kerneltseitins[kernel] = tseitinsymbol;
		_kerneltseitinsWithoutConstraints.insert(kernel);
	}

	void visit(const FOBDDAtomKernel* atom) {
		BDDToFO::visit(atom); //We shouldn't create tseitins for atom.
	}

	void visit(const FOBDDQuantKernel* quantkernel) {
		if (shouldTseitinifyFormula(quantkernel)) {
			tseitinifyFormula(quantkernel);
			return;
		}
		BDDToFO::visit(quantkernel);
	}

	void visit(const FOBDDAggKernel* fak) {
		if (shouldTseitinifyFormula(fak)) {
			tseitinifyFormula(fak);
			return;
		}
		BDDToFO::visit(fak);
	}

	void visit(const FOBDD* bdd) {
		if (shouldTseitinifyFormula(bdd)) {
			tseitinifyFormula(bdd);
			return;
		}
		BDDToFO::visit(bdd);
	}

}
;

#endif /* BDDTOTSEITINFORMULA554578_HPP_ */
