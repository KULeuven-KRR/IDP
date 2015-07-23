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
#include <internalargument.hpp>
#include "commandinterface.hpp"
#include "inferences/debugging/UnsatExtraction.hpp"
#include "inferences/debugging/MinimizeMarkers.hpp"


typedef TypedInference<LIST(bool, bool, AbstractTheory*, Structure*, Vocabulary*)> ModelExpandVocInferenceBase;
class UnsatCoreInference: public ModelExpandVocInferenceBase {
public:
	UnsatCoreInference()
			: ModelExpandVocInferenceBase("unsatcore",
									   "Returns a structure, subset of the given structure, that is unsatisfiable for the given theory.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto assumeTheo = get<0>(args);
		auto assumeStruc = get<1>(args);
		auto theory = get<2>(args);
		auto struc = get<3>(args);
		auto voc = get<4>(args);
		try{
			auto core = UnsatExtraction::extractCore(assumeTheo,assumeStruc,theory, struc, voc);
			InternalArgument output;
			output._type = AT_MULT;

			output._value._table = new std::vector<InternalArgument>();
			output._value._table->push_back(InternalArgument(core.first));
			if(core.second){
				output._value._table->push_back(InternalArgument(core.second));
			}

			return output;
		} catch(const AlreadySatisfiableException& e) {
			std::cerr << e.getMessage();
			return InternalArgument();
		}
	}
};

