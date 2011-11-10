#ifndef CONTAINSFUNCTERMS_HPP_
#define CONTAINSFUNCTERMS_HPP_

#include <vector>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBdd.hpp"

/**
 * Checks whether the given term contains functions
 */
class ContainsFuncTerms: public FOBDDVisitor {
private:
	bool _result;
	void visit(const FOBDDFuncTerm*) {
		_result = true;
		return;
	}

public:
	ContainsFuncTerms(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}
	template<typename Tree>
	bool check(const Tree* arg) {
		_result = false;
		arg->accept(this);
		return _result;
	}
};

#endif /* CONTAINSFUNCTERMS_HPP_ */
