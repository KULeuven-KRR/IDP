/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDINGCONTEXT_HPP_
#define GROUNDINGCONTEXT_HPP_

#include "Utils.hpp"

enum class CompContext {
	SENTENCE, HEAD, FORMULA
};
enum class GenType {
	CANMAKETRUE, CANMAKEFALSE
};

GenType operator not(GenType orig);

class PFSymbol;
class Variable;

int getIDForUndefined();

#define iff(x, y) ((x && y) || (not x && not y))

struct GroundingContext {
	GenType gentype; // if a certainly checker succeeds, then this type applies. if a possible checker fails, then the formula is irrelevant
	Context _funccontext;
	Context _monotone;
	CompContext _component; // Indicates the context of the visited formula
	TsType _tseitin; // Indicates the type of tseitin definition that needs to be used.
	int currentDefID; // If tstype = rule, this indicates the definition to which the grounders belong
	int getCurrentDefID() const{
		Assert(_tseitin!=TsType::RULE || currentDefID!=getIDForUndefined());
		return currentDefID;
	}
	std::set<PFSymbol*> _defined; // Indicates whether the visited rule is recursive.

	std::set<Variable*> _mappedvars; // Keeps track of which variables where added to the varmapping.

	bool _conjPathUntilNode, _conjunctivePathFromRoot;
	// If true, there is a conjunctive path from the root of the sentence parsetree to this node.
	// NOTE advantage: can optimize by creating less tseitins by using the knowledge that the formula can be added directly into cnf

	bool _allowDelaySearch;
};

#endif /* GROUNDINGCONTEXT_HPP_ */
