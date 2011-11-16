/************************************
  	ApproxCheckTwoValued.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef APPROXCHECKTWOVALUED_HPP_
#define APPROXCHECKTWOVALUED_HPP_

#include <cassert>
#include "visitors/TheoryVisitor.hpp"

class AbstractStructure;

class ApproxCheckTwoValued : public TheoryVisitor {
	private:
		AbstractStructure*	_structure;
		bool				_returnvalue;
	public:
		ApproxCheckTwoValued(AbstractStructure* str) : _structure(str), _returnvalue(true) { }
		bool	returnvalue()	const { return _returnvalue;	}
		void	visit(const PredForm*);
		void	visit(const FuncTerm*);
		void 	visit(const SetExpr*) { /* TODO */ assert(false); }
};

#endif /* APPROXCHECKTWOVALUED_HPP_ */
