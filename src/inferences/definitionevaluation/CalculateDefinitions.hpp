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

class AbstractStructure;
class Theory;
class Definition;

class CalculateDefinitions {
public:
	//!Removes calculated definitions from the theory.
	// Also modifies the structure. Clone your theory and structure before doing this!
	static std::vector<AbstractStructure*> doCalculateDefinitions(Theory* theory, AbstractStructure* structure) {
		CalculateDefinitions c;
		return c.calculateKnownDefinitions(theory, structure);
	}

private:

	bool calculateDefinition(Definition* definition, AbstractStructure* structure) const;
	bool calculateAllDefinitions(std::set<Definition*> definitions, AbstractStructure* structure) const;
	std::set<Definition*> getAllCalculatableDefinitions(Theory* theory, AbstractStructure* structure) const;

	std::vector<AbstractStructure*> calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const;


};
