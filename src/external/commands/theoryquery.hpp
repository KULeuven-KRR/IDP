/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "inferences/definitionevaluation/AtomQuery.hpp"

typedef TypedInference<LIST(Query*, AbstractTheory* ,Structure*)> TheoryQueryInferenceBase;
class TheoryQueryInference: public TheoryQueryInferenceBase {
public:
	TheoryQueryInference()
			: TheoryQueryInferenceBase("theoryquery", "Generate all solutions to the given query for the given theory in the given structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<1>(args);
		Theory* theory = dynamic_cast<Theory*>(t);
		if(theory == NULL){
			Error::error("Can only query with a non-ground theory.");
			return nilarg();
		}
		auto result = AtomQuerying::doSolveAtomQuery(get<0>(args), theory, get<2>(args));
		return InternalArgument(result);
	}
};
