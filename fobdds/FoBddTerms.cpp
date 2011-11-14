#include <cassert>
#include <vector>
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"

#include "vocabulary.hpp"
#include "structure.hpp"

using namespace std;

bool FOBDDFuncTerm::containsDeBruijnIndex(unsigned int index) const {
	for(size_t n = 0; n < _args.size(); ++n) {
		if(_args[n]->containsDeBruijnIndex(index)) { return true; }
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

const FOBDDArgument* FOBDDVariable::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDArgument* FOBDDDeBruijnIndex::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDArgument* FOBDDDomainTerm::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDArgument* FOBDDFuncTerm::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}

const FOBDDDomainTerm* add(FOBDDManager* manager, const FOBDDDomainTerm* d1, const FOBDDDomainTerm* d2) {
	auto addsort = SortUtils::resolve(d1->sort(), d2->sort());
	auto addfunc = Vocabulary::std()->func("+/2");
	addfunc = addfunc->disambiguate(std::vector<Sort*>(3, addsort), NULL);
	assert(addfunc!=NULL);
	auto inter = addfunc->interpretation(NULL);
	auto result = inter->funcTable()->operator[]({d1->value(), d2->value()});
	return manager->getDomainTerm(addsort, result);
}
