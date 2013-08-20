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

#pragma once

#include <iostream>
#include "commandinterface.hpp"

#include "inferences/modelexpansion/UnsatCoreExtraction.hpp"

typedef TypedInference<LIST(AbstractTheory*, Structure*)> ModelExpandInferenceBase;
class UnsatCoreInference: public ModelExpandInferenceBase {
public:
	UnsatCoreInference()
			: ModelExpandInferenceBase("unsatcore",
					"Unsat core extraction on input level.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		auto core = UnsatCoreExtraction::extractCore(theory, get<1>(args));
		auto coretheory = new Theory("unsat_core", theory->vocabulary(), {});
		for(auto c: core){
			coretheory->add(c);
		}
		return {coretheory};
	}
};
