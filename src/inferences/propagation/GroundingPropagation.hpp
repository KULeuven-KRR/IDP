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
#include "inferences/modelexpansion/PropagateMonitor.hpp"
#include "inferences/grounding/Grounding.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/SolverConnection.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundingPropagation {
public:
	std::vector<Structure*> propagate(AbstractTheory* theory, Structure* structure) {
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)

		//Set MinisatID solver options
		auto data = SolverConnection::createsolver(0);

		auto clonetheory = theory->clone();
		auto result = structure->clone();
		auto voc = new Vocabulary("intern_voc");
		voc->add(clonetheory->vocabulary());
		result->changeVocabulary(voc);
		clonetheory->vocabulary(voc);

		auto grounding = GroundingInference<PCSolver>::doGrounding(clonetheory, result, NULL, NULL, NULL, true, data);

		data->finishParsing();
		auto mx = SolverConnection::initpropsolution(data);
		mx->execute();

		result->changeVocabulary(structure->vocabulary());

		auto translator = grounding->translator();
		auto entailed = mx->getEntailedLiterals();
		for (auto literal = entailed.cbegin(); literal < entailed.cend(); ++literal) {
			int atomnr = var(*literal);
			if (translator->isInputAtom(atomnr)) {
				auto symbol = translator->getSymbol(atomnr);
				auto args = translator->getArgs(atomnr);
				if (sign(*literal)) {
					result->inter(symbol)->makeFalseAtLeast(args);
				} else {
					result->inter(symbol)->makeTrueAtLeast(args);
				}
			}
		}
		result->clean();
		clonetheory->recursiveDelete();
		delete (voc);
		delete (data);
		delete (mx);

		if (not result->isConsistent()) {
			return std::vector<Structure*> { };
		}
		return {result};
	}
};
