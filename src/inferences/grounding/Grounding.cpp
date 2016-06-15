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
	t->setTranslator(grounder->translator());
	t->setSolver(solver);
}

void addSymmetryBreaking(AbstractTheory* theory, Structure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm, bool nbModelsEquivalent) {
	switch (getGlobal()->getOptions()->symmetryBreaking()) {
	case SymmetryBreaking::NONE:
		break;
	case SymmetryBreaking::STATIC: {
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Constructing symmetry breakers at ");
		}
		// Detect symmetry
        std::vector<Symmetry*> generators;
        std::vector<InterchangeabilityGroup*> intchgroups;
		detectSymmetry(intchgroups, generators, theory, structure, minimizeTerm);
		// Break symmetry
		if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
		  clog << "Breaking " << intchgroups.size() << " interchangeability groups." << std::endl;
		}
		for(auto icg: intchgroups){
		  icg->addBreakingClauses(grounding, structure, nbModelsEquivalent);
          delete icg;
		}
        if (getOption(IntType::VERBOSE_SYMMETRY) > 0) {
          clog << "Also breaking " << generators.size() << " symmetry generators found by Saucy." << std::endl;
		}
        for(auto sym: generators){
		  sym->addBreakingClauses(grounding, structure, nbModelsEquivalent);
          delete sym;
		}
        
        if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Ending construction of symmetry breakers at ");
		}
	}
	}
}

