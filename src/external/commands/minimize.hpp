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

#ifndef OPTIMIZE_HPP_
#define OPTIMIZE_HPP_

#include <iostream>
#include "commandinterface.hpp"

#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "lua/luaconnection.hpp"

typedef TypedInference<LIST(AbstractTheory*, Structure*, Term*)> OptimizeInferenceBase;
class MinimizeInference: public OptimizeInferenceBase {
public:
	MinimizeInference() :
			OptimizeInferenceBase(
					"minimize",
					"Return a vector of models of the given theory, more precise than the given structure. The second return value is a boolean, representing whether or not the models are optimal with respect to the given term. The third value is the optimal value of the term.",
					false) {
		setNameSpace(getInternalNamespaceName());
	}

	// TODO trace is returned as the SECOND return value of the lua call
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		LuaTraceMonitor* tracer = NULL;
		if (getOption(BoolType::TRACE)) {
			tracer = LuaConnection::getLuaTraceMonitor();
		}
		auto mxresult = ModelExpansion::doMinimization(get<0>(args), get<1>(args), get<2>(args), NULL, tracer);
		auto models = mxresult._models;
		auto optimumfound = mxresult._optimumfound;
		auto value = mxresult._optimalvalue;

		// Convert models to internal arguments
		InternalArgument luamodels;
		luamodels._type = AT_TABLE;
		luamodels._value._table = new std::vector<InternalArgument>();
		addToGarbageCollection(luamodels._value._table);
		for (auto it = models.cbegin(); it != models.cend(); ++it) {
			luamodels._value._table->push_back(InternalArgument(*it));
		}

		//All return values
		InternalArgument randt;
		randt._type = AT_MULT;
		randt._value._table = new std::vector<InternalArgument>(1, luamodels);
		addToGarbageCollection(randt._value._table);

		//Optimumfound
		InternalArgument opt;
		opt._type = AT_BOOLEAN;
		opt._value._boolean = optimumfound;
		randt._value._table->push_back(opt);

		//Optimal value
		InternalArgument val;
		opt._type = AT_INT;
		opt._value._int = value;
		randt._value._table->push_back(opt);

		if (tracer != NULL) {

			InternalArgument trace;
			trace._type = AT_REGISTRY;
			trace._value._string = tracer->index();
			randt._value._table->push_back(trace);
			delete (tracer);
		}

		return randt;
	}
};

#endif //OPTIMIZE_HPP_
