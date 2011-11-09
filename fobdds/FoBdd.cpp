#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBddVisitor.hpp"

bool FOBDD::containsDeBruijnIndex(unsigned int index) const {
	if (_kernel->containsDeBruijnIndex(index)) {
		return true;
	} else if (_falsebranch && _falsebranch->containsDeBruijnIndex(index)) {
		return true;
	} else if (_truebranch && _truebranch->containsDeBruijnIndex(index)) {
		return true;
	} else {
		return false;
	}
}

void FOBDD::accept(FOBDDVisitor* visitor) const{
	visitor->visit(this);
}
