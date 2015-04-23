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
#include "theory/TheoryUtils.hpp"
#include "errorhandling/error.hpp"

typedef TypedInference<LIST(Structure*, Structure*)> MergeStructuresInferenceBase;
class MergeStructuresInference: public MergeStructuresInferenceBase {
public:
	MergeStructuresInference()
			: MergeStructuresInferenceBase("merge", "Make the first structure more precise using the second one. Only shared relations are combined.") {
		setNameSpace(getStructureNamespaceName());
	}

	void addToSortTable(SortTable* toAddTo, SortTable* toGetFrom) const {
		if (toAddTo->internTable() != toGetFrom->internTable()) {
			for (auto it = toGetFrom->sortBegin(); not it.isAtEnd(); ++it) {
				toAddTo->add(*it);
			}
		}
	}

	void addToPredInter(PredInter* toAddTo, PredInter* toGetFrom) const {
		//NOTE: LOTS OF smarter stuff can be done here to merge more efficiently. E.g. comparing table sizes, e.g., disabling consistency checks (if safe), ...
		auto toGetFromCT = toGetFrom->ct();
		auto toAddToCT = toAddTo->ct();
		auto toGetFromCF = toGetFrom->cf();
		auto toAddToCF = toAddTo->cf();
		if (toAddToCT->approxEmpty() && toAddToCF->approxEmpty()) {
			toAddTo->setTables(new PredTable(toGetFromCT->internTable(), toAddToCT->universe()),
					new PredTable(toGetFromCF->internTable(), toAddToCF->universe()), true, true);
			return;
		}

		if (not toGetFromCT->approxEqual(toAddToCT)) {
			for (auto it = toGetFrom->ct()->begin(); not it.isAtEnd(); ++it) {
				toAddTo->makeTrueAtLeast(*it);
			}
		}


		if (not toGetFromCF->approxEqual(toAddToCF)) {
			for (auto it = toGetFrom->cf()->begin(); not it.isAtEnd(); ++it) {
				toAddTo->makeFalseAtLeast(*it);
			}
		}
	}

	void warnIfNoLongerTwoValued(Structure const * const firstStructure, Structure const * const secondStructure,
			Structure* result) const {
		//In case some relation was originally two-valued, but no longer is two-valued now (due to the merging of the sorts), we want to warn the user

		if (not result->isConsistent()) {
			Warning::warning("Merging structures resulted in an inconsistent structure.");
		}

		// Predicates
		for (auto pred2inter : result->getPredInters()) {
			auto pred = pred2inter.first;
			bool wasTwoValued =  (firstStructure->vocabulary()->contains(pred) && firstStructure->inter(pred)->approxTwoValued())
									|| (secondStructure->vocabulary()->contains(pred) && secondStructure->inter(pred)->approxTwoValued());
			if (wasTwoValued) {
				if (not pred2inter.second->approxTwoValued()) {
					std::stringstream ss;
					ss << "Merging sorts resulted in a structure where the relation " << print(pred) << " is no longer two-valued.";
					Warning::warning(ss.str());
				}
			}
		}
		// Functions
		for (auto func2inter : result->getFuncInters()) {
			auto func = func2inter.first;
			bool wasTwoValued = (firstStructure->vocabulary()->contains(func) && firstStructure->inter(func)->approxTwoValued())
							|| (secondStructure->vocabulary()->contains(func) &&  secondStructure->inter(func)->approxTwoValued());
			if (wasTwoValued) {
				if (not func2inter.second->approxTwoValued()) {
					std::stringstream ss;
					ss << "Merging sorts resulted in a structure where the relation " << print(func) << " is no longer two-valued.";
					Warning::warning(ss.str());
				}
			}
		}
	}

	Structure* merge(Structure const * const firstStructure, Structure const * const secondStructure) const {

		auto result = firstStructure->clone();
		if (firstStructure->vocabulary() != secondStructure->vocabulary()) {
			//In case the vocabularies differ, we merge but give a warning.
			std::stringstream ss;
			ss << "merge_of_" << firstStructure->vocabulary()->name() << "_and_" << secondStructure->vocabulary()->name();
			auto voc = new Vocabulary(ss.str());
			voc->add(firstStructure->vocabulary());
			voc->add(secondStructure->vocabulary());
			Warning::warning("Vocabularies of the structures that I should merge, are not equal. The result will be a structure over a merged vocabulary.");
			result->changeVocabulary(voc);
		}

		// Sorts
		for (auto sort2inter : result->getSortInters()) {
			auto sort = sort2inter.first;
			if (secondStructure->vocabulary()->contains(sort)) {
				addToSortTable(sort2inter.second, secondStructure->inter(sort));
			}
		}
		// Predicates
		for (auto pred2inter : result->getPredInters()) {
			auto pred = pred2inter.first;
			if (secondStructure->vocabulary()->contains(pred)) {
				addToPredInter(pred2inter.second, secondStructure->inter(pred));
			}
		}
		// Functions
		for (auto func2inter : result->getFuncInters()) {
			auto func = func2inter.first;
			auto funcinter = func2inter.second;
			if (secondStructure->vocabulary()->contains(func)) {
				//In case that the sorts have changed, the functable becomes invalid and we should remove it.
				//We do this by setting thegraphinter again.
				funcinter->graphInter(funcinter->graphInter()->clone(funcinter->universe()));
				addToPredInter(funcinter->graphInter(), secondStructure->inter(func)->graphInter());
			}
		}

		result->clean();

		warnIfNoLongerTwoValued(firstStructure, secondStructure, result);

		return result;
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(merge(get<0>(args), get<1>(args)));
	}
};
