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
#include "inferences/CalculateDefinitions.hpp"
#include "inferences/InferenceSolverConnection.hpp"

#include "commands/propagate.hpp"
#include "symmetry.hpp"

#include "utils/TheoryUtils.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/propagation/PropagatorFactory.hpp"

#include "external/TerminationManagement.hpp"

using namespace std;

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
	auto newstructure = structure;
	if (not opts->getValue(BoolType::GROUNDLAZILY) && sametypeid<Theory>(*theory)) {

		newstructure = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(theory), structure);
		if (not newstructure->isConsistent()) {
			return std::vector<AbstractStructure*> { };
		}
	}


	// Create solver and grounder
	SATSolver* solver = InferenceSolverConnection::createsolver();
	if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
		clog << "Approximation\n";
	}
	auto symstructure = generateNaiveApproxBounds(theory, newstructure);
	// TODO bugged! auto symstructure = generateApproxBounds(theory, structure);
	if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
		clog << "Grounding\n";
	}
	GrounderFactory grounderfactory(newstructure, symstructure);
	Grounder* grounder = grounderfactory.create(theory, solver);
	if (getOption(BoolType::TRACE)) {
		tracemonitor->setTranslator(grounder->getTranslator());
		tracemonitor->setSolver(solver);
	}
	grounder->toplevelRun();
	AbstractGroundTheory* grounding = grounder->getGrounding();

	// Execute symmetry breaking
	if (opts->getValue(IntType::SYMMETRY) != 0) {
		if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
			clog << "Symmetry breaking\n";
		}
		auto ivsets = findIVSets(theory, structure);
		if (opts->getValue(IntType::SYMMETRY) == 1) {
			addSymBreakingPredicates(grounding, ivsets);
		} else if (opts->getValue(IntType::SYMMETRY) == 2) {
			for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
				std::vector<std::map<int, int> > breakingSymmetries = (*ivsets_it)->getBreakingSymmetries(grounding);
				for (auto bs_it = breakingSymmetries.cbegin(); bs_it != breakingSymmetries.cend(); ++bs_it) {
					MinisatID::Symmetry symmetry;
					for (auto s_it = bs_it->begin(); s_it != bs_it->end(); ++s_it) {
						MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
						MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
						std::pair<MinisatID::Atom, MinisatID::Atom> entry = std::pair<MinisatID::Atom, MinisatID::Atom>(a1, a2);
						symmetry.symmetry.insert(entry);
					}
					solver->add(symmetry);
				}
			}
		} else {
			std::clog << "Unknown symmetry option...\n";
		}
	}

	// Run solver
	MinisatID::Solution* abstractsolutions = InferenceSolverConnection::initsolution();
	if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
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
	if (getOption(IntType::GROUNDVERBOSITY) >= 1) {
		clog << "Generate 2-valued models\n";
	}
	for (auto model = abstractsolutions->getModels().cbegin(); model != abstractsolutions->getModels().cend(); ++model) {
		AbstractStructure* newsolution = newstructure->clone();
		InferenceSolverConnection::addLiterals(*model, grounding->translator(), newsolution);
		InferenceSolverConnection::addTerms(*model, grounding->termtranslator(), newsolution);
		newsolution->clean();
		solutions.push_back(newsolution);
		Assert(newsolution->isConsistent());
	}

	grounding->recursiveDelete();
	delete (solver);
	delete (abstractsolutions);

	return solutions;
}
