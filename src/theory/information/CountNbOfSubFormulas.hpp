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

#ifndef COUNTSUBFORMULAS_HPP_
#define COUNTSUBFORMULAS_HPP_

/**
 * Count the number of subformulas
 */
class CountNbOfSubFormulas: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	int _result;
	void addAndTraverse(const Formula* f) {
		++_result;
		traverse(f);
	}
public:
	template<typename T>
	int execute(const T t) {
		_result = 0;
		t->accept(this);
		return _result;
	}
protected:
	void visit(const PredForm* f) {
		addAndTraverse(f);
	}
	void visit(const BoolForm* f) {
		addAndTraverse(f);
	}
	void visit(const EqChainForm* f) {
		addAndTraverse(f);
	}
	void visit(const QuantForm* f) {
		addAndTraverse(f);
	}
	void visit(const EquivForm* f) {
		addAndTraverse(f);
	}
	void visit(const AggForm* f) {
		addAndTraverse(f);
	}
};

#endif /* COUNTSUBFORMULAS_HPP_ */
