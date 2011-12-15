/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MODELEXPAND_HPP_
#define MODELEXPAND_HPP_

#include <vector>
#include <string>
#include <iostream>
#include "commandinterface.hpp"

#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"

class ModelExpandInference: public Inference {
public:
	ModelExpandInference() :
			Inference("modelexpand", false) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	// TODO trace is returned as the SECOND return value of the lua call
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory()->clone();
		AbstractStructure* structure = args[1].structure()->clone();
		GlobalData::instance()->setOptions(args[2].options());

		auto models = ModelExpansion::doModelExpansion(theory, structure, NULL);

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		for (auto it = models.cbegin(); it != models.cend(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}

		return result;
	}
};

class ModelExpandWithTraceInference: public Inference {
public:
	ModelExpandWithTraceInference() :
			Inference("modelexpand", false) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
		add(AT_TRACEMONITOR);
	}

	// TODO trace is returned as the SECOND return value of the lua call
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory()->clone();
		AbstractStructure* structure = args[1].structure()->clone();
		GlobalData::instance()->setOptions(args[2].options());
		LuaTraceMonitor* tracemonitor = args[3]._value._tracemonitor;

		auto models = ModelExpansion::doModelExpansion(theory, structure, tracemonitor);

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		for (auto it = models.cbegin(); it != models.cend(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}

		InternalArgument randt;
		randt._type = AT_MULT;
		randt._value._table = new std::vector<InternalArgument>(1, result);
		InternalArgument trace;
		trace._type = AT_REGISTRY;
		trace._value._string = tracemonitor->index();
		randt._value._table->push_back(trace);
		result = randt;

		return result;
	}
};

#endif
