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

#ifndef SHAREDTSEITINS354654_HPP_
#define SHAREDTSEITINS354654_HPP_

#include <vector>
#include <map>
#include "visitors/TheoryMutatingVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/bddvisitors/BddToFormulaWithTseitins.hpp"
#include "fobdds/bddvisitors/CountOccurences.hpp"
#include "fobdds/FoBddFactory.hpp"


class FOBDDManager;
class FOBDD;
class BDDToFOWithTseitins;

class IntroduceSharedTseitins{
private:
	std::shared_ptr<FOBDDManager> _manager;
	FOBDDFactory _factory;
	CountOccurences _counter;
	BDDToFOWithTseitins _bddtofo;
public:
	//NOTE: if the structure is given, more efficient unnesting can happen (deriving of term bounds)
	IntroduceSharedTseitins();

	//Introduces Tseitins for subformulas of the Theory that occur a lot. Modifies the theory's vocabulary (by adding tseitin symbols)
	//WARNING: modifies the vocabulary of the theory! Does not modify a structure.
	Theory* execute(Theory* t, Structure* s = NULL);
};
#endif /* SHAREDTSEITINS354654_HPP_ */
