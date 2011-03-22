/************************************
	checker.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "checker.hpp"
#include "structure.hpp"

using namespace std;

InstanceChecker* CheckerFactory::create(PredInter* inter, bool ctpf, bool c) {
	PredTable* pt = ctpf ? inter->ctpf() : inter->cfpt();
	bool cpt = ctpf ? inter->ct() : inter->cf();
	if(cpt == c) {
		if(pt->empty()) return new FalseInstanceChecker();
		else return new TableInstanceChecker(pt);
	}
	else {
		if(pt->empty()) return new TrueInstanceChecker();
		else return new InvTableInstanceChecker(pt);
	}
}

inline bool TableInstanceChecker::run(const vector<domelement>& vd) const {
	return _table->contains(vd);
}

inline bool InvTableInstanceChecker::run(const vector<domelement>& vd) const {
	return !(_table->contains(vd));
}
