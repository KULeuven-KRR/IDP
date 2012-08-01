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
#include "inferences/symmetrybreaking/Symmetry.hpp"
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

template<typename GroundingReceiver>
void fixTraceMonitor(TraceMonitor*, Grounder*, GroundingReceiver*) {
	return;
}
//Do nothing unless GroundingReciever is  PCSolver (see Grounding.cpp)
template<> void fixTraceMonitor(TraceMonitor* t, Grounder* grounder, PCSolver* solver);

void addSymmetryBreaking(AbstractTheory* theory, AbstractStructure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm);

//GroundingReciever can be a solver, a printmonitor, ...
template<typename GroundingReceiver>
class GroundingInference {
private:
	Theory* _theory;
	AbstractStructure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done
	GroundingReceiver* _receiver;
	Grounder* _grounder; //The grounder that is created by this inference. Is deleted together with the inference (for lazy grounding, can be needed when the ground method is finished)
	bool _prepared;
	bool _nbmodelsequivalent; //If true, the produced grounding will have as many models as the original theory, if false, the grounding might have more models.

public:
	//NOTE: modifies the theory and the structure. Clone before passing them!
	static std::shared_ptr<GroundingInference> createGroundingInference(AbstractTheory* theory, AbstractStructure* structure, Term* term,
			TraceMonitor* tracemonitor, bool nbModelsEquivalent, GroundingReceiver* solver) {
		if (theory == NULL || structure == NULL) {
			throw IdpException("Unexpected NULL-pointer.");
		}
		auto t = dynamic_cast<Theory*>(theory); // TODO handle other cases
		if (t == NULL) {
			throw notyetimplemented("Grounding of already ground theories");
		}
		if (t->vocabulary() != structure->vocabulary()) {
			throw IdpException("Grounding requires that the theory and structure range over the same vocabulary.");
		}
		auto m = std::shared_ptr<GroundingInference>(new GroundingInference(t, structure, term, tracemonitor, nbModelsEquivalent, solver));

		return m;
	}
	GroundingInference(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor, bool nbModelsEquivalent,
			GroundingReceiver* solver)
			: _theory(theory), _structure(structure), _tracemonitor(tracemonitor), _minimizeterm(minimize), _receiver(solver), _grounder(NULL),
				_prepared(false), _nbmodelsequivalent(nbModelsEquivalent) {
		auto voc = new Vocabulary("intern_voc"); // FIXME name uniqueness!
		voc->add(_theory->vocabulary());
		_structure->changeVocabulary(voc); // FIXME should move to the location where the clones are made!
		_theory->vocabulary(voc);
	}

	~GroundingInference() {
		if (_grounder != NULL) {
			delete _grounder;
		}
	}

	AbstractGroundTheory* returnUnsat(){
		if(getOption(IntType::VERBOSE_CREATE_GROUNDERS)>0 || getOption(IntType::VERBOSE_GROUNDING)>0){
			clog << "Unsat detected during grounding\n";
		}
		return NULL;
	}

	//Grounds the theory with the given structure
	AbstractGroundTheory* ground() {
		// Calculate known definitions
		if (getOption(BoolType::SHAREDTSEITIN)) {
			_theory = FormulaUtils::sharedTseitinTransform(_theory, _structure);
		}
		if (not getOption(BoolType::GROUNDLAZILY)) {
			if ( getOption(IntType::VERBOSE_GROUNDING) >= 1) {
				std::clog << "Evaluating definitions\n";
			}
			auto defCalculated = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(_theory), _structure);
			if (defCalculated.size() == 0) {
				return returnUnsat();
			}
			Assert(defCalculated[0]->isConsistent());
			_structure = defCalculated[0];
		}
		// Create grounder
		if ( getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			std::clog << "Approximation\n";
		}
		auto symstructure = generateBounds(_theory, _structure, getOption(BoolType::LIFTEDUNITPROPAGATION));
		if (not _structure->isConsistent()) {
			if ( getOption(IntType::VERBOSE_GROUNDING)  > 0 ||  getOption(IntType::VERBOSE_PROPAGATING)>0) {
				std::clog << "approximation detected UNSAT\n";
			}
			delete symstructure->manager();
			delete (symstructure);
			return returnUnsat();
		}
		if ( getOption(IntType::VERBOSE_GROUNDING)  >= 1) {
			std::clog << "Grounding\n";
		}
		if (_grounder != NULL) {
			delete (_grounder);
		}
		GroundInfo gi = { _theory, _minimizeterm, _structure, symstructure, _nbmodelsequivalent };
		if (_receiver == NULL) {
			_grounder = GrounderFactory::create(gi);
		} else {
			_grounder = GrounderFactory::create(gi, _receiver);
		}
		if (getOption(BoolType::TRACE)) {
			fixTraceMonitor(_tracemonitor, _grounder, _receiver);
		}
		// Run grounder
		_grounder->toplevelRun();
		auto grounding = _grounder->getGrounding();

		// Execute symmetry breaking
		addSymmetryBreaking(_theory, _structure, grounding, _minimizeterm);

		// Print grounding statistics
		if ( getOption(IntType::VERBOSE_GROUNDING)  > 0) {
			auto maxsize = _grounder->getFullGroundSize();
			//cout <<"full|grounded|%|time\n";
			//cout <<toString(maxsize) <<"|" <<toString(grounder->groundedAtoms()) <<"|";
			std::clog << "Grounded " << toString(_grounder->groundedAtoms()) << " for a full grounding of " << toString(maxsize) << "\n";
			if (maxsize._type == TableSizeType::TST_EXACT) {
				//cout <<(double)grounder->groundedAtoms()/maxsize._size*100 <<"\\%";
				std::clog << ">>> " << (double) (_grounder->groundedAtoms()) / maxsize._size * 100 << "% of the full grounding.\n";
			}
		}

		delete symstructure->manager();
		delete (symstructure);

		return grounding;
	}
};

#endif //GROUNDINGINFERENCE16514_HPP_
