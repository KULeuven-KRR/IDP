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
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "inferences/definitionevaluation/refineStructureWithDefinitions.hpp"
#include "errorhandling/error.hpp"
#include <vector>

InternalArgument performCalculateDefinitions(Theory* theory, Structure* structure, Vocabulary* vocabulary) {
	auto sols = CalculateDefinitions::doCalculateDefinitions(theory, structure, vocabulary, false);
	if (not sols._hasModel ) {
		return InternalArgument();
	}
	Assert(sols._hasModel and sols._calculated_model != NULL);
	return InternalArgument(sols._calculated_model);
}

class CalculateDefinitionInference: public TheoryStructureBase {
public:
	CalculateDefinitionInference()
			: TheoryStructureBase("calculatedefinitions",
					"Make the structure more precise than the given one by evaluating all definitions with known open symbols.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		Theory* theory = NULL;
		if (isa<Theory>(*t)) {
			theory = (dynamic_cast<Theory*>(t))->clone(); //Because the doCalculateDefinitions Inferences changes the theory.
		} else {
			Error::error("Can only calculate definitions with a non-ground theory.");
			return nilarg();
		}
		return performCalculateDefinitions(theory,get<1>(args)->clone(),theory->vocabulary());
	}
};

class CalculateDefinitionWithVocabularyInference: public TheoryStructureVocabularyBase {
public:
	CalculateDefinitionWithVocabularyInference()
			: TheoryStructureVocabularyBase("calculatedefinitions",
					"Make the structure more precise than the given one by evaluating all definitions with known open symbols for those symbols that are also in the given vocabulary.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		Theory* theory = NULL;
		if (isa<Theory>(*t)) {
			theory = (dynamic_cast<Theory*>(t))->clone(); //Because the doCalculateDefinitions Inferences changes the theory.
		} else {
			Error::error("Can only calculate definitions with a non-ground theory.");
			return nilarg();
		}
		
		return performCalculateDefinitions(theory,get<1>(args)->clone(),get<2>(args));
	}
};

class RefineDefinitionsInference: public TheoryStructureBase {
public:
	RefineDefinitionsInference()
			: TheoryStructureBase("refinedefinitions",
					"Make the structure more precise than the given one by evaluating all definitions as much as possible for the given open symbols.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		Theory* theory = NULL;
		if (isa<Theory>(*t)) {
			theory = (dynamic_cast<Theory*>(t))->clone(); //Because the doCalculateDefinitions Inferences changes the theory.
		} else {
			Error::error("Can only calculate definitions with a non-ground theory.");
			return nilarg();
		}
		auto newTheory = new Theory("", theory->vocabulary(), ParseInfo());
		for (auto definition : theory->definitions()) {
			newTheory->add(definition->clone());
		}
		auto result = CalculateDefinitions::doCalculateDefinitions(newTheory,get<1>(args));
		auto structure = result._calculated_model;
		auto sols = refineStructureWithDefinitions::doRefineStructureWithDefinitions(newTheory, structure);
		if(not sols._hasModel ){
			delete newTheory;
			return nilarg();
		}
		Assert(sols._hasModel and sols._calculated_model != NULL);

		return InternalArgument(sols._calculated_model);
	}
};
