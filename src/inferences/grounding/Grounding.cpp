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

#include "Grounding.hpp"

#include "inferences/SolverInclude.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"

template<>
void connectTraceMonitor(TraceMonitor* t, Grounder* grounder, PCSolver* solver) {
	t->setTranslator(grounder->getTranslator());
	t->setSolver(solver);
}

void addSymmetryBreaking(AbstractTheory* theory, AbstractStructure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm, bool nbModelsEquivalent) {
	switch (getGlobal()->getOptions()->symmetryBreaking()) {
	case SymmetryBreaking::NONE:
		break;
	case SymmetryBreaking::STATIC: {
		// Add symmetry breakers
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Adding symmetry breakers");
		}
		auto ivsets = findIVSets(theory, structure, minimizeTerm);
		addSymBreakingPredicates(grounding, ivsets, nbModelsEquivalent);
		break;
	}
	case SymmetryBreaking::DYNAMIC: {
		// Add symmetry breakers
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Adding symmetry propagators");
		}
		auto ivsets = findIVSets(theory, structure, minimizeTerm);
		for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
			grounding->addSymmetries((*ivsets_it)->getBreakingSymmetries(grounding));
		}
		break;
	}
	}
}

