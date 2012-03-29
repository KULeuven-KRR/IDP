/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INFERENCESOLVERCONN_HPP_
#define INFERENCESOLVERCONN_HPP_

#include "SolverInclude.hpp"

class AbstractStructure;
class GroundTranslator;
class GroundTermTranslator;
class TraceMonitor;

namespace MinisatID {
	class Solution;
	class Model;
	class WrappedPCSolver;
}

namespace SolverConnection {
	// Note: default find all models
	PCSolver* createsolver(int nbmodels = 0);
	void setTranslator(PCSolver*, GroundTranslator* translator);
	PCModelExpand* initsolution(PCSolver*, int nbmodels);
	PCModelExpand* initpropsolution(PCSolver*, int nbmodels);

	// Parse model into structure
	void addLiterals(const MinisatID::Model& model, GroundTranslator* translator, AbstractStructure* init);

	// Parse cp-model into structure
	void addTerms(const MinisatID::Model& model, GroundTermTranslator* termtranslator, AbstractStructure* init);
}

#endif //INFERENCESOLVERCONN_HPP_
