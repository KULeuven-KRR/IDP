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

#include "FoBddSetExpr.hpp"
#include "FoBdd.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "common.hpp"

bool FOBDDQuantSetExpr::containsDeBruijnIndex(unsigned int i) const {
	auto j = i + _quantvarsorts.size();
	return (_subformula->containsDeBruijnIndex(j) || _subterm->containsDeBruijnIndex(j));
}
bool FOBDDEnumSetExpr::containsDeBruijnIndex(unsigned int i) const {
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); it.operator ++()) {
		if ((*it)->containsDeBruijnIndex(i)) {
			return true;
		}
	}
	return false;
}

void FOBDDQuantSetExpr::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDEnumSetExpr::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
const FOBDDQuantSetExpr* FOBDDQuantSetExpr::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);

}
const FOBDDEnumSetExpr* FOBDDEnumSetExpr::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

std::ostream& FOBDDQuantSetExpr::put(std::ostream& output) const {
	output << "QUANTSET WITH:";
	pushtab();
	output << nt() << "* Quantified variables of sorts:";
	pushtab();
	output << nt() << print(_quantvarsorts);
	poptab();
	output << nt() << "* Formula:";
	pushtab();
	output << nt() << print(_subformula);
	poptab();
	output << nt() << "* Term:";
	pushtab();
	output << nt() << print(_subterm);
	poptab();
	poptab();
	return output;
}
std::ostream& FOBDDEnumSetExpr::put(std::ostream& output) const {
	output << "ENUMSET WITH:";
	pushtab();
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); it++) {
		output << nt() << "- " << print(*it);
	}
	poptab();
	return output;
}
