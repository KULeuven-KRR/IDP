#include "ReplaceLTCSymbols.hpp"
#include "IncludeComponents.hpp"
#include "data/LTCData.hpp"
#include "data/StateVocInfo.hpp"
#include "utils/ListUtils.hpp"
#include "theory/TheoryUtils.hpp"

ReplaceLTCSymbols::ReplaceLTCSymbols(Vocabulary* ltcVoc, bool replaceByNext)
		: 	_ltcVoc(ltcVoc),
			_replacyeByNext(replaceByNext),
			_ltcVocInfo(LTCData::instance()->getStateVocInfo(ltcVoc)) {

}
ReplaceLTCSymbols::~ReplaceLTCSymbols() {

}

template<class T>
PFSymbol* ReplaceLTCSymbols::getNewSymbolAndSubterms(T* pf, std::vector<Term*>& newsubterms, PFSymbol* symbol) {
	PFSymbol* newSymbol = NULL;
	auto subterms = pf->subterms();
	for (auto subterm : subterms) {
		if (subterm->sort() != _ltcVocInfo->time) {
			newsubterms.push_back(subterm->accept(this));
		} else {
			if (isNextTime(subterm) || _replacyeByNext) {
				newSymbol = _ltcVocInfo->LTC2NextState.at(symbol);
			} else {
				if (isa<VarTerm>(*subterm)) {
					//Variable over sort Time
					newSymbol = _ltcVocInfo->LTC2State.at(symbol);
				} else if (isa<FuncTerm>(*subterm) && dynamic_cast<FuncTerm*>(subterm)->function() == _ltcVocInfo->start) {
					//Start symbol: initial state axiom
					newSymbol = _ltcVocInfo->LTC2State.at(symbol);
				} else {
					Error::LTC::unexpectedTimeTerm(toString(subterm), pf->pi());
				}

			}
			delete (subterm);
		}
	}
	Assert(newSymbol != NULL);
	Assert(newsubterms.size() == pf->subterms().size() - 1);
	return newSymbol;
}

Formula* ReplaceLTCSymbols::visit(PredForm* pf) {
	PFSymbol* symbol = pf->symbol();
	if (shouldReplace(symbol)) {
		std::vector<Term*> newsubterms;
		auto newSymbol = getNewSymbolAndSubterms(pf, newsubterms, symbol);
		auto result = new PredForm(pf->sign(), newSymbol, newsubterms, pf->pi());
		delete pf; //non-recursively (subterms are reused)
		return result;
	}
	auto timesort = _ltcVocInfo->time;
	if (symbol == timesort->pred()) {
		//Special case: explicit typechecking Time(Next(t)).
		Assert(pf->subterms().size() == 1);
		auto term = pf->subterms()[0];
		VarTerm* vt;
		FuncTerm* ft;
		switch (term->type()) {
		case TermType::VAR:
			vt = dynamic_cast<VarTerm*>(term);
			if (vt->var()->sort() != timesort) {
				Error::LTC::unexpectedTimeTerm(toString(vt), pf->pi());
			}
			break;
		case TermType::FUNC:
			ft = dynamic_cast<FuncTerm*>(term);
			if (ft->function() != _ltcVocInfo->start && ft->function() != _ltcVocInfo->next) {
				Error::LTC::unexpectedTimeTerm(toString(ft), pf->pi());

			}
			break;
		case TermType::AGG:
		case TermType::DOM:
			Error::LTC::unexpectedTimeTerm(toString(term), pf->pi());
			break;
		}
		return FormulaUtils::trueFormula();
	}
	return traverse(pf);
}

Term* ReplaceLTCSymbols::visit(FuncTerm* ft) {
	PFSymbol* symbol = ft->function();
	if (shouldReplace(symbol)) {
		std::vector<Term*> newsubterms;
		auto newSymbol = getNewSymbolAndSubterms(ft, newsubterms, symbol);
		Assert(isa<Function>(*newSymbol));
		auto newFunc = dynamic_cast<Function*>(newSymbol);
		auto result = new FuncTerm(newFunc, newsubterms, ft->pi());
		delete ft; //non-recursively (subterms are reused)
		return result;
	}
	return traverse(ft);
}

bool ReplaceLTCSymbols::shouldReplace(PFSymbol* pf) {
	return contains(_ltcVocInfo->LTC2State, pf);
}

bool ReplaceLTCSymbols::isNextTime(Term* t) {
	Assert(t->sort() == _ltcVocInfo->time);
	if (isa<FuncTerm>(*t)) {
		auto ft = dynamic_cast<FuncTerm*>(t);
		if (ft->function() == _ltcVocInfo->start) {
			return false;
		}
		Assert(ft->function() == _ltcVocInfo->next);
		auto subterm = ft->subterms()[0];
		if (not isa<VarTerm>(*subterm)) {
			Error::LTC::unexpectedTimeTerm(toString(t), t->pi());
		}
		return true;
	}
	return false;
}

