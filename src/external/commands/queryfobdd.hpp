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

#ifndef CMD_QUERYFOBDD_HPP_
#define CMD_QUERYFOBDD_HPP_

#include "commandinterface.hpp"
#include "inferences/querying/Query.hpp"

typedef TypedInference<LIST(const FOBDD*, Structure*)> FOBDDInferenceBase;
class QueryFOBDDInference: public FOBDDInferenceBase {
public:
	QueryFOBDDInference()
			: FOBDDInferenceBase("queryfobdd", "Generate all solutions to the given fobdd in the given structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = Querying::doSolveBDDQuery(get<0>(args), get<1>(args));
		return InternalArgument(result);
	}
};

#endif /* CMD_QUERYFOBDD_HPP_ */
