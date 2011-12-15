/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CALCULATEDEFINITIONSCOMMAND_HPP_
#define CALCULATEDEFINITIONSCOMMAND_HPP_

#include <vector>
#include <iostream>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "common.hpp"
#include "utils/TheoryUtils.hpp"
#include "inferences/CalculateDefinitions.hpp"

class CalculateDefinitionInference: public Inference {
public:
	CalculateDefinitionInference() :
			Inference("calculatedefinitions") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Assert(sametypeid<Theory>(*(args[0].theory())));
		Assert(sametypeid<Structure>(*(args[1].structure())));
		auto theory = dynamic_cast<Theory*>(args[0].theory());
		auto structure = dynamic_cast<Structure*>(args[1].structure());
		GlobalData::instance()->setOptions(args[2].options());
		auto clone = structure->clone();
		auto result = CalculateDefinitions::doCalculateDefinitions(theory, clone);
		if (not result->isConsistent() ) {
			if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
				std::clog << "Calculating definitions resulted in inconsistent model. \n" << "Theory is unsatisfiable.\n";
			}
		}
		return InternalArgument(result);
	}
};

#endif /* CALCULATEDEFINITIONSCOMMAND_HPP_ */
