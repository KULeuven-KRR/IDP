#ifndef COUNTSUBFORMULAS_HPP_
#define COUNTSUBFORMULAS_HPP_

/**
 * Count the number of subformulas
 */
class CountNbOfSubFormulas: public TheoryVisitor {
	VISITORFRIENDS()
private:
	int _result;
	void addAndTraverse(const Formula* f) {
		++_result;
		traverse(f);
	}
public:
	template<typename T>
	int execute(const T t){
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
