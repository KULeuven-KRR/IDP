
#include "PropagationDomain.hpp"
#include "structure/MainStructureComponents.hpp"


FOPropDomain::FOPropDomain(const std::vector<Variable*>& vars)
		: _vars(vars) {
}
FOPropDomain::~FOPropDomain() {
}
const std::vector<Variable*>& FOPropDomain::vars() const {
	return _vars;
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

