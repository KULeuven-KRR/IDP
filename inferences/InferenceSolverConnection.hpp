/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INFERENCESOLVERCONN_HPP_
#define INFERENCESOLVERCONN_HPP_

#include "groundtheories/SolverPolicy.hpp"

class AbstractStructure;
class GroundTranslator;
class GroundTermTranslator;
class TraceMonitor;
namespace MinisatID {
	class Solution;
	class Model;
	class WrappedPCSolver;
}

namespace InferenceSolverConnection {
	SATSolver* createsolver();
	MinisatID::Solution* initsolution();

	// Parse model into structure
	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init);
	// Parse cp-model into structure
	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init);
}

#endif //INFERENCESOLVERCONN_HPP_
