/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "inferences/functiondetection/FunctionDetection.hpp"

class DetectFunctionsInference: public TheoryBase {
public:
	DetectFunctionsInference()
			: TheoryBase("detectfunctionsNotEmpty", "Detect functions, in the assumption that types will never be empty.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		auto theory = dynamic_cast<Theory*>(t);
		if(theory==NULL){
			return InternalArgument(false);
		}
		FunctionDetection::doDetectAndRewriteIntoFunctions(theory, true);
		return InternalArgument(theory);
	}
};

class DetectFunctionsInference2: public TheoryBase {
public:
	DetectFunctionsInference2()
			: TheoryBase("detectfunctions", "Detect functions and rewrite the theory accordingly, assuming that types can be empty.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		auto theory = dynamic_cast<Theory*>(t);
		if(theory==NULL){
			return InternalArgument(false);
		}
		FunctionDetection::doDetectAndRewriteIntoFunctions(theory,false);
		return InternalArgument(theory);
	}
};
