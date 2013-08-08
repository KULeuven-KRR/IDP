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

#include <memory>
#include <vector>

class PredTable;
class Query;
class Structure;
class FOBDD;
class Variable;
class FOBDDManager;
class GenerateBDDAccordingToBounds;

class Querying {
public:
	static PredTable* doSolveQuery(Query* q, Structure const * const s) {
		Querying c;
		return c.solveQuery(q, s);
	}
	static PredTable* doSolveQuery(Query* q, Structure const * const s, std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure) {
		Querying c;
		return c.solveQuery(q, s, symbolicstructure);
	}

private:
	PredTable* solveQuery(Query* q, Structure const * const s) const;
	PredTable* solveQuery(Query* q, Structure const * const s, std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure) const;
	PredTable* solveBdd(const std::vector<Variable*>& vars, FOBDDManager* manager, const FOBDD* bdd, Structure const * const structure) const;
};
