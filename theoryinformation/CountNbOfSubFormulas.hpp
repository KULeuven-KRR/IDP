/************************************
	CountNrOfSubformulas.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COUNTSUBFORMULAS_HPP_
#define COUNTSUBFORMULAS_HPP_

/**
 * Count the number of subformulas
 */
class CountNbOfSubFormulas: public TheoryVisitor {
private:
	int _result;
	void addAndTraverse(const Formula* f) {
		++_result;
		traverse(f);
	}
public:
	CountNbOfSubFormulas() :
			_result(0) {
	}
	int result() const {
		return _result;
	}
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
