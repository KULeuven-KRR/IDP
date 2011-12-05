#include "common.hpp"
#include "utils/TheoryUtils.hpp"
#include "theorytransformations/GraphFunctions.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

Formula* GraphFunctions::visit(PredForm* pf) {
	if (_recursive) {
		pf = dynamic_cast<PredForm*>(traverse(pf));
	}
	if (pf->symbol()->name() == "=/2") {
		PredForm* newpf = 0;
		if (typeid(*(pf->subterms()[0])) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[0]);
			vector<Term*> vt = ft->subterms();
			vt.push_back(pf->subterms()[1]);
			newpf = new PredForm(pf->sign(), ft->function(), vt, pf->pi().clone());
			delete (ft);
			delete (pf);
		} else if (typeid(*(pf->subterms()[1])) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterms()[1]);
			vector<Term*> vt = ft->subterms();
			vt.push_back(pf->subterms()[0]);
			newpf = new PredForm(pf->sign(), ft->function(), vt, pf->pi().clone());
			delete (ft);
			delete (pf);
		} else {
			newpf = pf;
		}
		return newpf;
	} else {
		return pf;
	}
}

Formula* GraphFunctions::visit(EqChainForm* ef) {
	auto finalpi = ef->pi();
	bool finalconj = ef->conj();
	if (_recursive) {
		ef = dynamic_cast<EqChainForm*>(traverse(ef));
	}
	set<unsigned int> removecomps;
	set<unsigned int> removeterms;
	vector<Formula*> graphs;
	for (size_t comppos = 0; comppos < ef->comps().size(); ++comppos) {
		CompType comparison = ef->comps()[comppos];
		if ((comparison == CompType::EQ && ef->conj()) || (comparison == CompType::NEQ && not ef->conj())) {
			if (typeid(*(ef->subterms()[comppos])) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos]);
				vector<Term*> vt = functerm->subterms();
				vt.push_back(ef->subterms()[comppos + 1]);
				graphs.push_back(new PredForm(ef->isConjWithSign() ? SIGN::POS : SIGN::NEG, functerm->function(), vt, FormulaParseInfo()));
				removecomps.insert(comppos);
				removeterms.insert(comppos);
			} else if (typeid(*(ef->subterms()[comppos + 1])) == typeid(FuncTerm)) {
				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos + 1]);
				vector<Term*> vt = functerm->subterms();
				vt.push_back(ef->subterms()[comppos]);
				graphs.push_back(new PredForm(ef->isConjWithSign() ? SIGN::POS : SIGN::NEG, functerm->function(), vt, FormulaParseInfo()));
				removecomps.insert(comppos);
				removeterms.insert(comppos + 1);
			}
		}
	}
	if (not graphs.empty()) {
		vector<Term*> newterms;
		vector<CompType> newcomps;
		for (size_t n = 0; n < ef->comps().size(); ++n) {
			if (removecomps.find(n) == removecomps.cend()) {
				newcomps.push_back(ef->comps()[n]);
			}
		}
		for (size_t n = 0; n < ef->subterms().size(); ++n) {
			if (removeterms.find(n) == removeterms.cend()) {
				newterms.push_back(ef->subterms()[n]);
			} else {
				delete (ef->subterms()[n]);
			}
		}
		EqChainForm* newef = new EqChainForm(ef->sign(), ef->conj(), newterms, newcomps, FormulaParseInfo());
		delete (ef);
		ef = newef;
	}

	bool remainingfuncterms = false;
	for (auto it = ef->subterms().cbegin(); it != ef->subterms().cend(); ++it) {
		if (typeid(*(*it)) == typeid(FuncTerm)) {
			remainingfuncterms = true;
			break;
		}
	}
	Formula* nf = 0;
	if (remainingfuncterms) {
		auto f = FormulaUtils::splitComparisonChains(ef);
		nf = f->accept(this);
	} else {
		nf = ef;
	}

	if (graphs.empty()) {
		return nf;
	} else {
		graphs.push_back(nf);
		return new BoolForm(SIGN::POS, isConj(nf->sign(), finalconj), graphs, finalpi.clone());
	}
}
