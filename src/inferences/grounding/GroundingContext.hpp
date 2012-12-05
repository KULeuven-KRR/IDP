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

#ifndef GROUNDINGCONTEXT_HPP_
#define GROUNDINGCONTEXT_HPP_

#include "GroundUtils.hpp"
#include "vocabulary/VarCompare.hpp"

enum class CompContext {
	HEAD, FORMULA
};
enum class GenType {
	CANMAKETRUE, CANMAKEFALSE
};

GenType operator not(GenType orig);

class PFSymbol;
class Variable;

DefId getIDForUndefined();

#define iff(x, y) ((x && y) || (not x && not y))

struct GroundingContext {
	GenType gentype; // if a certainly checker succeeds, then this type applies. if a possible checker fails, then the formula is irrelevant
	Context _funccontext;
	Context _monotone;
	CompContext _component; // Indicates the context of the visited formula

	TsType _tseitin; // Indicates the type of tseitin definition that needs to be used.
	DefId currentDefID; // If tstype = rule, this indicates the definition to which the grounders belong
	std::set<PFSymbol*> _defined; // Indicates which symbols are defined, allowing to derive loops.

	varset _mappedvars; // Keeps track of which variables where added to the varmapping.

	// True if all parent nodes in the parse tree are such that the current node can be treated as if it was the root.
	bool _conjPathUntilNode;
	// True of all child nodes can be treated as if they were root themselves.
	bool _conjunctivePathFromRoot;

	TruthValue _cpablerelation; // Indicates whether the current object is in the context of a relation eligible for CP.

	DefId getCurrentDefID() const {
		Assert(_tseitin!=TsType::RULE || currentDefID!=getIDForUndefined());
		return currentDefID;
	}
};

#endif /* GROUNDINGCONTEXT_HPP_ */
