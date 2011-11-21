/************************************
  	UnnestThreeValuedTerms.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theorytransformations/UnnestThreeValuedTerms.hpp"

#include "vocabulary.hpp"
#include "structure.hpp"

#include "utils/TheoryUtils.hpp"

using namespace std;

UnnestThreeValuedTerms::UnnestThreeValuedTerms(AbstractStructure* str, Context context, bool cpsupport, const std::set<const PFSymbol*>& cpsymbols) :
		UnnestTerms(context, str->vocabulary()), _structure(str), _cpsupport(cpsupport), _cpsymbols(cpsymbols) {
}

bool UnnestThreeValuedTerms::isCPSymbol(const PFSymbol* symbol) const {
	return (VocabularyUtils::isComparisonPredicate(symbol)) || (_cpsymbols.find(symbol) != _cpsymbols.cend());
}

bool UnnestThreeValuedTerms::shouldMove(Term* t) {
	if(getAllowedToUnnest()) {
		switch (t->type()) {
		case TT_FUNC: {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(t);
			Function* func = ft->function();
			FuncInter* finter = _structure->inter(func);
			return (not finter->approxTwoValued());
		}
		case TT_AGG: {
			//TODO include test for CPSymbols...
			AggTerm* at = dynamic_cast<AggTerm*>(t);
			return (not SetUtils::approxTwoValued(at->set(), _structure));
		}
		default:
			break;
		}
	}
	return false;
}

// BUG: allowed to unnest is default false???
Formula* UnnestThreeValuedTerms::traverse(PredForm* f) {
	Context savecontext = getContext();
	bool savemovecontext = getAllowedToUnnest();
	if(isNeg(f->sign())) {
		setContext(not getContext());
	}
	for(size_t n = 0; n < f->subterms().size(); ++n) {
		if (_cpsupport) {
			setAllowedToUnnest(not isCPSymbol(f->symbol()));
		}
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	return f;
}
