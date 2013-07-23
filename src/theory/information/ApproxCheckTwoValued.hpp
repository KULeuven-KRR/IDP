/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef APPROXCHECKTWOVALUED_HPP_
#define APPROXCHECKTWOVALUED_HPP_

#include "common.hpp"
#include "visitors/TheoryVisitor.hpp"

class Structure;

class ApproxCheckTwoValued: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const Structure* _structure;
	bool _returnvalue;
public:
	template<typename T>
	bool execute(const T f, const Structure* str) {
		_structure = str;
		_returnvalue = true;
		f->accept(this);
		return _returnvalue;
	}
protected:
	void visit(const PredForm*);
	void visit(const FuncTerm*);
	void visit(const QuantSetExpr*);
};

#endif /* APPROXCHECKTWOVALUED_HPP_ */
