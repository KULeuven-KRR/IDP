/************************************
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef MODELEXPAND_HPP_
#define MODELEXPAND_HPP_

#include <vector>
#include <string>
#include <iostream>
#include "commandinterface.hpp"

#include "inferences/ModelExpansion.hpp"

class ModelExpandInference: public Inference {
public:
	ModelExpandInference() :
			Inference("modelexpand", false, true) {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory()->clone();
		AbstractStructure* structure = args[1].structure()->clone();
		GlobalData::instance()->setOptions(args[2].options());

		auto models = ModelExpansion::doModelExpansion(theory, structure, tracemonitor());

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		for (auto it = models.cbegin(); it != models.cend(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}
		if (GlobalData::instance()->getOptions()->getValue(BoolType::TRACE)) {
			InternalArgument randt;
			randt._type = AT_MULT;
			randt._value._table = new std::vector<InternalArgument>(1, result);
			InternalArgument trace;
			trace._type = AT_REGISTRY;
			//trace._value._string = monitor->index(); // FIXME what does this value mean exactly?
			randt._value._table->push_back(trace);
			result = randt;
		}

		return result;
	}
};

#endif
