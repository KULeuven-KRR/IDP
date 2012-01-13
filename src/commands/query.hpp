/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef CMD_QUERY_HPP_
#define CMD_QUERY_HPP_

#include "commandinterface.hpp"
#include "inferences/Query.hpp"

typedef TypedInference<LIST(Query*, AbstractStructure*)> QueryInferenceBase;
class QueryInference: public QueryInferenceBase {
public:
	QueryInference()
			: QueryInferenceBase("query", "Generate all solutions to the given query in the given structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = Querying::doSolveQuery(get<0>(args), get<1>(args));
		return InternalArgument(result);
	}
};

#endif /* CMD_QUERY_HPP_ */
