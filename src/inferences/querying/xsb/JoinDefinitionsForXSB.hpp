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
template<class T>
class UniqueNames;
class Options;
class AbstractTheory;
class Theory;
class Predicate;
class Definition;
class PFSymbol;
class Structure;

/**
 * Join definitions that can be calculated using XSB as much as possible.
 * We do this so the XSB overhead is minimized.
 *
 * For example, the definitions
 *   { p <- q. }
 *   { q <- r. }
 * can be calculated in advance separately, but this created unneeded overhead in XSB:
 *   - first q will be calculated using the first definition,
 *   - XSB will load q's interpretation back into IDP, which is string-level communication
 *   - XSB deletes all tables used for the previous execution (i.e., q's calculated table)
 *   - XSB handles the second definition by loading q's interpretation as data, which is again
 *     string-level communication
 *   - XSB calculates p's table given q's interpretation as data
 *   - p's interpretation is communicated back into IDP, which is again string-level communication
 *
 * However, we can join the definitions to reduce XSB's overhead.
 * The definition
 *   { p <- q.
 *     q <- r. }
 * will only need to load in facts once (about r) and calculates p and q at once, such that q's table
 * is reused instead of the extra overhead described above.
 *
 * This transformation joins definitions which XSB can calculate as much as possible, without introducing
 * dependency loops or making the joined definition not calculable by XSB.
 *
 * For example, the definition
 *   { q <- p. }
 *   { p <- q. }
 * cannot be joined because it would change the meaning under the Well-Founded Semantics.
 */

class JoinDefinitionsForXSB {

public:
	Theory* execute(Theory* t, const Structure* s);
private:
	std::vector<Definition*> joinForXSB(Structure*,
			UniqueNames<const Definition*>&); //Takes a structure containing metadata regarding one or more definitions and splits
	void prepare(Structure*,
			UniqueNames<const Definition*>&,
			const Structure*,
			UniqueNames<PFSymbol*>&); //Prepares all options etcetera
	void finish(); //Should be ran before exiting! Set back options.

private:
	Options* savedOptions;

	AbstractTheory* jointheo;
	//Output of the bootstrapping
	Predicate* sameDef;
	Predicate* xsbCalculable;
	Predicate* twoValuedSymbol;

};
