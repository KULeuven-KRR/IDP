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

#include "PropagationDomain.hpp"
#include "structure/MainStructureComponents.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "utils/ListUtils.hpp"


FOPropDomain::FOPropDomain(const std::vector<Variable*>& vars)
		: _vars(vars) {
}
FOPropDomain::~FOPropDomain() {
}
const std::vector<Variable*>& FOPropDomain::vars() const {
	return _vars;
}

// A formula is valid iff _vars is a subset of the free variables of the formula
bool FOPropDomain::isValidFor(const Formula* f) const {
	return isSubset(_vars, f->freeVars());
}

FOPropBDDDomain::FOPropBDDDomain(const FOBDD* bdd, const std::vector<Variable*>& vars)
		: 	FOPropDomain(vars),
			_bdd(bdd) {
}
FOPropBDDDomain* FOPropBDDDomain::clone() const {
	return new FOPropBDDDomain(_bdd, _vars);
}
const FOBDD* FOPropBDDDomain::bdd() const {
	return _bdd;
}

// Valid iff bddvars subset vars subset freevars(f)
bool FOPropBDDDomain::isValidFor(const Formula* f, std::shared_ptr<FOBDDManager> manager) const {
	return isSubset(getFOVariables(variables(_bdd, manager)), _vars) && FOPropDomain::isValidFor(f);
}

void FOPropBDDDomain::put(std::ostream& stream) const{
	stream << print(_bdd);
}


FOPropTableDomain::FOPropTableDomain(PredTable* t, const std::vector<Variable*>& v)
		: 	FOPropDomain(v),
			_table(t) {
}
FOPropTableDomain* FOPropTableDomain::clone() const {
	return new FOPropTableDomain(new PredTable(_table->internTable(), _table->universe()), _vars);
}
PredTable* FOPropTableDomain::table() const {
	return _table;
}

