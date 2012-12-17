/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
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
#include "LazyGroundingManager.hpp"
#include "utils/LogAction.hpp"

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

void addSymmetryBreaking(AbstractTheory* theory, Structure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm,
		bool nbModelsEquivalent);

//GroundingReciever can be a solver, a printmonitor, ...
template<typename GroundingReceiver>
class GroundingInference {
private:
	Theory* _theory;
	Structure* _structure;
	Vocabulary* _outputvocabulary; //If not NULL, symbols outside this vocabulary are not relevant
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done
	GroundingReceiver* _receiver;
	LazyGroundingManager* _grounder; //The grounder that is created by this inference. Is deleted together with the inference (for lazy grounding, can be needed when the ground method is finished)
	bool _prepared;
	bool _nbmodelsequivalent; //If true, the produced grounding will have as many models as the original theory, if false, the grounding might have more models.

public:
	//NOTE: modifies the theory and the structure. Clone before passing them!
	static AbstractGroundTheory* doGrounding(AbstractTheory* theory, Structure* structure, Vocabulary* outputvocabulary, Term* term,
			TraceMonitor* tracemonitor, bool nbModelsEquivalent, GroundingReceiver* solver) {
		return createGroundingAndExtender(theory, structure, outputvocabulary, term, tracemonitor, nbModelsEquivalent, solver).first;
	}
	static std::pair<AbstractGroundTheory*, StructureExtender*> createGroundingAndExtender(AbstractTheory* theory, Structure* structure,
			Vocabulary* outputvocabulary, Term* term, TraceMonitor* tracemonitor, bool nbModelsEquivalent, GroundingReceiver* solver) {
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
		auto m = new GroundingInference(t, structure, outputvocabulary, term, tracemonitor, nbModelsEquivalent, solver);
		auto grounding = m->ground();
		Assert(grounding!=NULL);
		// FIXME deleting lazy grounders here is a problem!!! delete(m);
		return {grounding, m->getManager()};
	}
private:
	GroundingInference(Theory* theory, Structure* structure, Vocabulary* outputvocabulary, Term* minimize, TraceMonitor* tracemonitor,
			bool nbModelsEquivalent, GroundingReceiver* solver)
			: 	_theory(theory),
				_structure(structure),
				_outputvocabulary(outputvocabulary),
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
	}

	LazyGroundingManager* getManager() const {
		return _grounder;
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
		auto satdelay = getOption(SATISFIABILITYDELAY);
		setOption(SATISFIABILITYDELAY, false);
		auto tseitindelay = getOption(TSEITINDELAY);
		setOption(TSEITINDELAY, false);
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Starting definition evaluation at ");
		}
		auto defCalculated = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(_theory), _structure, satdelay);
		if(getOption(VERBOSE_GROUNDSTATS) > 1){
			cout <<"\ndefs&&max:" <<toDouble(Grounder::getFullGroundingSize()) <<"&&grounded:" <<Grounder::groundedAtoms() <<"\n";
		}
		if (defCalculated.size() == 0) {
			return returnUnsat(GroundInfo { _theory, { _structure, NULL }, _outputvocabulary, _nbmodelsequivalent, _minimizeterm }, _receiver);
		}
		Assert(defCalculated[0]->isConsistent());
		_structure = defCalculated[0];
		setOption(SATISFIABILITYDELAY, satdelay);
		setOption(TSEITINDELAY, tseitindelay);

		// Approximation
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Starting approximation at ");
		}
		bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
		bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
		auto symstructure = generateBounds(_theory, _structure, propagate, LUP, _outputvocabulary);
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			clog <<"Symbolic structure = " <<toString(symstructure) <<"\n";
		}

		if (not _structure->isConsistent()) {
			if (getOption(IntType::VERBOSE_GROUNDING) > 0 || getOption(IntType::VERBOSE_PROPAGATING) > 0) {
				std::clog << "approximation detected UNSAT\n";
			}
			return returnUnsat(GroundInfo { _theory, { _structure, symstructure }, _outputvocabulary, _nbmodelsequivalent, _minimizeterm }, _receiver);
		}
		if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
			logActionAndTime("Creating grounders at ");
		}
		auto gi = GroundInfo(_theory, { _structure, symstructure }, _outputvocabulary, _nbmodelsequivalent, _minimizeterm);
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
			logActionAndTime("Starting grounding at ");
		}
		if (getOption(VERBOSE_GROUNDSTATS) >= 1) {
			logActionAndTime("grounding-start");
		}
		bool unsat = _grounder->toplevelRun();
		if(unsat){
			auto grounding = returnUnsat(GroundInfo { _theory, { _structure, symstructure }, _outputvocabulary, _nbmodelsequivalent, _minimizeterm }, _receiver);
			delete (symstructure);
			return grounding;
		}

		addSymmetryBreaking(_theory, _structure, _grounder->getGrounding(), _minimizeterm, _nbmodelsequivalent);

		if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
			std::clog <<"groundsize&&" <<_grounder->getGrounding()->getSize() <<"\n";
		}

		return _grounder->getGrounding();
	}
};
