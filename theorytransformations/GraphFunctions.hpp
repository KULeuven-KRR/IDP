/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef GRAPHFUNCTIONS_HPP_
#define GRAPHFUNCTIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

/***********************************
	Replace F(x) = y by P_F(x,y)
***********************************/

class GraphFunctions: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _recursive;
public:
	template<typename T>
	T execute(T t, bool recursive = false){
		_recursive = recursive;
		return t->accept(this);
	}
protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
};

#endif /* GRAPHFUNCTIONS_HPP_ */
