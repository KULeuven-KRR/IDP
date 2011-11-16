/************************************
	VariableCollector.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VARIABLECOLLECTOR_HPP_
#define VARIABLECOLLECTOR_HPP_

#include <set>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddKernel.hpp"

/**
 * Class to obtain all variables of a bdd.
 */
class VariableCollector: public FOBDDVisitor {
private:
	std::set<const FOBDDVariable*> _result;
public:
	VariableCollector(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}
	void visit(const FOBDDVariable* v) {
		_result.insert(v);
	}

	template<typename Node>
	const std::set<const FOBDDVariable*>& getVariables(const Node* arg) {
		_result.clear();
		arg->accept(this);
		return _result;
	}
};

#endif /* VARIABLECOLLECTOR_HPP_ */
