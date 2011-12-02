#include "common.hpp"
#include "theorytransformations/PushQuantifications.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"

using namespace std;

std::set<Variable*> clone(const std::set<Variable*>& origvars){
	std::set<Variable*> newvars;
	for (auto it = origvars.cbegin(); it != origvars.cend(); ++it) {
		auto v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newvars.insert(v);
	}
	return newvars;
}

Formula* PushQuantifications::visit(QuantForm* qf) {
	if (not sametypeid<BoolForm>(*(qf->subformula()))) {
		return TheoryMutatingVisitor::visit(qf);
	}

	auto bf = dynamic_cast<BoolForm*>(qf->subformula());
	auto u = qf->isUnivWithSign() ? QUANT::UNIV : QUANT::EXIST;
	auto c = bf->isConjWithSign();

	if ((u == QUANT::UNIV && not bf->isConjWithSign()) || (u == QUANT::EXIST && bf->isConjWithSign())) {
		return TheoryMutatingVisitor::visit(qf);
	}

	auto s = (qf->sign() == bf->sign()) ? SIGN::POS : SIGN::NEG;
	vector<Formula*> vf;
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		vf.push_back(new QuantForm(s, u, clone(qf->quantVars()), (*it)->clone(), FormulaParseInfo()));
	}
	auto nbf = new BoolForm(SIGN::POS, c, vf, (qf->pi()).clone());
	qf->subformula()->recursiveDelete();
	delete (qf);
	return nbf->accept(this);
}
