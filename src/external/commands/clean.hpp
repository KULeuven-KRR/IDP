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

#ifndef CLEAN_HPP_
#define CLEAN_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

class CleanInference: public StructureBase {
public:
	CleanInference()
			: StructureBase("clean",
					"Combines fully specified three-valued relations into two-valued ones.\nModifies its argument and does not return anything.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		get<0>(args)->clean();
		return nilarg();
	}
};

#endif /* CLEAN_HPP_ */
