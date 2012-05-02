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

#include "inferences/SolverInclude.hpp"

#include "theory/TheoryUtils.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "fobdds/FoBddManager.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/grounders/OptimizationTermGrounders.hpp"

#include "TraceMonitor.hpp"

using namespace std;

std::vector<AbstractStructure*> ModelExpansion::doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, Vocabulary* outputvoc, TraceMonitor* tracemonitor) {
	auto m = createMX(theory, structure, NULL, outputvoc, tracemonitor);
	return m->expand();
}
std::vector<AbstractStructure*> ModelExpansion::doMinimization(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvoc,TraceMonitor* tracemonitor) {
	if (term == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	auto m = createMX(theory, structure, term, outputvoc, tracemonitor);
	return m->expand();
}

shared_ptr<ModelExpansion> ModelExpansion::createMX(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvoc,TraceMonitor* tracemonitor){
	if (theory == NULL || structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	auto t = dynamic_cast<Theory*>(theory); // TODO handle other cases
	if (t == NULL) {
		throw notyetimplemented("Modelexpansion of already ground theories.\n");
	}
	if (t->vocabulary() != structure->vocabulary()) {
		throw IdpException("Modelexpansion requires that the theory and structure range over the same vocabulary.");
	}
	auto m = shared_ptr<ModelExpansion>(new ModelExpansion(t, structure, term, tracemonitor));
	if(outputvoc!=NULL){
		m->setOutputVocabulary(outputvoc);
	}
	return m;
}

ModelExpansion::ModelExpansion(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor)
		: 	theory(theory),
			structure(structure),
			tracemonitor(tracemonitor),
			minimizeterm(minimize),
			outputvoc(NULL) {
}

void ModelExpansion::setOutputVocabulary(Vocabulary* v) {
	if (VocabularyUtils::isSubVocabulary(v, theory->vocabulary())) {
		throw IdpException("The output-vocabulary of model expansion can only be a subvocabulary of the theory.");
	}
	outputvoc = v;
}

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding);

class SolverTermination: public TerminateMonitor {
private:
	PCModelExpand* solver;
public:
	SolverTermination(PCModelExpand* solver)
			: solver(solver) {

	}
	void notifyTerminateRequested() {
		solver->notifyTerminateRequested();
	}
};

std::vector<AbstractStructure*> ModelExpansion::expand() const {
	auto opts = GlobalData::instance()->getOptions();
	// Calculate known definitions
	auto clonetheory = theory->clone();
	Assert(sametypeid<Theory>(*clonetheory));
	AbstractStructure* newstructure = NULL;
	if (not opts->getValue(BoolType::GROUNDLAZILY)) {
		if (verbosity() >= 1) {
			clog << "Evaluating definitions\n";
		}
		auto defCalculated = CalculateDefinitions::doCalculateDefinitions(dynamic_cast<Theory*>(clonetheory), structure);
		if (defCalculated.size() == 0) {
			delete (newstructure);
			clonetheory->recursiveDelete();
			return std::vector<AbstractStructure*> { };
		}
		Assert(defCalculated[0]->isConsistent());
		newstructure = defCalculated[0];
	} else {
		newstructure = structure->clone();
	}
	// Create solver and grounder
	auto data = SolverConnection::createsolver(getOption(IntType::NBMODELS));
	if (verbosity() >= 1) {
		clog << "Approximation\n";
	}
	auto symstructure = generateBounds(clonetheory, newstructure);
	if (not newstructure->isConsistent()) {
		clonetheory->recursiveDelete();
		delete (newstructure);
		delete symstructure->manager();
		delete (symstructure);
		return std::vector<AbstractStructure*> { };
	}

	if (verbosity() >= 1) {
		clog << "Grounding\n";
	}
	auto grounder = GrounderFactory::create( { clonetheory, newstructure, symstructure }, data);
	if (getOption(BoolType::TRACE)) {
		tracemonitor->setTranslator(grounder->getTranslator());
		tracemonitor->setSolver(data);
	}
	grounder->toplevelRun();

	auto grounding = grounder->getGrounding();

	if (minimizeterm != NULL) {
		auto term = dynamic_cast<AggTerm*>(minimizeterm);
		if (term != NULL) {
			auto setgrounder = GrounderFactory::create(term->set(), { newstructure, symstructure }, grounding);
			auto optimgrounder = AggregateOptimizationGrounder(grounding, term->function(), setgrounder);
			optimgrounder.setOrig(minimizeterm);
			optimgrounder.run();
		} else {
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
	auto mx = SolverConnection::initsolution(data, getOption(NBMODELS));
	if (verbosity() > 0) {
		clog << "Solving\n";
	}
	auto terminator = new SolverTermination(mx);
	getGlobal()->addTerminationMonitor(terminator);
	try {
		// TODO here, make all ground atoms in the output vocabulary decision vars. (and set the option that not everything has to be decided)
		mx->execute(); // FIXME wrap other solver calls also in try-catch
	} catch (MinisatID::idpexception& error) {
		std::stringstream ss;
		ss << "Solver was aborted with message \"" << error.what() << "\"";
		throw IdpException(ss.str());
	}

	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	if (verbosity() > 0) {
		auto maxsize = grounder->getFullGroundSize();
		//cout <<"full|grounded|%|time\n";
		//cout <<toString(maxsize) <<"|" <<toString(grounder->groundedAtoms()) <<"|";
		clog << "Grounded " << toString(grounder->groundedAtoms()) << " for a full grounding of " << toString(maxsize) << "\n";
		if (maxsize._type == TableSizeType::TST_EXACT) {
			//cout <<(double)grounder->groundedAtoms()/maxsize._size*100 <<"\\%";
			clog << ">>> " << (double) grounder->groundedAtoms() / maxsize._size << "% of the full grounding.\n";
		}
		cout << "|";
	}

	// Collect solutions
	auto abstractsolutions = mx->getSolutions();
	//FIXME propagator code broken structure = propagator->currstructure(structure);
	std::vector<AbstractStructure*> solutions;
	if (minimizeterm != NULL) { // Optimizing
		if (abstractsolutions.size() > 0) {
			Assert(mx->getBestSolutionsFound().size()>0);
			auto list = mx->getBestSolutionsFound();
			for (auto i = list.cbegin(); i < list.cend(); ++i) {
				solutions.push_back(handleSolution(newstructure, **i, grounding));
			}
		}
	} else {
		if (verbosity() > 0) {
			clog << "Solver generated " << abstractsolutions.size() << " models.\n";
		}
		for (auto model = abstractsolutions.cbegin(); model != abstractsolutions.cend(); ++model) {
			solutions.push_back(handleSolution(newstructure, **model, grounding));
		}
	}

	// Clean up: remove all objects that are only used here.
	grounding->recursiveDelete();
	delete (grounder);
	clonetheory->recursiveDelete();
	getGlobal()->removeTerminationMonitor(terminator);
	delete (terminator);
	delete (newstructure);
	delete symstructure->manager();
	delete (symstructure);
	delete (data);
	delete (mx);

	return solutions;
}

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding) {
	auto newsolution = structure->clone();
	SolverConnection::addLiterals(model, grounding->translator(), newsolution);
	SolverConnection::addTerms(model, grounding->termtranslator(), newsolution);
	newsolution->clean();
	Assert(newsolution->isConsistent());
	return newsolution;
}
