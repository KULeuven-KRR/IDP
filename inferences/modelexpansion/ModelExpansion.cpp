#include <ctime>
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

class SolverTermination: public TerminateMonitor{
public:
	void notifyTerminateRequested(){
		requestTermination();
	}
};

std::vector<AbstractStructure*> ModelExpansion::expand() const {
	auto opts = GlobalData::instance()->getOptions();
	// TODO Option::pushOptions(options);

	// Calculate known definitions
	// FIXME currently skipping if working lazily!
	if (not opts->getValue(BoolType::GROUNDLAZILY) && sametypeid<Theory>(*theory)) {

		bool satisfiable = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(theory), structure);
		if (not satisfiable) {
			return std::vector<AbstractStructure*> { };
		}
	}

	// Create solver and grounder
	SATSolver* solver = InferenceSolverConnection::createsolver();
	if(getOption(IntType::GROUNDVERBOSITY)>=1){
		clog <<"Approximation\n";
	}
	auto symstructure = generateNaiveApproxBounds(theory, structure);
	// TODO bugged! auto symstructure = generateApproxBounds(theory, structure);
	if(getOption(IntType::GROUNDVERBOSITY)>=1){
		clog <<"Grounding\n";
	}
	GrounderFactory grounderfactory(structure, symstructure);
	Grounder* grounder = grounderfactory.create(theory, solver);
	if (getOption(BoolType::TRACE)) {
		tracemonitor->setTranslator(grounder->getTranslator());
		tracemonitor->setSolver(solver);
	}
	grounder->toplevelRun();
	AbstractGroundTheory* grounding = grounder->getGrounding();

	// Execute symmetry breaking
	if (opts->getValue(IntType::SYMMETRY) != 0) {
		if(getOption(IntType::GROUNDVERBOSITY)>=1){
			clog <<"Symmetry breaking\n";
		}
		clock_t start = clock();
		auto ivsets = findIVSets(theory, structure);
		float time = (float) (clock() - start) / CLOCKS_PER_SEC;
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
	if(getOption(IntType::GROUNDVERBOSITY)>=1){
		clog <<"Solving\n";
	}
	getGlobal()->addTerminationMonitor(new SolverTermination());
	solver->solve(abstractsolutions);
	if(getGlobal()->terminateRequested()){
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	//FIXME propagator code broken structure = propagator->currstructure(structure);
	std::vector<AbstractStructure*> solutions;
	if(getOption(IntType::GROUNDVERBOSITY)>=1){
		clog <<"Generate 2-valued models\n";
	}
	for (auto model = abstractsolutions->getModels().cbegin(); model != abstractsolutions->getModels().cend(); ++model) {
		AbstractStructure* newsolution = structure->clone();
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
