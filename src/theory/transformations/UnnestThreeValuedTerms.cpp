/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "UnnestThreeValuedTerms.hpp"
#include "IncludeComponents.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

bool UnnestThreeValuedTerms::shouldMove(Term* t) {
	if (getAllowedToUnnest()) {
		switch (t->type()) {
		case TT_FUNC: {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(t);
			Function* func = ft->function();
			FuncInter* finter = _structure->inter(func);
			return not finter->approxTwoValued()
				and not (_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(ft,_vocabulary));
		}
		case TT_AGG: {
			AggTerm* at = dynamic_cast<AggTerm*>(t);
			return not SetUtils::approxTwoValued(at->set(), _structure)
				and not (_cpsupport and getAllowedToLeave() and CPSupport::eligibleForCP(at,_structure));
		}
		case TT_VAR:
		case TT_DOM:
			break;
		}
	}
	return false;
}

//Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
//	bool saveAllowedToLeave = getAllowedToLeave();
//	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(predform,_vocabulary));
//	auto result = UnnestTerms::visit(predform);
//	setAllowedToLeave(saveAllowedToLeave);
//	return result;
//}

template<typename T>
Formula* UnnestTerms::doRewrite(T origformula) {
	auto rewrittenformula = rewrite(origformula);
	if (rewrittenformula == origformula) {
		return origformula;
	} else {
		return rewrittenformula->accept(this);
	}
}

Formula* UnnestThreeValuedTerms::visit(PredForm* predform) {
	bool savemovecontext = getAllowedToUnnest();
	bool saveAllowedToLeave = getAllowedToLeave();
	setAllowedToLeave(_cpsupport and CPSupport::eligibleForCP(predform,_vocabulary));

// Special treatment for (in)equalities: possibly only one side needs to be moved
	bool moveonlyleft = false;
	bool moveonlyright = false;
	if (VocabularyUtils::isComparisonPredicate(predform->symbol())) {
		auto leftterm = predform->subterms()[0];
		auto rightterm = predform->subterms()[1];
		if (leftterm->type() == TT_AGG) {
			moveonlyright = true;
		} else if (rightterm->type() == TT_AGG) {
			moveonlyleft = true;
		} else if (predform->symbol()->name() == "=/2") {
			moveonlyright = (leftterm->type() != TT_VAR) && (rightterm->type() != TT_VAR);
		} else {
			setAllowedToUnnest(true);
		}

		if (predform->symbol()->name() == "=/2") {
			auto leftsort = leftterm->sort();
			auto rightsort = rightterm->sort();
			if (SortUtils::isSubsort(leftsort, rightsort)) {
				_chosenVarSort = leftsort;
			} else {
				_chosenVarSort = rightsort;
			}
		}
	} else {
		setAllowedToUnnest(true);
	}
	// Traverse the atom
	Formula* newf = predform;
	if (moveonlyleft) {
		predform->subterm(1, predform->subterms()[1]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(0, predform->subterms()[0]->accept(this));
	} else if (moveonlyright) {
		predform->subterm(0, predform->subterms()[0]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(1, predform->subterms()[1]->accept(this));
	} else {
		newf = traverse(predform);
	}

	_chosenVarSort = NULL;
	setAllowedToLeave(saveAllowedToLeave);
	setAllowedToUnnest(savemovecontext);
	return doRewrite(newf);
}
