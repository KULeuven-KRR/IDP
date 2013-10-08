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

#include "SplitDefinitions.hpp"
#include "theory/TheoryUtils.hpp"

#include "IncludeComponents.hpp"


Theory* SplitDefinitions::visit(Theory* theory) {

	for (auto def : theory->definitions()) {
		def->accept(this);
	}
	theory->definitions().clear();
	for (auto newdef : _definitions_to_add) {
		theory->add(newdef);
	}
	return theory;
}

Definition* SplitDefinitions::visit(Definition* def) {
	notyetimplemented("Splitting definitions");
	std::map<PFSymbol*,Definition*> defsymbols2def;

	// Split all defined symbols into separate definitions
	for (auto symb : def->defsymbols()) {
		auto newdef = new Definition();
		for (auto rule : def->rules()) {
			if (symb == rule->head()->symbol()) {
				newdef->add(rule);
			}
		}
		defsymbols2def[symb]=newdef;
	}

	// Now loop over all definitions and re-merge those that were a dependency loop
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		// iterate over all defined symbols and their definitions
		for (auto symbol2def : defsymbols2def) {
			auto firstopens = DefinitionUtils::opens(symbol2def.second);
			// find a second definition that contains the first as open and where the first definition has the second as open
			for (auto symbol2def_2 : defsymbols2def) {
				auto secondopens = DefinitionUtils::opens(symbol2def_2.second);
				// Two definitions are in loop (have to be merged) if:
				// they don't refer to the same definition already
				// and the first definition has the defined symbol of the second definition in its opens
				// and the second definition has the defined symbol of the first definition in its opens
				if (symbol2def.second != symbol2def_2.second &&
					firstopens.find(symbol2def_2.first) != firstopens.cend() &&
					secondopens.find(symbol2def.first) != secondopens.cend()) {
							// now we have to merge both definitions
							fixpoint = false;
							auto merged_def = merge_defs(symbol2def.second, symbol2def_2.second);
							for (auto defsymbol : merged_def->defsymbols()) {
								defsymbols2def[defsymbol]=merged_def;
							}
				}
			}
		}
	}

	for (auto pair : defsymbols2def) {
		_definitions_to_add.insert(pair.second);
	}
	return def;
}


Definition* SplitDefinitions::merge_defs(Definition* one, Definition* two) {
	auto newdef = new Definition();
	newdef->add(one->rules());
	newdef->add(two->rules());
	return newdef;
}
