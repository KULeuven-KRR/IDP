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

#ifndef INFERENCE_SOLVERCONN_HPP_
#define INFERENCE_SOLVERCONN_HPP_

#include "SolverInclude.hpp"
#include "commontypes.hpp"

class Structure;
class GroundTranslator;
class TraceMonitor;

namespace SolverConnection {
	uint	getDefConstrID();
	MinisatID::VarID convert(VarId varid);
	MinisatID::AggType convert(AggFunction agg);
	MinisatID::EqType convert(CompType rel);
	MinisatID::Atom createAtom(const int lit);
	MinisatID::Lit createLiteral(const int lit);
	MinisatID::literallist createList(const litlist& origlist);
	MinisatID::Weight createWeight(double weight);

	// Note: default find all models
	PCSolver* createsolver(int nbmodels = 0);
	void setTranslator(PCSolver*, GroundTranslator* translator);
	PCModelExpand* initsolution(PCSolver*, int nbmodels);
	PCUnitPropagate* initpropsolution(PCSolver*);

	// Parse model into structure
	void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, Structure* init);

	// Parse cp-model into structure
	void addTerms(const MinisatID::Model& model, GroundTranslator* termtranslator, Structure* init);

	bool useUFSAndOnlyIfSem();
}

#endif //INFERENCE_SOLVERCONN_HPP_
