/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INFERENCE_SOLVERCONN_HPP_
#define INFERENCE_SOLVERCONN_HPP_

#include "SolverInclude.hpp"
#include "commontypes.hpp"

class AbstractStructure;
class GroundTranslator;
class GroundTermTranslator;
class TraceMonitor;

namespace SolverConnection {
	MinisatID::AggType convert(AggFunction agg);
	MinisatID::EqType convert(CompType rel);
	MinisatID::Atom createAtom(const int lit);
	MinisatID::Literal createLiteral(const int lit);
	MinisatID::literallist createList(const litlist& origlist);
	MinisatID::Weight createWeight(double weight);

	// Note: default find all models
	PCSolver* createsolver(int nbmodels = 0);
	void setTranslator(PCSolver*, GroundTranslator* translator);
	PCModelExpand* initsolution(PCSolver*, int nbmodels);
	PCUnitPropagate* initpropsolution(PCSolver*);

	// Parse model into structure
	void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, AbstractStructure* init);

	// Parse cp-model into structure
	void addTerms(const MinisatID::Model& model, GroundTermTranslator* termtranslator, AbstractStructure* init);
}

#endif //INFERENCE_SOLVERCONN_HPP_
