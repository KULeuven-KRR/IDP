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

#include <cstdlib>
#include <memory>
#include <ctime>
#include "inferences/SolverInclude.hpp"

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
#include "grounders/Grounder.hpp"
#include "errorhandling/UnsatException.hpp"
#include "utils/LogActionTime.hpp"

class Theory;
class AbstractTheory;
class Structure;
class TraceMonitor;
class Term;
class AbstractGroundTheory;

template<typename GroundingReceiver>
void connectTraceMonitor(TraceMonitor*, Grounder*, GroundingReceiver*) {
}
//Do nothing unless GroundingReciever is PCSolver (see Grounding.cpp)
template<> void connectTraceMonitor(TraceMonitor* t, Grounder* grounder, PCSolver* solver);

void addSymmetryBreaking(AbstractTheory* theory, Structure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm, bool nbModelsEquivalent);


//GroundingReciever can be a solver, a printmonitor, ...
template<typename GroundingReceiver>
class GroundingInference {
private:
	Theory* _theory;
	Structure* _structure;
	Vocabulary* _outputvoc; //If not NULL, symbols outside this vocabulary are not relevant
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done
	GroundingReceiver* _receiver;
	Grounder* _grounder; //The grounder that is created by this inference. Is deleted together with the inference (for lazy grounding, can be needed when the ground method is finished)
	bool _prepared;
	bool _nbmodelsequivalent; //If true, the produced grounding will have as many models as the original theory, if false, the grounding might have more models.

public:
	// NOTE: modifies the theory and the structure. Clone before passing them!
	static AbstractGroundTheory* doGrounding(AbstractTheory* theory, Structure* structure, Term* term, TraceMonitor* tracemonitor,
			bool nbModelsEquivalent, GroundingReceiver* solver, Vocabulary* outputvoc = NULL) {
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
		auto m = new GroundingInference(t, structure, term, tracemonitor, nbModelsEquivalent, solver, outputvoc);
		auto grounding = m->ground();
		Assert(grounding!=NULL);
		// FIXME deleting lazy grounders here is a problem!!! delete(m);
		return grounding;
	}
private:
	GroundingInference(Theory* theory, Structure* structure, Term* minimize,  TraceMonitor* tracemonitor, bool nbModelsEquivalent,
			GroundingReceiver* solver, Vocabulary* outputvoc = NULL)
			: 	_theory(theory),
				_structure(structure),
				_outputvoc(outputvoc),
				_tracemonitor(tracemonitor),
				_minimizeterm(minimize),
				_receiver(solver),
				_grounder(NULL),
				_prepared(false),
				_nbmodelsequivalent(nbModelsEquivalent) {
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

	AbstractGroundTheory* returnUnsat(GroundInfo info, GroundingReceiver* receiver) {
		if (getOption(IntType::VERBOSE_CREATE_GROUNDERS) > 0 || getOption(IntType::VERBOSE_GROUNDING) > 0) {
			clog << "Unsat detected during grounding\n";
		}
		try {
			if(receiver==NULL){
				_grounder = GrounderFactory::create(info);
			}else{
				_grounder = GrounderFactory::create(info, receiver);
			}
			_grounder->getGrounding()->addEmptyClause();
		} catch (UnsatException&) {

		}
		return _grounder->getGrounding();
	}

	//Grounds the theory with the given structure
	AbstractGroundTheory* ground() {
		Assert(_grounder==NULL);

		if (getOption(BoolType::SHAREDTSEITIN)) {
			_theory = FormulaUtils::sharedTseitinTransform(_theory, _structure);
		}

		// Calculate known definitions
		if (not getOption(BoolType::SATISFIABILITYDELAY)) {
			if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
				logActionAndTime("Evaluating definitions");
			}
			auto defCalculated = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(_theory), _structure);
			if (defCalculated.size() == 0) {
				// TODO allow this without symbolic structure?
				bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
				bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
				auto symstructure = generateBounds(_theory, _structure, propagate, LUP, _outputvoc);
				auto grounding = returnUnsat(GroundInfo{_theory, {_structure, symstructure}, _nbmodelsequivalent, _minimizeterm}, _receiver);
				delete(symstructure);
				return grounding;
			}
			Assert(defCalculated[0]->isConsistent());
			_structure = defCalculated[0];
		}

		// Approximation
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Approximation");
		}
		bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
		bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
		auto symstructure = generateBounds(_theory, _structure, propagate, LUP);
		if (not _structure->isConsistent()) {
			if (getOption(IntType::VERBOSE_GROUNDING) > 0 || getOption(IntType::VERBOSE_PROPAGATING) > 0) {
				std::clog << "approximation detected UNSAT\n";
			}
			auto grounding = returnUnsat(GroundInfo{_theory, {_structure, symstructure}, _nbmodelsequivalent, _minimizeterm}, _receiver);
			delete (symstructure);
			return grounding;
		}
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Creating grounders");
		}
		auto gi = GroundInfo(_theory, {_structure, symstructure}, _nbmodelsequivalent, _minimizeterm);
		if (_receiver == NULL) {
			_grounder = GrounderFactory::create(gi);
		} else {
			_grounder = GrounderFactory::create(gi, _receiver);
		}

		if (getOption(BoolType::TRACE)) {
			connectTraceMonitor(_tracemonitor, _grounder, _receiver);
		}

		// Run grounder
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Grounding");
		}
		bool unsat = _grounder->toplevelRun();
		if(unsat){
			auto grounding = returnUnsat(GroundInfo { _theory, { _structure, symstructure }, _nbmodelsequivalent, _minimizeterm }, _receiver);
			delete (symstructure);
			return grounding;
		}

		addSymmetryBreaking(_theory, _structure, _grounder->getGrounding(), _minimizeterm, _nbmodelsequivalent);

		// Print grounding statistics
		if (getOption(IntType::VERBOSE_GROUNDING) > 0) {
			auto maxsize = _grounder->getFullGroundSize();
			//cout <<"full|grounded|%|time\n";
			//cout <<print(maxsize) <<"|" <<print(grounder->groundedAtoms()) <<"|";
			std::clog << "Grounded " << print(_grounder->groundedAtoms()) << " for a full grounding of " << print(maxsize) << "\n";
			if (maxsize._type == TableSizeType::TST_EXACT) {
				//cout <<(double)grounder->groundedAtoms()/maxsize._size*100 <<"\\%";
				std::clog << ">>> " << (double) (_grounder->groundedAtoms()) / maxsize._size * 100 << "% of the full grounding.\n";
			}
		}

		delete (symstructure);
		return _grounder->getGrounding();
	}
};
