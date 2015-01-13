/* 
 * File:   modeliteration.hpp
 * Author: rupsbant
 *
 * Created on November 18, 2014, 2:35 PM
 */

#pragma once

#include <iostream>
#include "commandinterface.hpp"

//#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/modelIteration/ModelIterator.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "utils/LogAction.hpp"
#include "lua/luaconnection.hpp"


// TODO trace is returned as the SECOND return value of the lua call
InternalArgument createIteratorCommand(AbstractTheory* theory, Structure* structure, Vocabulary* outputvoc) {
	LuaTraceMonitor* tracer = NULL;
	if (getOption(BoolType::TRACE)) {
		tracer = LuaConnection::getLuaTraceMonitor();
	}
	auto iterator =  createIterator(theory, structure, outputvoc, tracer);
        auto wrap = new WrapModelIterator(iterator);
        InternalArgument ia(wrap);
        iterator->init();
        return ia;
}

typedef TypedInference<LIST(AbstractTheory*, Structure*)> ModelIterationInferenceBase;
class ModelIterationInference: public ModelIterationInferenceBase {
public:
	ModelIterationInference()
			: ModelIterationInferenceBase("createMXIterator",
					"Create an iterator generating models that satisfy the theory and which are more precise than the given structure.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
                auto ret = createIteratorCommand(get<0>(args), get<1>(args), NULL);
		return ret;
	}
};

typedef TypedInference<LIST(AbstractTheory*, Structure*, Vocabulary*)> ModelIterationWithVocInferenceBase;
class ModelIterationWithOutputVocInference: public ModelIterationWithVocInferenceBase {
public:
	ModelIterationWithOutputVocInference()
			: ModelIterationWithVocInferenceBase("createMXIterator",
					"Create an iterator generating models that satisfy the theory and which are more precise than the given structure.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
            return createIteratorCommand(get<0>(args), get<1>(args), get<2>(args));
	}
};

