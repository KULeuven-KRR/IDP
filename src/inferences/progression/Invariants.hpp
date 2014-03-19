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

#include <stddef.h>

class Structure;
class AbstractTheory;
class Theory;
class Formula;

class ProveInvariantInference {
	const Theory* _ltcTheo;
	const Theory* _invariant;
	const Structure* _structure;

public:
	static bool proveInvariant(const AbstractTheory* ltcTheo, const AbstractTheory* invar, const Structure* struc = NULL);
private:

	ProveInvariantInference(const Theory* ltcTheo, const Theory* invar, const Structure* struc = NULL);
	~ProveInvariantInference();
	bool run();
	/* Checks whether or not implication is implied by hypothesis in the context of a given structure.
	 * Takes ownership of context and of implication
	 *
	 * bool initial is true if and only if this is an initial state check (is only used for printing)
	 */
	bool checkImplied(const Theory* hypothesis, Formula* implication, Structure* context, bool initial);
	/* Checks whether or not implication is implied by hypothesis
	 * Takes ownership of implication
	 * bool initial is true if and only if this is an initial state check (is only used for printing)
	 */
	bool checkImplied(const Theory* hypothesis, Formula* implication, bool initial);
};
