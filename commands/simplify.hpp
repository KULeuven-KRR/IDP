/************************************
	simplify.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SIMPLIFY_HPP_
#define SIMPLIFY_HPP_

#include <vector>
#include "commandinterface.hpp"

/**
 *	Class to test simplification of bdds. 
 *  Gets a query as input and returns a string representation of the simplified bdd associated to the query.
 */
class SimplifyInference : public Inference {
public:
	SimplifyInference() : Inference("simplify") {
		add(AT_QUERY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;

		// Translate the query to a bdd
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		const FOBDD* bdd = factory.run(q->query());
		
		// Simplify the bdd
		const FOBDD* simplifiedbdd = manager.simplify(bdd);

		// Return the result
		std::stringstream sstr;
		manager.put(sstr,simplifiedbdd);
		InternalArgument ia(StringPointer(sstr.str()));
		return ia;
	}
};

#endif /* SIMPLIFY_HPP_ */
