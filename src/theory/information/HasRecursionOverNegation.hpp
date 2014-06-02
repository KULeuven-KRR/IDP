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

#include <set>

class Definition;
template<class T>
class UniqueNames;
class Options;
class PFSymbol;
class Structure;
class AbstractTheory;
class Predicate;

class HasRecursionOverNegation {

public:
	bool execute(const Definition* d);

};

class RecursionOverNegationSymbols {

public:
	std::set<PFSymbol*> execute(const Definition* d);

private:
	void prepare();
	void finish();
	std::set<PFSymbol*> handle(Structure* struc, UniqueNames<PFSymbol*>);

private:
	Options* savedOptions;
	AbstractTheory* bootstraptheo;
	Predicate* recursivePredicates;

};

//Cheaper method with the same goal as hasrecursionovernegation. Might give too much "true" answers.
//However, in case definitions are split, is guaranteed to be equal.
class ApproxHasRecursionOverNegation {

public:
	bool execute(const Definition* d);

};

//Cheaper method with the same goal as hasrecursionovernegation. Might give too much answers.
//However, in case definitions are split, is guaranteed to be equal.
class ApproxRecursionOverNegationSymbols {

public:
	std::set<PFSymbol*> execute(const Definition* d);
};
