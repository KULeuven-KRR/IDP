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

#pragma once

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/Grounding.hpp"
#include "inferences/grounding/GrounderFactory.hpp"

#include "theory/TheoryUtils.hpp" //TODO REMOVE
//#include "external/FlatZincRewriter.hpp"

typedef TypedInference<LIST(AbstractTheory*, Structure*, bool)> GroundBase;
class GroundInference: public GroundBase {
public:
	GroundInference()
			: GroundBase("ground",
					"Returns theory which is the grounding of the given theory in the given structure.\n Does not change its input argument. The boolean parameter should be true if the grounding should preserve the number of models.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto grounding = ground(get<0>(args), get<1>(args), get<2>(args));
		return InternalArgument(grounding);
	}
private:
	AbstractTheory* ground(AbstractTheory* theory, Structure* structure, bool modelcountequivalence) const {
		auto t = theory->clone();
		auto s = structure->clone();
		auto voc = new Vocabulary("intern_voc");
		voc->add(t->vocabulary());
		voc->add(s->vocabulary());
		s->changeVocabulary(voc);
		t->vocabulary(voc);

		//Giving InteractivePrintMonitor as template argument but in fact, nothing is needed...
		auto grounding = GroundingInference<InteractivePrintMonitor>::doGrounding(t, s, NULL, NULL, NULL, modelcountequivalence, NULL);
		if (grounding == NULL) {
			grounding = new GroundTheory<GroundPolicy>(NULL, {NULL, shared_ptr<GenerateBDDAccordingToBounds>()}, false);
			grounding->addEmptyClause();
		}
		// FIXME probably not allowed to delete any of these in case of lazy grounding
		t->recursiveDelete();
		delete (s);
		// delete (voc); // Certainly not allowed to delete this one, as it is the voc of the ground theory itself and the voc of a cloned structure inside the ground theory
		return grounding;
	}
};

// TODO add an option to write to a file instead to stdout.

class PrintGroundingInference: public GroundBase {
public:
	PrintGroundingInference()
			: GroundBase("printgrounding",
					"Prints the grounding to cout. The boolean parameter should be true if the grounding should preserve the number of models.", true) {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		ground(get<0>(args), get<1>(args), printmonitor(), get<2>(args));
		return nilarg();
	}
private:
	void ground(AbstractTheory* theory, Structure* structure, InteractivePrintMonitor* monitor, bool modelcountequivalence) const {
		auto t = theory->clone();
		auto s = structure->clone();
		auto voc = new Vocabulary("intern_voc");
		voc->add(t->vocabulary());
		voc->add(s->vocabulary());
		s->changeVocabulary(voc);
		t->vocabulary(voc);

		auto grounding = GroundingInference<InteractivePrintMonitor>::doGrounding(t, s, NULL, NULL, NULL, modelcountequivalence, monitor);
		t->recursiveDelete();
		delete (s);
		grounding->recursiveDelete();
		delete (voc);
		monitor->flush();
	}
};
