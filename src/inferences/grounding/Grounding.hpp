/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#ifndef GROUNDINGINFERENCE16514_HPP_
#define GROUNDINGINFERENCE16514_HPP_

#include <cstdlib>
#include <memory>
#include "inferences/SolverInclude.hpp"

#include "Grounding.hpp"
#include "GrounderFactory.hpp"
#include "common.hpp"
#include "options.hpp"
#include "GlobalData.hpp"
#include "inferences/symmetrybreaking/symmetry.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"

class Theory;
class AbstractTheory;
class AbstractStructure;
class TraceMonitor;
class Term;
class AbstractGroundTheory;

template<typename GroundingReciever>
void fixTraceMonitor(TraceMonitor* t, Grounder* grounder, GroundingReciever* something) {
	return; //Do nothing unless GroundingReciever is  PCSolver (see Grounding.cpp)
}

void addSymmetryBreaking(AbstractTheory* theory, AbstractStructure* structure, AbstractGroundTheory* grounding);

//GroundingReciever can be a solver, a printmonitor, ...
template<typename GroundingReciever>
class GroundingInference {
private:
	Theory* _theory;
	AbstractStructure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done
	GroundingReciever* _reciever;
	bool _prepared;

public:
	//NOTE: modifies the theory and the structure. Clone before passing them!
	static std::shared_ptr<GroundingInference> createGroundingInference(AbstractTheory* theory, AbstractStructure* structure, Term* term,
			TraceMonitor* tracemonitor, GroundingReciever* solver) {

		if (theory == NULL || structure == NULL) {
			throw IdpException("Unexpected NULL-pointer.");
		}
		auto t = dynamic_cast<Theory*>(theory); // TODO handle other cases
		if (t == NULL) {
			throw notyetimplemented("Grounding of already ground theories.\n");
		}
		if (t->vocabulary() != structure->vocabulary()) {
			throw IdpException("Grounding requires that the theory and structure range over the same vocabulary.");
		}
		auto m = std::shared_ptr<GroundingInference>(new GroundingInference(t, structure, term, tracemonitor, solver));

		return m;
	}
	GroundingInference(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor, GroundingReciever* solver)
			: 	_theory(theory),
				_structure(structure),
				_tracemonitor(tracemonitor),
				_minimizeterm(minimize),
				_reciever(solver),
				_prepared(false) {
	}

//Grounds the theory with the given structure
	AbstractGroundTheory* ground() {
		// Calculate known definitions
		if (getOption(BoolType::SHAREDTSEITIN)) {
			_theory = FormulaUtils::sharedTseitinTransform(_theory, _structure);
			_structure->changeVocabulary(_theory->vocabulary());
		}
		if (not getOption(BoolType::GROUNDLAZILY)) {
			if (verbosity() >= 1) {
				std::clog << "Evaluating definitions\n";
			}
			auto defCalculated = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(_theory), _structure);
			if (defCalculated.size() == 0) {
				return NULL;
			}
			Assert(defCalculated[0]->isConsistent());
			_structure = defCalculated[0];
		}
		// Creategrounder
		if (verbosity() >= 1) {
			std::clog << "Approximation\n";
		}
		auto symstructure = generateBounds(_theory, _structure);
		if (not _structure->isConsistent()) {
			if (verbosity() > 0) {
				std::clog << "approximation detected UNSAT\n";
			}
			delete symstructure->manager();
			delete (symstructure);
			return NULL;
		}
		if (verbosity() >= 1) {
			std::clog << "Grounding\n";
		}
		Grounder* grounder;
		if (_reciever == NULL) {
			grounder = GrounderFactory::create(GroundInfo { _theory, _structure, symstructure });
		} else {
			grounder = GrounderFactory::create( { _theory, _structure, symstructure }, _reciever);
		}
		if (getOption(BoolType::TRACE)) {
			fixTraceMonitor(_tracemonitor, grounder, _reciever);
		}
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();
		if (_minimizeterm != NULL) {
			auto optimgrounder = GrounderFactory::create(_minimizeterm, _theory->vocabulary(), GroundStructureInfo { _structure, symstructure }, grounding);
			optimgrounder->toplevelRun();
		}
		// Execute symmetry breaking
		addSymmetryBreaking(_theory, _structure, grounding);
		if (verbosity() > 0) {
			auto maxsize = grounder->getFullGroundSize();
			//cout <<"full|grounded|%|time\n";
			//cout <<toString(maxsize) <<"|" <<toString(grounder->groundedAtoms()) <<"|";
			std::clog << "Grounded " << toString(grounder->groundedAtoms()) << " for a full grounding of " << toString(maxsize) << "\n";
			if (maxsize._type == TableSizeType::TST_EXACT) {
				//cout <<(double)grounder->groundedAtoms()/maxsize._size*100 <<"\\%";
				std::clog << ">>> " << (double) (grounder->groundedAtoms()) / maxsize._size * 100 << "% of the full grounding.\n";
			}
			std::cout << "|";
		}

		delete (grounder);
		delete symstructure->manager();
		delete (symstructure);

		return grounding;
	}

};

#endif //GROUNDINGINFERENCE16514_HPP_