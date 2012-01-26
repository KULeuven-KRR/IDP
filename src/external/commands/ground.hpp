/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUND_HPP_
#define GROUND_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"

typedef TypedInference<LIST(AbstractTheory*, AbstractStructure*)> GroundBase;
class GroundInference: public GroundBase {
public:
	GroundInference()
			: GroundBase("ground", "Returns theory which is the grounding of the given theory in the given structure.\n Does not change its input argument.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto grounding = ground(get<0>(args), get<1>(args));
		return InternalArgument(grounding);
	}
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure) const {
		// TODO bugged! auto symstructure = generateApproxBounds(theory, structure);
		auto symstructure = generateNaiveApproxBounds(theory, structure);
		GrounderFactory factory(structure, symstructure);
		auto grounder = std::shared_ptr<Grounder>(factory.create(theory));
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();
		delete (symstructure);
		return grounding;
	}
};

// TODO add an option to write to a file instead to stdout.

class PrintGroundingInference: public GroundBase {
public:
	PrintGroundingInference()
			: GroundBase("printgrounding", "Prints the grounding to cout.", true) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		ground(get<0>(args), get<1>(args), printmonitor());
		return nilarg();
	}
private:
	void ground(AbstractTheory* theory, AbstractStructure* structure, InteractivePrintMonitor* monitor) const {
		auto symstructure = generateNaiveApproxBounds(theory, structure);
		GrounderFactory factory(structure, symstructure);
		auto grounder = std::shared_ptr<Grounder>(factory.create(theory, monitor));
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();
		grounding->recursiveDelete();
		delete (symstructure);
		monitor->flush();
	}
};

#endif /* GROUND_HPP_ */