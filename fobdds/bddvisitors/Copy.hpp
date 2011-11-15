/************************************
	Copy.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COPY_HPP_
#define COPY_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"
#include "fobdds/FoBdd.hpp"

class Copy: public FOBDDVisitor {
private:
	FOBDDManager* _originalmanager;
	FOBDDManager* _copymanager;
	const FOBDDKernel* _kernel;
	const FOBDDArgument* _argument;
public:
	Copy(FOBDDManager* orig, FOBDDManager* copy) :
			FOBDDVisitor(orig), _originalmanager(orig), _copymanager(copy) {
	}

	void visit(const FOBDDVariable* var) {
		_argument = _copymanager->getVariable(var->variable());
	}

	void visit(const FOBDDDeBruijnIndex* index) {
		_argument = _copymanager->getDeBruijnIndex(index->sort(), index->index());
	}

	void visit(const FOBDDDomainTerm* term) {
		_argument = _copymanager->getDomainTerm(term->sort(), term->value());
	}

	void visit(const FOBDDFuncTerm* term) {
		std::vector<const FOBDDArgument*> newargs;
		for (auto it = term->args().cbegin(); it != term->args().cend(); ++it) {
			newargs.push_back(copy(*it));
		}
		_argument = _copymanager->getFuncTerm(term->func(), newargs);
	}

	void visit(const FOBDDQuantKernel* kernel) {
		auto newbdd = copy(kernel->bdd());
		_kernel = _copymanager->getQuantKernel(kernel->sort(), newbdd);
	}

	void visit(const FOBDDAtomKernel* kernel) {
		std::vector<const FOBDDArgument*> newargs;
		for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
			newargs.push_back(copy(*it));
		}
		_kernel = _copymanager->getAtomKernel(kernel->symbol(), kernel->type(), newargs);
	}

	const FOBDDArgument* copy(const FOBDDArgument* arg) {
		arg->accept(this);
		return _argument;
	}

	const FOBDDKernel* copy(const FOBDDKernel* kernel) {
		kernel->accept(this);
		return _kernel;
	}

	const FOBDD* copy(const FOBDD* bdd) {
		if (bdd == _originalmanager->truebdd()) {
			return _copymanager->truebdd();
		} else if (bdd == _originalmanager->falsebdd()) {
			return _copymanager->falsebdd();
		} else {
			auto falsebranch = copy(bdd->falsebranch());
			auto truebranch = copy(bdd->truebranch());
			auto kernel = copy(bdd->kernel());
			return _copymanager->ifthenelse(kernel, truebranch, falsebranch);
		}
	}
};

#endif /* COPY_HPP_ */
