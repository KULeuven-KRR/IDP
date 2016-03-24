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

#include "IncludeComponents.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/SolverConnection.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagation {
public:
	 std::vector<Structure*>  propagate(AbstractTheory* theory, Structure* structure) {
		 // TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		 Warning::warning("Optimal propagator does not work with non-propositional grounding yet, so disabling cpsupport.");
		 auto storedcp = getOption(CPSUPPORT);
		 setOption(CPSUPPORT, false);

		// TODO: make a clean version of the implementation (should call ModelExpansion)

		// Compute all models

		auto data = SolverConnection::createsolver(0);

		//Grounding
		auto symstructure = generateBounds(theory, structure, true, true);
		auto grounder = GrounderFactory::create(GroundInfo{theory, {structure, symstructure}, NULL, true, NULL /*TODO CHeck*/}, data);
		bool unsat = grounder->toplevelRun();
		if(unsat){
			return std::vector<Structure*> { };
		}
		auto grounding = grounder->getGrounding();

		data->finishParsing();
		auto mx = SolverConnection::initsolution(data, 0);
		mx->execute();

		setOption(CPSUPPORT, storedcp);

		auto abstractsolutions = mx->getSolutions();

		std::set<int> intersection;
		if (abstractsolutions.empty()) {
			return std::vector<Structure*> { };
		}
		// Take the intersection of all models
		auto firstmodel = *(abstractsolutions.cbegin());
		for (auto it = firstmodel->literalinterpretations.cbegin(); it != firstmodel->literalinterpretations.cend(); ++it) {
			intersection.insert(getIntLit(*it));
		}
		for (auto currmodel = (abstractsolutions.cbegin()); currmodel != abstractsolutions.cend(); ++currmodel) {
			for (auto it = (*currmodel)->literalinterpretations.cbegin(); it != (*currmodel)->literalinterpretations.cend(); ++it) {
				if (intersection.find(getIntLit(*it)) == intersection.cend()) {
					intersection.erase(-1*getIntLit(*it));
				}
			}
		}

		//Translate the result
		auto translator = grounding->translator();
		auto result = structure->clone();
		for (auto literal = intersection.cbegin(); literal != intersection.cend(); ++literal) {
			int atomnr = (*literal > 0) ? *literal : (-1) * (*literal);
			if (translator->isInputAtom(atomnr)) {
				auto symbol = translator->getSymbol(atomnr);
				const auto& args = translator->getArgs(atomnr);
				if (*literal < 0) {
					result->inter(symbol)->makeFalseAtLeast(args);
				} else {
					result->inter(symbol)->makeTrueAtLeast(args);
				}
			}
		}
		result->clean();

		delete (grounder);
		grounding->recursiveDelete();
		delete(mx);
		delete(data);

		if(not result->isConsistent()){
			return std::vector<Structure*> { };
		}
		return {result};
	}
};
