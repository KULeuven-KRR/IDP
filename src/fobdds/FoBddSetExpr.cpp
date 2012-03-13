#include "FoBddSetExpr.hpp"
#include "FoBdd.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddTerm.hpp"
#include "common.hpp"

bool FOBDDSetExpr::containsDeBruijnIndex(unsigned int i) const {
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); it.operator ++()) {
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
const FOBDDSetExpr* FOBDDQuantSetExpr::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);

}
const FOBDDSetExpr* FOBDDEnumSetExpr::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

std::ostream& FOBDDQuantSetExpr::put(std::ostream& output) const {
	output << "QUANTSET WITH:";
	pushtab();
	output << nt() << "* Quantified variables of sorts:";
	pushtab();
	output << nt() << toString(_quantvarsorts);
	poptab();
	output << nt() << "* Formula:";
	pushtab();
	output << nt() << toString(_subformulas[0]);
	poptab();
	output << nt() << "* Term:";
	pushtab();
	output << nt() << toString(_subterms[0]);
	poptab();
	poptab();
	return output;
}
std::ostream& FOBDDEnumSetExpr::put(std::ostream& output) const {
	output << "ENUMSET WITH:";
	pushtab();
	for (int i = 0; i < _subformulas.size(); i++) {
		output << nt() << "* Formula:";
		pushtab();
		output << nt() << toString(_subformulas[i]);
		poptab();
		output << nt() << "* With term:";
		pushtab();
		output << nt() << toString(_subterms[i]);
		poptab();
	}
	poptab();
	return output;
}
