/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
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

struct GroundingContext {
	GenType gentype; // if a certainly checker succeeds, then this type applies. if a possible checker fails, then the formula is irrelevant
	Context _funccontext;
	Context _monotone;
	CompContext _component; // Indicates the context of the visited formula
	TsType _tseitin; // Indicates the type of tseitin definition that needs to be used.
	std::set<PFSymbol*> _defined; // Indicates whether the visited rule is recursive.

	bool _conjPathUntilNode, _conjunctivePathFromRoot;
	// If true, there is a conjunctive path from the root of the sentence parsetree to this node.
	// NOTE advantage: can optimize by creating less tseitins by using the knowledge that the formula can be added directly into cnf
};

#endif /* GROUNDINGCONTEXT_HPP_ */
