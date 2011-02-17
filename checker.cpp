#include "checker.hpp"

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
