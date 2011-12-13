/************************************
	ModelExpansion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INFERENCESOLVERCONN_HPP_
#define INFERENCESOLVERCONN_HPP_

#include "groundtheories/SolverPolicy.hpp"


class AbstractStructure;
class GroundTranslator;
class GroundTermTranslator;
class TraceMonitor;
namespace MinisatID{
	class Solution;
	class Model;
	class WrappedPCSolver;
}


namespace InferenceSolverConnection {
	SATSolver* createsolver();

	MinisatID::Solution* initsolution();

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init);

	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init);
};

#endif //INFERENCESOLVERCONN_HPP_
