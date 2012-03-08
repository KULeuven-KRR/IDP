/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "ModelExpansion.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "inferences/SolverConnection.hpp"

#include "inferences/symmetrybreaking/symmetry.hpp"

#include "theory/TheoryUtils.hpp"

#include "groundtheories/GroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/grounders/OptimizationTermGrounders.hpp"

#include "tracemonitor.hpp"

using namespace std;

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding);

class SolverTermination: public TerminateMonitor {
public:
	void notifyTerminateRequested() {
		requestTermination();
	}
};

std::vector<AbstractStructure*> ModelExpansion::expand() const {
	auto opts = GlobalData::instance()->getOptions();
	// Calculate known definitions
	// FIXME currently skipping if working lazily!
	auto clonetheory = theory->clone();
	AbstractStructure* newstructure = NULL;
	if (not opts->getValue(BoolType::GROUNDLAZILY) && sametypeid<Theory>(*clonetheory)) {
		newstructure = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(clonetheory), structure);
		if (not newstructure->isConsistent()) {
			delete(newstructure);
			return std::vector<AbstractStructure*> { };
		}
	}else{
		newstructure = structure->clone();
	}

	// Create solver and grounder
	auto solver = SolverConnection::createsolver(getOption(IntType::NBMODELS));
	if (verbosity() >= 1) {
		clog << "Approximation\n";
	}
	auto symstructure = generateBounds(clonetheory, newstructure);
	if (verbosity() >= 1) {
		clog << "Grounding\n";
	}
	auto grounder = GrounderFactory::create({clonetheory, newstructure, symstructure}, solver);
	SolverConnection::setTranslator(solver, grounder->getTranslator());
	if (getOption(BoolType::TRACE)) {
		tracemonitor->setTranslator(grounder->getTranslator());
		tracemonitor->setSolver(solver);
	}
	grounder->toplevelRun();
	auto grounding = grounder->getGrounding();

	// TODO refactor optimization!

	if(minimizeterm!=NULL){
		auto term = dynamic_cast<AggTerm*>(minimizeterm);
		if(term!=NULL){
			auto setgrounder = GrounderFactory::create(term->set(), {newstructure, symstructure}, grounding);
			auto optimgrounder = AggregateOptimizationGrounder(grounding, term->function(), setgrounder);
			optimgrounder.setOrig(minimizeterm);
			optimgrounder.run();
		}else{
			throw notyetimplemented("Optimization over non-aggregate terms.");
		}
	}

	// Execute symmetry breaking
	if (opts->getValue(IntType::SYMMETRY) != 0) {
		if (verbosity() >= 1) {
			clog << "Symmetry breaking\n";
		}
		auto ivsets = findIVSets(clonetheory, newstructure);
		if (opts->getValue(IntType::SYMMETRY) == 1) {
			addSymBreakingPredicates(grounding, ivsets);
		} else if (opts->getValue(IntType::SYMMETRY) == 2) {
			std::clog << "Dynamic symmetry breaking not yet implemented...\n";
//			for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
//				std::vector<std::map<int, int> > breakingSymmetries = (*ivsets_it)->getBreakingSymmetries(grounding);
//				for (auto bs_it = breakingSymmetries.cbegin(); bs_it != breakingSymmetries.cend(); ++bs_it) {
//					MinisatID::Symmetry symmetry;
//					for (auto s_it = bs_it->begin(); s_it != bs_it->end(); ++s_it) {
//						MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
//						MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
//						std::pair<MinisatID::Atom, MinisatID::Atom> entry = std::pair<MinisatID::Atom, MinisatID::Atom>(a1, a2);
//						symmetry.symmetry.insert(entry);
//					}
//					solver->add(symmetry);
//				}
//			}
		} else {
			std::clog << "Unknown symmetry option...\n";
		}
	}

	// Run solver
	auto abstractsolutions = SolverConnection::initsolution();
	if (verbosity() >= 1) {
		clog << "Solving\n";
	}
	getGlobal()->addTerminationMonitor(new SolverTermination());
	solver->solve(abstractsolutions);
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	//FIXME propagator code broken structure = propagator->currstructure(structure);
	std::vector<AbstractStructure*> solutions;
	if(minimizeterm!=NULL){ // Optimizing
		if(abstractsolutions->getModels().size()>0){
			solutions.push_back(handleSolution(newstructure, abstractsolutions->getBestModelFound(), grounding));
		}
	}else{
		if (verbosity() >= 1) {
			clog << "Solver generated " <<abstractsolutions->getModels().size() <<" models.\n";
		}
		for (auto model = abstractsolutions->getModels().cbegin(); model != abstractsolutions->getModels().cend(); ++model) {
			solutions.push_back(handleSolution(newstructure, **model, grounding));
		}
	}

	// Clean up: remove all objects that are only used here.
	delete (solver);
	grounding->recursiveDelete();
	// delete (grounder); TODO UNCOMMENT AND FIX MEM MANAG FOR BDDs
	delete (abstractsolutions);
	clonetheory->recursiveDelete();
	delete (newstructure);
	delete (symstructure);

	return solutions;
}

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding){
	auto newsolution = structure->clone();
	SolverConnection::addLiterals(model, grounding->translator(), newsolution);
	SolverConnection::addTerms(model, grounding->termtranslator(), newsolution);
	newsolution->clean();
	Assert(newsolution->isConsistent());
	return newsolution;
}
