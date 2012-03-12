#include "FoBddSetExpr.hpp"
#include "FoBdd.hpp"
#include "fobdds/FoBddVisitor.hpp"

bool FOBDDSetExpr::containsDeBruijnIndex(unsigned int i) const {
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); it.operator ++()) {
		if ((*it)->containsDeBruijnIndex(i)) {
			return true;
		}
	}
	return false;
}
std::ostream& FOBDDSetExpr::put(std::ostream& output) const {
	output << "SOME FOBDDSETEXPR (TODO: better printing)" << nt();
	return output;
}

void FOBDDQuantSetExpr::accept(FOBDDVisitor* v ) const {
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
