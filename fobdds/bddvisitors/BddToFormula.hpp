/************************************
	BddToFormula.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BDDTOFORMULA_HPP_
#define BDDTOFORMULA_HPP_

#include <vector>
#include <map>
#include <set>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddUtils.hpp"
#include "fobdds/FoBdd.hpp"

#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

/**
 * Given a bdd or a kernel, creates the associated formula.
 * Given a bddterm, creates the associated term.
 */
class BDDToFormula: public FOBDDVisitor {
private:
	Formula* _currformula;
	Term* _currterm;
	std::map<const FOBDDDeBruijnIndex*, Variable*> _dbrmapping;

	void reset(){
		_currformula = NULL;
		_currterm = NULL;
		_dbrmapping.clear();
	}

public:
	BDDToFormula(FOBDDManager* m) :
			FOBDDVisitor(m), _currformula(NULL), _currterm(NULL) {
		_dbrmapping.clear();
	}

	template<typename BddNode>
	Formula* createFormula(const BddNode* kernel) {
		reset();
		kernel->accept(this);
		return _currformula;
	}

	template<typename BddTerm>
	Term* createTerm(const BddTerm* arg) {
		reset();
		arg->accept(this);
		return _currterm;
	}

private:
	void visit(const FOBDDDeBruijnIndex* index) {
		auto it = _dbrmapping.find(index);
		Variable* v;
		if (it == _dbrmapping.cend()) {
			v = new Variable(index->sort());
			_dbrmapping[index] = v;
		} else {
			v = it->second;
		}
		_currterm = new VarTerm(v, TermParseInfo());
	}

	void visit(const FOBDDVariable* var) {
		_currterm = new VarTerm(var->variable(), TermParseInfo());
	}

	void visit(const FOBDDDomainTerm* dt) {
		_currterm = new DomainTerm(dt->sort(), dt->value(), TermParseInfo());
	}

	void visit(const FOBDDFuncTerm* ft) {
		std::vector<Term*> args;
		for (auto it = ft->args().cbegin(); it != ft->args().cend(); ++it) {
			(*it)->accept(this);
			args.push_back(_currterm);
		}
		_currterm = new FuncTerm(ft->func(), args, TermParseInfo());
	}

	void visit(const FOBDDAtomKernel* atom) {
		std::vector<Term*> args;
		for (auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
			(*it)->accept(this);
			args.push_back(_currterm);
		}
		switch (atom->type()) {
		case AtomKernelType::AKT_TWOVALUED:
			_currformula = new PredForm(SIGN::POS, atom->symbol(), args, FormulaParseInfo());
			break;
		case AtomKernelType::AKT_CT:
			_currformula = new PredForm(SIGN::POS, atom->symbol()->derivedSymbol(ST_CT), args, FormulaParseInfo());
			break;
		case AtomKernelType::AKT_CF:
			_currformula = new PredForm(SIGN::POS, atom->symbol()->derivedSymbol(ST_CF), args, FormulaParseInfo());
			break;
		}
	}

	void visit(const FOBDDQuantKernel* quantkernel) {
		std::map<const FOBDDDeBruijnIndex*, Variable*> savedmapping = _dbrmapping;
		_dbrmapping.clear();
		for (auto it = savedmapping.cbegin(); it != savedmapping.cend(); ++it){
			_dbrmapping[_manager->getDeBruijnIndex(it->first->sort(), it->first->index() + 1)] = it->second;
		}

		FOBDDVisitor::visit(quantkernel->bdd());

		auto quantvar = _dbrmapping[_manager->getDeBruijnIndex(quantkernel->sort(), NULL)];
		_dbrmapping = savedmapping;
		_currformula = new QuantForm(SIGN::POS, QUANT::EXIST, {quantvar}, _currformula, FormulaParseInfo());
	}

	void visit(const FOBDD* bdd) {
		if (_manager->isTruebdd(bdd)) {
			_currformula = FormulaUtils::trueFormula();
			return;
		}

		if (_manager->isFalsebdd(bdd)) {
			_currformula = FormulaUtils::falseFormula();
			return;
		}

		bdd->kernel()->accept(this);
		// NOTE afterwards, _currformula is a formula representing the kernel

		if (_manager->isFalsebdd(bdd->falsebranch())) {
			if(_manager->isTruebdd(bdd->truebranch())){
				return; // kernel is the whole formula
			}
			auto kernelformula = _currformula;
			FOBDDVisitor::visit(bdd->truebranch());
			auto branchformula = _currformula;
			_currformula = new BoolForm(SIGN::POS, true, kernelformula, branchformula, FormulaParseInfo());

		} else if (_manager->isFalsebdd(bdd->truebranch())) {
			_currformula->negate();

			if(_manager->isTruebdd(bdd->truebranch())){
				return; // \lnot kernel is the whole formula
			}

			auto kernelformula = _currformula;
			FOBDDVisitor::visit(bdd->falsebranch());
			auto branchformula = _currformula;
			_currformula = new BoolForm(SIGN::POS, true, kernelformula, branchformula, FormulaParseInfo());

		} else { // No branch is false
			auto kernelformula = _currformula;
			auto negkernelformula = kernelformula->clone();
			negkernelformula->negate();

			if (_manager->isTruebdd(bdd->falsebranch())) {
				FOBDDVisitor::visit(bdd->truebranch());
				auto bf = new BoolForm(SIGN::POS, true, kernelformula, _currformula, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, negkernelformula, bf, FormulaParseInfo());
			} else if (_manager->isTruebdd(bdd->truebranch())) {
				FOBDDVisitor::visit(bdd->falsebranch());
				auto bf = new BoolForm(SIGN::POS, true, negkernelformula, _currformula, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, kernelformula, bf, FormulaParseInfo());
			} else {
				FOBDDVisitor::visit(bdd->truebranch());
				auto  trueform = _currformula;
				FOBDDVisitor::visit(bdd->falsebranch());
				auto falseform = _currformula;
				auto bf1 = new BoolForm(SIGN::POS, true, kernelformula, trueform, FormulaParseInfo());
				auto bf2 = new BoolForm(SIGN::POS, true, negkernelformula, falseform, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, bf1, bf2, FormulaParseInfo());
			}
		}
	}
};

#endif /* BDDTOFORMULA_HPP_ */
