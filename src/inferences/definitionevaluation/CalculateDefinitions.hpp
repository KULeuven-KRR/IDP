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
#include <vector>
#include <set>
#include "vocabulary/vocabulary.hpp"

class Structure;
class Theory;
class Definition;

class CalculateDefinitions {
public:
	//!Removes calculated definitions from the theory.
	// Also modifies the structure. Clone your theory and structure before doing this!
	static std::vector<Structure*> doCalculateDefinitions(Theory* theory, Structure* structure,
			bool satdelay = false, std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		CalculateDefinitions c;
		return c.calculateKnownDefinitions(theory, structure, satdelay, symbolsToQuery);
	}
	static std::vector<Structure*> doCalculateDefinitions(
			Definition* definition, Structure* structure,
			std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		CalculateDefinitions c;
		return c.calculateKnownDefinition(definition, structure, symbolsToQuery);
	}

private:
	std::vector<Structure*> calculateKnownDefinitions(Theory* theory, Structure* structure,
			bool satdelay, std::set<PFSymbol*> symbolsToQuery) const;
	bool calculateDefinition(Definition* definition, Structure* structure,
			bool satdelay, bool& tooExpensive, bool withxsb, std::set<PFSymbol*> symbolsToQuery) const;

	std::vector<Structure*> calculateKnownDefinition(Definition* definition, Structure* structure, std::set<PFSymbol*> symbolsToQuery);
};
