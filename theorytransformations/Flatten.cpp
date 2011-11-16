/************************************
  	Flatten.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <vector>

#include "theorytransformations/Flatten.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"

using namespace std;

Formula* Flatten::visit(BoolForm* bf) {
	vector<Formula*> newsubf;
	traverse(bf);
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		if (typeid(*(*it)) == typeid(BoolForm)) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(*it);
			if ((bf->conj() == sbf->conj()) && isPos(sbf->sign())) {
				for (auto jt = sbf->subformulas().cbegin(); jt != sbf->subformulas().cend(); ++jt) {
					newsubf.push_back(*jt);
				}
				delete (sbf);
			} else {
				newsubf.push_back(*it);
			}
		} else {
			newsubf.push_back(*it);
		}
	}
	bf->subformulas(newsubf);
	return bf;
}

Formula* Flatten::visit(QuantForm* qf) {
	traverse(qf);
	if (typeid(*(qf->subformula())) == typeid(QuantForm)) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subformula());
		if ((qf->quant() == sqf->quant()) && isPos(sqf->sign())) {
			qf->subformula(sqf->subformula());
			for (auto it = sqf->quantVars().cbegin(); it != sqf->quantVars().cend(); ++it) {
				qf->add(*it);
			}
			delete (sqf);
		}
	}
	return qf;
}
