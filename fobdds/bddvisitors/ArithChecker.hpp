/************************************
 ArithChecker.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef ARITHCHECKER_HPP_
#define ARITHCHECKER_HPP_

class ArithChecker: public FOBDDVisitor {
private:
	bool _result;
public:
	ArithChecker(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	bool check(const FOBDDKernel* k) {
		_result = true;
		k->accept(this);
		return _result;
	}
	bool check(const FOBDDArgument* a) {
		_result = true;
		a->accept(this);
		return _result;
	}

	void visit(const FOBDDAtomKernel* kernel) {
		for (auto it = kernel->args().cbegin(); it != kernel->args().cend(); ++it) {
			if (_result) {
				(*it)->accept(this);
			}
		}
		_result = _result && Vocabulary::std()->contains(kernel->symbol());
	}

	void visit(const FOBDDQuantKernel*) {
		_result = false;
	}

	void visit(const FOBDDVariable* variable) {
		_result = _result && SortUtils::isSubsort(variable->sort(), VocabularyUtils::floatsort());
	}

	void visit(const FOBDDDeBruijnIndex* index) {
		_result = _result && SortUtils::isSubsort(index->sort(), VocabularyUtils::floatsort());
	}

	void visit(const FOBDDDomainTerm* domainterm) {
		_result = _result && SortUtils::isSubsort(domainterm->sort(), VocabularyUtils::floatsort());
	}

	void visit(const FOBDDFuncTerm* functerm) {
		for (auto it = functerm->args().cbegin(); it != functerm->args().cend(); ++it) {
			if (_result) {
				(*it)->accept(this);
			}
		}
		_result = _result && Vocabulary::std()->contains(functerm->func());
	}

};

#endif /* ARITHCHECKER_HPP_ */
