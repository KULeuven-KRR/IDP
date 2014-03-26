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

#include "visitors/TheoryMutatingVisitor.hpp"

#include <vector>
template<class T>
class UniqueNames;
class Options;
class Vocabulary;
class Theory;
class AbstractTheory;
class Function;
class Predicate;
class Structure;

/**
 * Split definitions as much as possible. We do this so we can detect more
 * definitions that can be calculated in advance. For example, the definition
 *   { p <- true .
 *     q <- r. }
 * cannot be calculated in advance if r is not two-valued in the input structure.
 * However, the rule p <- true can still be calculated independently, so if we split
 * the definition to the following:
 *   { p <- true. }
 *   { q <- r.    }
 * The value of p can be calculated in advance.
 *
 * This transformation splits definitions as much as possible, without breaking dependency
 * loops. For example, the definition
 *   { q <- p.
 *     p <- q. }
 *  Cannot be split into two separate definitions because it would change the meaning
 *  under the Well-Founded Semantics.
 */

class SplitDefinitions {

public:
	Theory* execute(Theory* t);
private:
	std::vector<Definition*> split(Structure* s,UniqueNames<Rule*>& uniqueRuleNames); //Takes a structure containing metadata regarding one or more definitions and splits
	void prepare(); //Prepares all options etcetera
	void finish(); //Should be ran before exiting! Set back options.

private:
	Options* savedOptions;

	AbstractTheory* splittheo;
	//Output of the bootstrapping
	Predicate* sameDef;

};
