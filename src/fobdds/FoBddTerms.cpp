/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "IncludeComponents.hpp"
#include "FoBddIndex.hpp"
#include "FoBddFuncTerm.hpp"
#include "FoBddAggTerm.hpp"
#include "FoBddDomainTerm.hpp"
#include "FoBddVariable.hpp"
#include "FoBddVisitor.hpp"
#include "FoBddManager.hpp"

using namespace std;

bool FOBDDFuncTerm::containsDeBruijnIndex(unsigned int index) const {
	for (size_t n = 0; n < _args.size(); ++n) {
		if (_args[n]->containsDeBruijnIndex(index)) {
			return true;
		}
	}
	return false;
}

void FOBDDVariable::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDDeBruijnIndex::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDDomainTerm::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDFuncTerm::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDAggTerm::accept(FOBDDVisitor* v) const {
	v->visit(this);
}

const FOBDDTerm* FOBDDVariable::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

const FOBDDTerm* FOBDDDeBruijnIndex::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

const FOBDDTerm* FOBDDDomainTerm::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

const FOBDDTerm* FOBDDFuncTerm::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

const FOBDDTerm* FOBDDAggTerm::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
std::ostream& FOBDDFuncTerm::put(std::ostream& output) const {
	output << toString(_function);
	if (_function->arity() > 0) {
		output << "(";
		output << toString(args(0));
		for (size_t n = 1; n < _function->arity(); ++n) {
			output << "," << toString(args(n));
		}
		output << ")";
	}
	return output;
}

std::ostream& FOBDDVariable::put(std::ostream& output) const {
	output << toString(_variable);
	return output;
}

std::ostream& FOBDDDeBruijnIndex::put(std::ostream& output) const {
	output << "<" << toString(_index) << ">[" << toString(_sort) << "]";
	return output;
}

std::ostream& FOBDDDomainTerm::put(std::ostream& output) const {
	output << toString(_value) << "[" << toString(_sort) << "]";
	return output;
}

std::ostream& FOBDDAggTerm::put(std::ostream& output) const {
	output << toString(_aggfunction)<<"{ "<<toString(_setexpr)<<nt()<<"}";
	return output;
}

Sort* FOBDDAggTerm::sort() const{
	return _setexpr->sort();
}


const FOBDDDomainTerm* add(FOBDDManager* manager, const FOBDDDomainTerm* d1, const FOBDDDomainTerm* d2) {
	auto addsort = SortUtils::resolve(d1->sort(), d2->sort());
	auto addfunc = Vocabulary::std()->func("+/2");
	addfunc = addfunc->disambiguate(std::vector<Sort*>(3, addsort), NULL);
	Assert(addfunc!=NULL);
	auto inter = addfunc->interpretation(NULL);
	auto result = inter->funcTable()->operator[]( { d1->value(), d2->value() });
	return manager->getDomainTerm(addsort, result);
}
