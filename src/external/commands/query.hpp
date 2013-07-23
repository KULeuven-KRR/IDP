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

#ifndef CMD_QUERY_HPP_
#define CMD_QUERY_HPP_

#include "commandinterface.hpp"
#include "inferences/querying/Query.hpp"

typedef TypedInference<LIST(Query*, Structure*)> QueryInferenceBase;
class QueryInference: public QueryInferenceBase {
public:
	QueryInference()
			: QueryInferenceBase("query", "Generate all solutions to the given query in the given structure.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = Querying::doSolveQuery(get<0>(args), get<1>(args));
		return InternalArgument(result);
	}
};

#endif /* CMD_QUERY_HPP_ */
