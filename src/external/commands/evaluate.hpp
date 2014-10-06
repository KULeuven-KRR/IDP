/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "inferences/querying/Query.hpp"
#include "errorhandling/error.hpp"

class EvaluateTheoryInference: public TheoryStructureBase {
public:
	EvaluateTheoryInference()
			: TheoryStructureBase("value", "Gets the value of a theory in a two-valued structure.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		for(auto s: theory->getComponents()){
			if(not evaluate(s, get<1>(args))){
				return InternalArgument(false);
			}
		}
		return InternalArgument(true);
	}
};

class EvaluateFormulaInference: public FormulaStructureBase {
public:
	EvaluateFormulaInference()
			: FormulaStructureBase("value", "Gets the value of a sentence in a two-valued structure.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(evaluate(get<0>(args), get<1>(args)));
	}
};

class EvaluateTermInference: public TermStructureBase {
public:
	EvaluateTermInference()
			: TermStructureBase("value", "Gets the value of a variable-free term in a two-valued structure.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(evaluate(get<0>(args), get<1>(args)));
	}
};
