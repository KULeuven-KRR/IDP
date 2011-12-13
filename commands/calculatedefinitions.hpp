/************************************
 calculatedefinitions.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

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
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Assert(sametypeid<Theory>(*(args[0].theory())));
		Assert(sametypeid<Structure>(*(args[1].structure())));
		auto theory = dynamic_cast<Theory*>(args[0].theory());
		auto structure = dynamic_cast<Structure*>(args[1].structure());
		if (not CalculateDefinitions::doCalculateDefinitions(theory, structure)) {
			if (getOption(IntType::GROUNDVERBOSITY) >= 1) { //TODO see issue 18. Fix the options
				std::clog << "Calculating definitions resulted in inconsistent model. \n" << "Theory is unsatisfiable.\n";
			}
		}
		return InternalArgument(); //TODO is this ok?
	}
};

#endif /* CALCULATEDEFINITIONSCOMMAND_HPP_ */
