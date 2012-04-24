/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "GraphFuncsAndAggs.hpp"

#include "theory/TheoryUtils.hpp"
#include "IncludeComponents.hpp"

using namespace std;

CompType GraphFuncsAndAggs::getCompType(const PredForm* pf) const {
	Assert(VocabularyUtils::isComparisonPredicate(pf->symbol()));
	if (pf->symbol()->name() == "=/2") {
		return isPos(pf->sign()) ? CompType::EQ : CompType::NEQ;
	} else if (pf->symbol()->name() == "</2") {
		return isPos(pf->sign()) ? CompType::LT : CompType::GEQ;
	} else {
		Assert(pf->symbol()->name() == ">/2");
		return isPos(pf->sign()) ? CompType::GT : CompType::LEQ;
	}
}

PredForm* GraphFuncsAndAggs::makeFuncGraph(SIGN sign, Term* functerm, Term* valueterm, const FormulaParseInfo& pi) const {
	Assert(functerm->type() == TT_FUNC);
	Assert(valueterm->type() != TT_FUNC && valueterm->type() != TT_AGG);
	FuncTerm* ft = dynamic_cast<FuncTerm*>(functerm);
	auto vt = ft->subterms();
	vt.push_back(valueterm);
	PredForm* funcgraph = new PredForm(sign, ft->function(), vt, pi);
	delete (ft);
	return funcgraph;
}

AggForm* GraphFuncsAndAggs::makeAggForm(Term* valueterm, CompType comp, Term* aggterm, const FormulaParseInfo& pi) const {
	Assert(aggterm->type() == TT_AGG);
	Assert(valueterm->type() != TT_FUNC && valueterm->type() != TT_AGG);
	AggTerm* at = dynamic_cast<AggTerm*>(aggterm);
	AggForm* aggform = new AggForm(SIGN::POS, valueterm, comp, at, pi);
	return aggform;
}

Formula* GraphFuncsAndAggs::visit(PredForm* pf) {
	if (VocabularyUtils::isComparisonPredicate(pf->symbol())) {

		Term* subterm1 = pf->subterms()[0];
		Term* subterm2 = pf->subterms()[1];

		bool eligibleForCP = _cpsupport && VocabularyUtils::isIntComparisonPredicate(pf->symbol(),_vocabulary)
				&& CPSupport::eligibleForCP(subterm1,_structure) && CPSupport::eligibleForCP(subterm2,_structure);

		if ((subterm1->type() == TT_FUNC || subterm1->type() == TT_AGG)
				&& (subterm2->type() == TT_FUNC || subterm2->type() == TT_AGG)
				&& not eligibleForCP) {
			auto splitformula = FormulaUtils::unnestFuncsAndAggs(pf, _structure, _context);
			return splitformula->accept(this);
		}

		Formula* newformula = NULL;

		if (pf->symbol()->name() == "=/2" && not eligibleForCP) {
			if (subterm1->type() == TT_FUNC) {
				newformula = makeFuncGraph(pf->sign(), subterm1, subterm2, pf->pi());
				delete (pf);
			} else if (subterm2->type() == TT_FUNC) {
				newformula = makeFuncGraph(pf->sign(), subterm2, subterm1, pf->pi());
				delete (pf);
			}
			if (newformula != NULL) {
				return traverse(newformula);
			}
		}

		if (subterm1->type() == TT_AGG  && not eligibleForCP) {
			newformula = makeAggForm(subterm2, invertComp(getCompType(pf)), subterm1, pf->pi());
			delete (pf);
		} else if (subterm2->type() == TT_AGG) {
			newformula = makeAggForm(subterm1, getCompType(pf), subterm2, pf->pi());
			delete (pf);
		}
		if (newformula != NULL) {
			return traverse(newformula);
		}
	}

	return traverse(pf);
}

Formula* GraphFuncsAndAggs::visit(EqChainForm* ef) {
	bool needsSplit = false;
	for (size_t n = 0; n < ef->subterms().size(); ++n) {
		TermType subtermtype = ef->subterms()[n]->type();
		if (subtermtype == TT_FUNC || subtermtype == TT_AGG) {
			needsSplit = true;
			break;
		}
	}
	if (needsSplit) {
		auto newformula = FormulaUtils::splitComparisonChains(ef);
		return newformula->accept(this);
	} else {
		return traverse(ef);
	}
}

//XXX Old code which does something smart for EqChainForms XXX
//Formula* GraphFunctions::visit(EqChainForm* ef) {
//	auto finalpi = ef->pi();
//	bool finalconj = ef->conj();
//	if (_recursive) {
//		ef = dynamic_cast<EqChainForm*>(traverse(ef));
//	}
//	set<unsigned int> removecomps;
//	set<unsigned int> removeterms;
//	vector<Formula*> graphs;
//	for (size_t comppos = 0; comppos < ef->comps().size(); ++comppos) {
//		CompType comparison = ef->comps()[comppos];
//		if ((comparison == CompType::EQ && ef->conj()) || (comparison == CompType::NEQ && not ef->conj())) {
//			if (typeid(*(ef->subterms()[comppos])) == typeid(FuncTerm)) {
//				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos]);
//				vector<Term*> vt = functerm->subterms();
//				vt.push_back(ef->subterms()[comppos + 1]);
//				graphs.push_back(new PredForm(ef->isConjWithSign() ? SIGN::POS : SIGN::NEG, functerm->function(), vt, FormulaParseInfo()));
//				removecomps.insert(comppos);
//				removeterms.insert(comppos);
//			} else if (typeid(*(ef->subterms()[comppos + 1])) == typeid(FuncTerm)) {
//				FuncTerm* functerm = dynamic_cast<FuncTerm*>(ef->subterms()[comppos + 1]);
//				vector<Term*> vt = functerm->subterms();
//				vt.push_back(ef->subterms()[comppos]);
//				graphs.push_back(new PredForm(ef->isConjWithSign() ? SIGN::POS : SIGN::NEG, functerm->function(), vt, FormulaParseInfo()));
//				removecomps.insert(comppos);
//				removeterms.insert(comppos + 1);
//			}
//		}
//	}
//	if (not graphs.empty()) {
//		vector<Term*> newterms;
//		vector<CompType> newcomps;
//		for (size_t n = 0; n < ef->comps().size(); ++n) {
//			if (removecomps.find(n) == removecomps.cend()) {
//				newcomps.push_back(ef->comps()[n]);
//			}
//		}
//		for (size_t n = 0; n < ef->subterms().size(); ++n) {
//			if (removeterms.find(n) == removeterms.cend()) {
//				newterms.push_back(ef->subterms()[n]);
//			} else {
//				delete (ef->subterms()[n]);
//			}
//		}
//		EqChainForm* newef = new EqChainForm(ef->sign(), ef->conj(), newterms, newcomps, FormulaParseInfo());
//		delete (ef);
//		ef = newef;
//	}
//
//	bool remainingfuncterms = false;
//	for (auto it = ef->subterms().cbegin(); it != ef->subterms().cend(); ++it) {
//		if (typeid(*(*it)) == typeid(FuncTerm)) {
//			remainingfuncterms = true;
//			break;
//		}
//	}
//	Formula* nf = 0;
//	if (remainingfuncterms) {
//		auto f = FormulaUtils::splitComparisonChains(ef);
//		nf = f->accept(this);
//	} else {
//		nf = ef;
//	}
//
//	if (graphs.empty()) {
//		return nf;
//	} else {
//		graphs.push_back(nf);
//		return new BoolForm(SIGN::POS, isConj(nf->sign(), finalconj), graphs, finalpi.clone());
//	}
//}
