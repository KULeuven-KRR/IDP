/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SIMPLIFY_HPP_
#define SIMPLIFY_HPP_

#include "commandinterface.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"

/**
 *	Class to test simplification of bdds.
 *  Gets a query as input and returns a string representation of the simplified bdd associated to the query.
 */
class SimplifyInference : public QueryBase {
public:
	SimplifyInference() : QueryBase("simplify", "Simplifies the given query if applicable, using bdds.") {
		setNameSpace(getInternalNamespaceName());

	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto q = get<0>(args);

		// Translate the query to a bdd
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		auto bdd = factory.turnIntoBdd(q->query());
		
		// Simplify the bdd
		auto simplifiedbdd = manager.simplify(bdd);

		// Return the result
		std::stringstream sstr;
		manager.put(sstr,simplifiedbdd);
		return InternalArgument(StringPointer(sstr.str()));
	}
};

#endif /* SIMPLIFY_HPP_ */
