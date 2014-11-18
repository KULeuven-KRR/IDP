/* 
 * File:   modeliteration.hpp
 * Author: rupsbant
 *
 * Created on November 18, 2014, 2:35 PM
 */

#ifndef MODELITERATION_HPP
#define	MODELITERATION_HPP

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
        InternalArgument ia(&iterator);
        std::cout << "There\n";
        return ia;
}

typedef TypedInference<LIST(AbstractTheory*, Structure*)> ModelIterationInferenceBase;
class ModelIterationInference: public ModelIterationInferenceBase {
public:
	ModelIterationInference()
			: ModelIterationInferenceBase("createIterator",
					"Create an iterator generating 2-valued models of the theory which are more precise than the given structure.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
                std::cout << "Here\n";
		return createIteratorCommand(get<0>(args), get<1>(args), NULL);
	}
};

typedef TypedInference<LIST(AbstractTheory*, Structure*, Vocabulary*)> ModelIterationWithVocInferenceBase;
class ModelIterationWithOutputVocInference: public ModelIterationWithVocInferenceBase {
public:
	ModelIterationWithOutputVocInference()
			: ModelIterationWithVocInferenceBase("createIterator",
					"Create an iterator generating models of the theory which are more precise than the given structure"
					" and twovalued on the output vocabulary.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return createIteratorCommand(get<0>(args), get<1>(args), get<2>(args));
	}
};


#endif	/* MODELITERATION_HPP */

