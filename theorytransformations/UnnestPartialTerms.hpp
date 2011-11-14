/************************************
  	UnnestPartialTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef REMOVEPARTIALTERMS_HPP_
#define REMOVEPARTIALTERMS_HPP_

#include "theorytransformations/UnnestTerms.hpp"

class UnnestPartialTerms: public UnnestTerms {
public:
	UnnestPartialTerms(Context context, Vocabulary* voc) :
		UnnestTerms(context, voc) {
	}

	bool shouldMove(Term* t) {
		return (getAllowedToUnnest() && TermUtils::isPartial(t));
	}
};

#endif /* REMOVEPARTIALTERMS_HPP_ */
