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

#include "inferences/debugging/UnsatCoreExtraction.hpp"

typedef TypedInference<LIST(AbstractTheory*, Structure*)> ModelExpandInferenceBase;
class UnsatCoreInference: public ModelExpandInferenceBase {
public:
	UnsatCoreInference()
			: ModelExpandInferenceBase("unsatcore",
					"Returns a theory, subset of the given theory, that is unsatisfiable in the given structure.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		auto corePtr = UnsatCoreExtraction::extractCore(theory, get<1>(args));
		if(corePtr){
			auto coretheory = new Theory("unsat_core", theory->vocabulary(), {});
			auto core = *corePtr;
			for(auto c: core){
				coretheory->add(c);
			}
			return {coretheory};
		}else{
			return InternalArgument();
		}
		
	}
};
