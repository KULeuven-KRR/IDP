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
#include "inferences/grounding/Grounding.hpp"
#include "inferences/grounding/GrounderFactory.hpp"

#include "theory/TheoryUtils.hpp" //TODO REMOVE
//#include "external/FlatZincRewriter.hpp"

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
		auto t = theory->clone();
		auto s = structure->clone();
		//Giving InteractivePrintMonitor as template argument but in fact, nothing is needed...
		auto grounder = GroundingInference<InteractivePrintMonitor>::createGroundingInference(t, s, NULL, NULL, NULL);
		auto grounding = grounder->ground();
		if (grounding == NULL) {
			grounding = new GroundTheory<GroundPolicy>(NULL);
			grounding->addEmptyClause();
		}
		t->recursiveDelete();
		delete (s);
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
		auto t = theory->clone();
		auto s = structure->clone();
		auto grounder = GroundingInference<InteractivePrintMonitor>::createGroundingInference(t, s, NULL, NULL, monitor);
		auto grounding = grounder->ground();
		t->recursiveDelete();
		delete (s);
		if (grounding != NULL) {
			grounding->recursiveDelete();
		}
		else{
			clog << "false\n";
		}
		monitor->flush();
	}
};

#endif /* GROUND_HPP_ */
