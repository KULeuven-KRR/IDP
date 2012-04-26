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
 * Given a bddterm, creates the associated term.
 */
class BDDToFOWithTseitins: public BDDToFO {
private:
	CountOccurences* _counter;
	std::map<const FOBDD*, Predicate*> _bddtseitins;
	std::map<const FOBDDKernel*, Predicate*> _kerneltseitins;
	std::vector<const FOBDD*> _bddtseitinsWithoutConstraints;
	std::vector<const FOBDDKernel*> _kerneltseitinsWithoutConstraints;
	int _boundary; //Only formulas that occur more than this number are tseitinified. Initially, this is 2 (since many formulas occur twice e.g. in equivalences).

public:
	BDDToFOWithTseitins(FOBDDManager* m, CountOccurences* counter)
			: BDDToFO(m), _counter(counter), _boundary(2) {
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

	Theory* addTseitinConstraints(Theory* t) {
		Assert(_bddtseitinsWithoutConstraints.size()==_bddtseitins.size());
		Assert(_kerneltseitinsWithoutConstraints.size()==_kerneltseitins.size());

		bool changed = true;
		while (changed) {
			changed = false;
			while (not _bddtseitinsWithoutConstraints.empty()) {
				changed = true;
				auto bdd = _bddtseitinsWithoutConstraints[_bddtseitinsWithoutConstraints.size() - 1];
				Assert(_bddtseitins.find(bdd) != _bddtseitins.cend());
				auto pred = _bddtseitins[bdd];
				auto newForm = tseitinFormula(pred, bdd);
				_bddtseitinsWithoutConstraints.pop_back();
				t->add(newForm);
			}
			while (not _kerneltseitinsWithoutConstraints.empty()) {
				changed = true;
				auto kernel = _kerneltseitinsWithoutConstraints[_kerneltseitinsWithoutConstraints.size() - 1];
				Assert(_kerneltseitins.find(kernel) != _kerneltseitins.cend());
				auto pred = _kerneltseitins[kernel];
				auto newForm = tseitinFormula(pred, kernel);
				_kerneltseitinsWithoutConstraints.pop_back();
				t->add(newForm);
			}
		}
		return t;
	}
private:
	template<typename BDDConstruct>
	Formula* tseitinFormula(Predicate* pred, const BDDConstruct* arg) {
		auto vars = VarUtils::makeNewVariables(pred->sorts());
		std::set<Variable*> varsset(vars.cbegin(), vars.cend());
		setDBRMappingToMatch(vars);
		createTseitinAtom(pred);
		auto tseitinAtom = _currformula;
		_boundary = _counter->getCount(arg);
		arg->accept(this);
		auto tseitinDefinition = _currformula;
		auto tseitinEquiv = new EquivForm(SIGN::POS, tseitinAtom, tseitinDefinition, FormulaParseInfo());
		return new QuantForm(SIGN::POS, QUANT::UNIV, varsset, tseitinEquiv, FormulaParseInfo());
	}

	void setDBRMappingToMatch(std::vector<Variable*> vars) {
		_dbrmapping.clear();
		for (size_t i = 0; i < vars.size(); ++i) {
			auto dbrindex = _manager->getDeBruijnIndex(vars[i]->sort(), i);
			_dbrmapping[dbrindex] = vars[i];
		}
	}

	void createTseitinAtom(Predicate* p) {
		Assert(p->arity() == _dbrmapping.size());
		std::vector<Term*> terms(_dbrmapping.size(), NULL);
		for (auto it = _dbrmapping.cbegin(); it != _dbrmapping.cend(); ++it) {
			auto index = it->first;
			auto var = it->second;
			auto nb = index->index();
			terms[nb] = new VarTerm(var, TermParseInfo());
		}
		_currformula = new PredForm(SIGN::POS, p, terms, FormulaParseInfo());
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

	bool tseitinifyFormula(const FOBDD* bdd) {
		if (not (_counter->getCount(bdd) > _boundary)) {
			return false;
			//TODO
			BDDToFO::visit(bdd);
		}
		auto res = _bddtseitins.find(bdd);

		if (res != _bddtseitins.cend()) {
			createTseitinAtom((*res).second);
		}

		//In this case, we need to create a new tseitin symbol.
		auto sorts = getDbrMappingSorts();

		//TODO: following should be improved!
		stringstream ss;
		ss << "Tseitin_" << _bddtseitins.size();
		auto tseitinsymbol = new Predicate(ss.str(), sorts, false);

		addTseitin(bdd, tseitinsymbol);
		createTseitinAtom((*res).second);
		return true;
	}
	bool tseitinifyFormula(const FOBDDKernel* kernel) {
		if (not (_counter->getCount(kernel) > _boundary)) {
			return false;
			//TODO
			kernel->accept(this);
		}
		auto res = _kerneltseitins.find(kernel);
		if (res != _kerneltseitins.cend()) {
			createTseitinAtom((*res).second);
		}

		//In this case, we need to create a new tseitin symbol.
		auto sorts = getDbrMappingSorts();

		//TODO: following should be improved!
		stringstream ss;
		ss << "KernelTseitin_" << _kerneltseitins.size();
		auto tseitinsymbol = new Predicate(ss.str(), sorts, false);

		_kerneltseitins[kernel] = tseitinsymbol;
		createTseitinAtom((*res).second);
		return true;
	}

	void addTseitin(const FOBDD* bdd, Predicate* tseitinsymbol) {
		_bddtseitins[bdd] = tseitinsymbol;
		_bddtseitinsWithoutConstraints.push_back(bdd);
	}
	void addTseitin(const FOBDDKernel* kernel, Predicate* tseitinsymbol) {
		_kerneltseitins[kernel] = tseitinsymbol;
		_kerneltseitinsWithoutConstraints.push_back(kernel);
	}

	void visit(const FOBDDAtomKernel* atom) {
		BDDToFO::visit(atom); //We shouldn't create tseitins for atom.
	}

	void visit(const FOBDDQuantKernel* quantkernel) {
		if (tseitinifyFormula(quantkernel)) {
			return;
		}
		BDDToFO::visit(quantkernel);
	}

	void visit(const FOBDDAggKernel* fak) {
		if (tseitinifyFormula(fak)) {
			return;
		}
		BDDToFO::visit(fak);
	}

	void visit(const FOBDD* bdd) {
		if (tseitinifyFormula(bdd)) {
			return;
		}
		BDDToFO::visit(bdd);
	}

};

#endif /* BDDTOTSEITINFORMULA554578_HPP_ */
