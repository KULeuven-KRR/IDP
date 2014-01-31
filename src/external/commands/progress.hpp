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

#include "inferences/progression/Progression.hpp"
#include "inferences/progression/Invariants.hpp"
#include "lua/luaconnection.hpp"

InternalArgument transformInitDataToInternalArgument(const initData& info) {
	auto models = info._models;
	// Convert to internal arguments
	InternalArgument luamodels;
	luamodels._type = AT_TABLE;
	luamodels._value._table = new std::vector<InternalArgument>();
	addToGarbageCollection(luamodels._value._table);
	for (auto model : models) {
		luamodels._value._table->push_back(InternalArgument(model));
	}
	// Convert to internal arguments
	InternalArgument result;
	result._type = AT_TABLE;
	result._value._table = new std::vector<InternalArgument>();
	result._value._table->push_back(luamodels);
	result._value._table->push_back(InternalArgument(info._bistateTheo));
	result._value._table->push_back(InternalArgument(info._initTheo));
	result._value._table->push_back(InternalArgument(info._bistateVoc));
	result._value._table->push_back(InternalArgument(info._onestateVoc));
	return result;
}

typedef TypedInference<LIST(AbstractTheory*, Structure*)> ProgressInferenceBase;
class ProgressInference: public ProgressInferenceBase {
public:
	ProgressInference()
			: ProgressInferenceBase("progress", "Progress an LTC theory", false) {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto models = ProgressionInference::doProgression(get<0>(args), get<1>(args));

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		addToGarbageCollection(result._value._table);
		for (auto model : models) {
			result._value._table->push_back(InternalArgument(model));
		}

		return result;
	}
};
typedef TypedInference<LIST(AbstractTheory*, Structure*, Sort*, Function*, Function*)> InitInferenceBase;
class InitInference: public InitInferenceBase {
public:
	InitInference()
			: InitInferenceBase("initialise",
					"Initialise an LTC theory. Input: an LTC theory and the used Time type, Start and Next functions. Output: (depending on stdoptions.nbmodels) a number of models, and (arguments 2-5 respectively) the used bistate theory, initial theory, bistate vocabulary (V_bs) and single-state vocabulary (V_ss) (for in case you want to modify the progression behaviour yourself)",
					false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto info = InitialiseInference::doInitialisation(get<0>(args), get<1>(args), get<2>(args), get<3>(args), get<4>(args));
		auto result = transformInitDataToInternalArgument(info);
		return result;
	}

};

class InitInferenceNoTime: public ProgressInferenceBase {
public:
	InitInferenceNoTime()
			: ProgressInferenceBase("initialise",
					"Initialise an LTC theory. Input: an LTC theory. Output: (depending on stdoptions.nbmodels) a number of models, and (arguments 2-5 respectively) the used bistate theory, initial theory, bistate vocabulary and initial vocabulary (for in case you want to modify the progression behaviour yourself)",
					false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto info = InitialiseInference::doInitialisation(get<0>(args), get<1>(args), NULL, NULL, NULL);
		auto result = transformInitDataToInternalArgument(info);
		return result;
	}
};

typedef TypedInference<LIST(AbstractTheory*, AbstractTheory*, Structure*)> InvariantInferenceBase;
class InvariantInference : public InvariantInferenceBase{
public:
	InvariantInference()
			: InvariantInferenceBase("isinvariant",
					"Returns true if the second theory is (provable with the induction method) an invariant of the first LTC-theory in the context of the given finite structure ",
					false) {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = ProveInvariantInference::proveInvariant(get<0>(args), get<1>(args), get<2>(args));
		return InternalArgument(result);
	}
};

typedef TypedInference<LIST(AbstractTheory*, AbstractTheory*)> ProverInvariantInferenceBase;
class ProverInvariantInference : public ProverInvariantInferenceBase{
public:
	ProverInvariantInference()
			: ProverInvariantInferenceBase("isinvariant",
					"Returns true if the second theory is (provable with the induction method) an invariant of the first LTC-theory ",
					false) {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = ProveInvariantInference::proveInvariant(get<0>(args), get<1>(args), NULL);
		return InternalArgument(result);
	}
};
