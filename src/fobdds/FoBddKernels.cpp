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

#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddAggKernel.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "fobdds/FoBddAggTerm.hpp"
#include "vocabulary/vocabulary.hpp"

using namespace std;

bool FOBDDQuantKernel::containsDeBruijnIndex(unsigned int index) const {
	return _bdd->containsDeBruijnIndex(index + 1);
}

bool FOBDDAtomKernel::containsDeBruijnIndex(unsigned int index) const {
	for (size_t n = 0; n < _args.size(); ++n) {
		if (_args[n]->containsDeBruijnIndex(index)) {
			return true;
		}
	}
	return false;
}
bool FOBDDAggKernel::containsDeBruijnIndex(unsigned int index) const {
	return _left->containsDeBruijnIndex(index) || _right->containsDeBruijnIndex(index);
}

void FOBDDAtomKernel::accept(FOBDDVisitor* v) const {

	v->visit(this);
}
void FOBDDQuantKernel::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDAggKernel::accept(FOBDDVisitor* v) const {
	v->visit(this);
}

std::ostream& FOBDDAtomKernel::put(std::ostream& output) const {
	output << print(_symbol);
	if (_type == AtomKernelType::AKT_CF) {
		output << "<cf>";
	} else if (_type == AtomKernelType::AKT_CT) {
		output << "<ct>";
	}
	if (isa<Predicate>(*_symbol)) {
		Assert(_args.size()==_symbol->nrSorts());
		if (_symbol->nrSorts() > 0) {
			output << "(";
			output << print(args(0));
			for (size_t n = 1; n < _symbol->nrSorts(); ++n) {
				output << ",";
				output << print(args(n));
			}
			output << ")";
		}
	} else {
		Assert(isa<Function>(*_symbol));
		if (_symbol->nrSorts() > 1) {
			output << "(";
			output << print(args(0));
			for (size_t n = 1; n < _symbol->nrSorts() - 1; ++n) {
				output << ",";
				output << print(args(n));
			}
			output << ")";
		}
		output << " = ";
		output << print(args(_symbol->nrSorts() - 1));
	}
	return output;
}
std::ostream& FOBDDQuantKernel::put(std::ostream& output) const {
	auto const v = new Variable(_sort);
	output << "EXISTS(" << print(v) << ") {";
	pushtab();
	auto bddmanager = FOBDDManager::createManager(false);
	auto newbdd=bddmanager->getBDDTryMaintainOrder(_bdd,_bdd->manager());
	auto nodebruijnbdd = bddmanager->substituteIndex(newbdd,bddmanager->getDeBruijnIndex(_sort,0),bddmanager->getVariable(v));
	output << "" << nt() << print(nodebruijnbdd);
	poptab();
	output << "" << nt();
	output << "}";
	delete(v);
	return output;
}
std::ostream& FOBDDAggKernel::put(std::ostream& output) const {
	output << print(_left) << " " << print(_comp) << " " << print(_right);
	return output;
}

std::ostream& TrueFOBDDKernel::put(std::ostream& output) const {
	output << "true";
	return output;
}
std::ostream& FalseFOBDDKernel::put(std::ostream& output) const {
	output << "false";
	return output;
}

const FOBDDKernel* FOBDDAtomKernel::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDKernel* FOBDDQuantKernel::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDKernel* FOBDDAggKernel::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
