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

#ifndef MERGETHEORIES_HPP_
#define MERGETHEORIES_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

typedef TypedInference<LIST(AbstractTheory*, AbstractTheory*)> MergeTheoriesInferenceBase;
class MergeTheoriesInference: public MergeTheoriesInferenceBase {
public:
	MergeTheoriesInference()
			: MergeTheoriesInferenceBase("merge", "Create a new theory which is the result of combining both input theories.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(FormulaUtils::merge(get<0>(args), get<1>(args)));
	}
};

#endif /* MERGETHEORIES_HPP_ */
