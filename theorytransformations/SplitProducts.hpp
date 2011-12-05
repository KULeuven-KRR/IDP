#ifndef SPLITPRODUCTS_HPP_
#define SPLITPRODUCTS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

/*
 * !< Serves for splitting product aggregates in aggregates conaining only positive values, is needed for the solver
 * If the solver is ever adapted, this can disappear
 * ALLWAYS call graphaggregates before calling this to ensure that all aggterms appear in an aggform
 * IMPORTANT: if you call this twice, it will keep on splitting.  There are no checks to see if something is allready split.
 */
class SplitProducts: public TheoryMutatingVisitor {
public:
	SplitProducts() {

	}

	Formula* execute(Formula* af);

protected:
	Formula* visit(AggForm* af);
};

#endif /* SPLITPRODUCTS_HPP_ */
