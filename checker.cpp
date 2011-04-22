/************************************
	checker.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "parseinfo.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "checker.hpp"

using namespace std;

InstanceChecker* CheckerFactory::create(PredInter* inter, bool ctpf, bool c) {
	PredTable* pt = ctpf ? (c ? inter->ct() : inter->pf()) : (c ? inter->cf() : inter->pt());
	if(pt->approxempty()) return new FalseInstanceChecker();
	else return new TableInstanceChecker(pt);
}

inline bool TableInstanceChecker::run(const ElementTuple& vd) const {
	return _table->contains(vd);
}
