/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef BDDTOTSEITINFORMULA554578_HPP_
#define BDDTOTSEITINFORMULA554578_HPP_
#include "BddToFormula.hpp"
#include "CountOccurences.hpp"

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

	BDDToFOWithTseitins(FOBDDManager* m, CountOccurences* counter, Vocabulary* voc = NULL)
			: BDDToFO(m), _counter(counter), _boundary(1), _inDefinition(false), _vocabulary(voc) {
	}
	template<typename BddNode>
	Formula* createFormulaWithFreeVars(const BddNode* object, set<const FOBDDVariable*, CompareBDDVars> freebddvars) {
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
				auto res = _bddtseitins.find(bdd);
				Assert(res != _bddtseitins.cend());
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
				auto res = _bddtseitins.find(bdd);
				Assert(res != _bddtseitins.cend());
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
		std::set<Variable*> varsset(vars.cbegin(), vars.cend());
		setDBRMappingToMatch(vars);
		createTseitinAtom(pred);
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
		std::set<Variable*> varsset(vars.cbegin(), vars.cend());
		setDBRMappingToMatch(vars);
		createTseitinAtom(pred);
		Assert(sametypeid<PredForm>(*_currformula));
		auto tseitinAtom = dynamic_cast<PredForm*>(_currformula);
		auto backup = _boundary;
		_boundary = _counter->getCount(arg);
		arg->accept(this);
		_boundary = backup;
		auto tseitinDefinition = _currformula;
		return new Rule(varsset, tseitinAtom, tseitinDefinition, ParseInfo());
	}

	void setDBRMappingToMatch(std::vector<Variable*> vars) {
		_dbrmapping.clear();
		for (size_t i = 0; i < vars.size(); ++i) {
			auto dbrindex = _manager->getDeBruijnIndex(vars[i]->sort(), i);
			_dbrmapping[dbrindex] = vars[i];
		}
	}

	void setDBRMappingToMatch(set<const FOBDDVariable*, CompareBDDVars> bddvars) {
		_dbrmapping.clear();
		size_t i = 0;
		for (auto it = bddvars.cbegin(); it != bddvars.cend(); ++it, ++i) {
			auto dbrindex = _manager->getDeBruijnIndex((*it)->sort(), i);
			_dbrmapping[dbrindex] = (*it)->variable();
		}
	}

	void createTseitinAtom(Predicate* p, bool negate = false) {
		Assert(p->arity() == _dbrmapping.size());
		std::vector<Term*> terms(_dbrmapping.size(), NULL);
		for (auto it = _dbrmapping.cbegin(); it != _dbrmapping.cend(); ++it) {
			auto index = it->first;
			auto var = it->second;
			auto nb = index->index();
			terms[nb] = new VarTerm(var, TermParseInfo());
		}

		_currformula = new PredForm(negate ? SIGN::NEG : SIGN::POS, p, terms, FormulaParseInfo());
	}

	std::vector<Sort*> getDbrMappingSorts() {
		std::vector<Sort*> sorts(_dbrmapping.size(), NULL);
		for (auto it = _dbrmapping.cbegin(); it != _dbrmapping.cend(); ++it) {
			auto index = it->first;
			auto var = it->second;
			auto nb = index->index();
			sorts[nb] = var->sort();
		}
		return sorts;
	}

	bool shouldTseitinifyFormula(const FOBDD* bdd) {
		if (_manager->isTruebdd(bdd) || _manager->isFalsebdd(bdd)) {
			return false;
		}
		return _counter->getCount(bdd) > _boundary;
	}
	bool shouldTseitinifyFormula(const FOBDDKernel* kernel) {
		//NOTE: truekernels should not be appear here...
		return _counter->getCount(kernel) > _boundary;
	}

	void tseitinifyFormula(const FOBDD* bdd) {
		auto negated = _manager->negation(bdd);
		auto res = _bddtseitins.find(negated);
		if (res != _bddtseitins.cend()) {
			createTseitinAtom((*res).second, true);
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
			createTseitinAtom((*res).second);
			return;
		}

		//In this case, we need to create a new tseitin symbol.
		auto sorts = getDbrMappingSorts();

		//TODO: following should be improved!
		Assert(_vocabulary !=NULL);
		auto tseitinsymbol = new Predicate(sorts, true);
		_vocabulary->add(tseitinsymbol);
		addTseitin(bdd, tseitinsymbol);
		createTseitinAtom(tseitinsymbol);
	}

	void tseitinifyFormula(const FOBDDKernel* kernel) {
		if (_inDefinition) {
			_definedkerneltseitins.insert(kernel);
		}
		auto res = _kerneltseitins.find(kernel);
		if (res != _kerneltseitins.cend()) {
			createTseitinAtom((*res).second);
			return;
		}

		//In this case, we need to create a new tseitin symbol.
		auto sorts = getDbrMappingSorts();

		//TODO: following should be improved!
		stringstream ss;
		ss << "KernelTseitin_" << _kerneltseitins.size();
		auto tseitinsymbol = new Predicate(ss.str(), sorts, false);

		_kerneltseitins[kernel] = tseitinsymbol;
		createTseitinAtom((*res).second);
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
