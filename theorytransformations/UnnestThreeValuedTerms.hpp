/************************************
  	UnnestThreeValuedTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef REMOVETHREEVALUEDTERMS_HPP_
#define REMOVETHREEVALUEDTERMS_HPP_

#include <set>

#include "commontypes.hpp"
#include "theory.hpp"
#include "term.hpp"

#include "theorytransformations/UnnestTerms.hpp"

class Vocabulary;
class Variable;
class AbstractStructure;
class PFSymbol;

/**
 *	Non-recursively moves terms that are three-valued according to a given structure
 *	outside a given atom. The applied rewriting depends on the given context:
 *		- positive context:
 *			P(t) becomes	! x : t = x => P(x).
 *		- negative context:
 *			P(t) becomes	? x : t = x & P(x).
 *	The fact that the rewriting is non-recursive means that in the above example, term t
 *	can still contain terms that are three-valued according to the structure.
 *
 *	\param pf			the given atom
 *	\param str			the given structure
 *	\param Context	true iff we are in a positive context
 *	\param usingcp
 *
 *	\return The rewritten formula. If no rewriting was needed, it is the same pointer as pf.
 *	If rewriting was needed, pf can be deleted, but not recursively.
 *
 */
class UnnestThreeValuedTerms: public UnnestTerms {
private:
	AbstractStructure* 				_structure;
	bool 							_cpsupport;
	const std::set<const PFSymbol*> _cpsymbols;

public:
	UnnestThreeValuedTerms(AbstractStructure* str, Context context, bool cpsupport, const std::set<const PFSymbol*>& cpsymbols);

	Formula* traverse(PredForm* f);
	Rule* traverse(Rule* r);

protected:
	bool shouldMove(Term* t);

private:
	bool isCPSymbol(const PFSymbol* symbol) const;
};

#endif /* REMOVETHREEVALUEDTERMS_HPP_ */
