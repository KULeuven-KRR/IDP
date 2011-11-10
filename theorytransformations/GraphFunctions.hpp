#ifndef GRAPHFUNCTIONS_HPP_
#define GRAPHFUNCTIONS_HPP_

#include "TheoryMutatingVisitor.hpp"

/***********************************
 Replace F(x) = y by P_F(x,y)
 ***********************************/

class GraphFunctions: public TheoryMutatingVisitor {
private:
	bool _recursive;
public:
	GraphFunctions(bool recursive = false) :
			_recursive(recursive) {
	}
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};

#endif /* GRAPHFUNCTIONS_HPP_ */
