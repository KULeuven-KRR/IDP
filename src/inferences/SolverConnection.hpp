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

#include "SolverInclude.hpp"
#include "commontypes.hpp"

class Structure;
class GroundTranslator;
class TraceMonitor;

namespace SolverConnection {
	MinisatID::VarID convert(VarId varid);
	MinisatID::AggType convert(AggFunction agg);
	MinisatID::EqType convert(CompType rel);
	MinisatID::Weight createWeight(double weight);

	// Note: default find all models
	PCSolver* createsolver(int nbmodels = 0);
	void setTranslator(PCSolver*, GroundTranslator* translator);
	MinisatID::ModelExpand* initsolution(PCSolver*, int nbmodels, const litlist& assumptions = litlist());
        MinisatID::ModelIterationTask* createIteratorSolution(PCSolver*, int nbmodels, const litlist& assumptions = litlist());
	PCUnitPropagate* initpropsolution(PCSolver*);

	// Parse model into structure
	void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, Structure* init);

	// Parse cp-model into structure
	void addTerms(const MinisatID::Model& model, GroundTranslator* termtranslator, Structure* init);

	MinisatID::literallist createList(const litlist& origlist);
	MinisatID::Atom createAtom(const int lit);
	MinisatID::Lit createLiteral(const int lit);
}
