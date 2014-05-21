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

class Formula;
class Term;
class DomainElement;
class PredTable;
class Query;
class FOBDD;
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
	static PredTable* doSolveBDDQuery(const FOBDD* b, Structure const * const s) {
		Querying c;
		return c.solveBDDQuery(b, s);
	}


private:
	PredTable* solveQuery(Query* q, Structure const * const s) const;
	PredTable* solveQuery(Query* q, Structure const * const s, std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure) const;
	PredTable* solveBdd(const std::vector<Variable*>& vars, std::shared_ptr<FOBDDManager> manager, const FOBDD* bdd, Structure const * const structure) const;
	PredTable* solveBDDQuery(const FOBDD* b, Structure const * const s) const;
};

class TheoryComponent;
bool evaluate(TheoryComponent* form, const Structure* structure);
const DomainElement* evaluate(Term* term, const Structure* structure);
