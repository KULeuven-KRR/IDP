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
		try{
			auto theory = get<0>(args);
			auto core = UnsatCoreExtraction::extractCore(theory, get<1>(args));
			if(core.succes){
				auto coretheory = new Theory("unsat_core", theory->vocabulary(), {});
				for(auto c: core.core){
					coretheory->add(c);
				}
				return InternalArgument(coretheory);
			}else{
				return InternalArgument();
			}
		} catch(const IdpException& e) {
			std::cerr << e.getMessage();
			return InternalArgument();
		}

		
	}
};

#include "inferences/debugging/UnsatStructureExtraction.hpp"
typedef TypedInference<LIST(AbstractTheory*, Structure*, Vocabulary*)> ModelExpandVocInferenceBase;
class UnsatStructureInference: public ModelExpandVocInferenceBase {
public:
	UnsatStructureInference()
			: ModelExpandVocInferenceBase("unsatstructure",
									   "Returns a structure, subset of the given structure, that is unsatisfiable for the given theory.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		auto struc = get<1>(args);
		auto voc = get<2>(args);
		try{
			auto core = UnsatStructureExtraction::extractStructure(theory, struc, voc);
			if(core.succes){
				return InternalArgument(core.core);
			}else{
				return InternalArgument();
			}
		} catch(const IdpException& e) {
				std::cerr << e.getMessage();
				return InternalArgument();
		}
	}
};

