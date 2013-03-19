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

#include "ModelExpansion.hpp"
#include "inferences/SolverConnection.hpp"

#include "inferences/grounding/Grounding.hpp"
#include "inferences/SolverInclude.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "errorhandling/error.hpp"

using namespace std;

MXResult ModelExpansion::doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor) {
	auto m = createMX(theory, structure, NULL, outputvoc, tracemonitor);
	return m->expand();
}
MXResult ModelExpansion::doMinimization(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor) {
	if (term == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (not term->freeVars().empty()) {
		throw IdpException("Cannot minimize over term with free variables.");
	}
	auto m = createMX(theory, structure, term, outputvoc, tracemonitor);
	return m->expand();
}

shared_ptr<ModelExpansion> ModelExpansion::createMX(AbstractTheory* theory, AbstractStructure* structure, Term* term, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor) {
	if (theory == NULL || structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	auto t = dynamic_cast<Theory*>(theory); // TODO handle other cases
	if (t == NULL) {
		throw notyetimplemented("Modelexpansion of already ground theories");
	}
	if(structure->vocabulary()!=theory->vocabulary()){
		throw IdpException("Modelexpansion requires that the theory and structure range over the same vocabulary.");
	}
	if(term!=NULL && structure->vocabulary()!=term->vocabulary()){
		throw IdpException("Modelexpansion requires that the minimization term and the structure range over the same vocabulary.");
	}
	auto m = shared_ptr<ModelExpansion>(new ModelExpansion(t, structure, term, tracemonitor));
	if (outputvoc != NULL) {
		m->setOutputVocabulary(outputvoc);
	}
	if (getGlobal()->getOptions()->symmetryBreaking() != SymmetryBreaking::NONE && getOption(NBMODELS) != 1) {
		Warning::warning("Cannot generate models symmetrical to models already found! More models might exist.");
	}
	return m;
}

ModelExpansion::ModelExpansion(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor)
		: 	_theory(theory),
			_structure(structure),
			_tracemonitor(tracemonitor),
			_minimizeterm(minimize),
			_outputvoc(NULL) {
}

void ModelExpansion::setOutputVocabulary(Vocabulary* v) {
	if (VocabularyUtils::isSubVocabulary(v, _theory->vocabulary())) {
		throw IdpException("The output-vocabulary of model expansion can only be a subvocabulary of the theory.");
	}
	_outputvoc = v;
}

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, Vocabulary* inputvoc);

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

MXResult ModelExpansion::expand() const {
	auto data = SolverConnection::createsolver(getOption(IntType::NBMODELS));
	auto inputvoc = _theory->vocabulary();
	auto clonetheory = _theory->clone();
	auto newstructure = _structure->clone();
	auto grounding = GroundingInference<PCSolver>::doGrounding(clonetheory, newstructure, _minimizeterm,  _tracemonitor,
			getOption(IntType::NBMODELS) != 1, data, _outputvoc);

	// Run solver
	auto mx = SolverConnection::initsolution(data, getOption(NBMODELS));
	if (getOption(IntType::VERBOSE_SOLVING) > 0) {
		logActionAndTime("Solving");
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

	bool optimumFound = true;

	if (getGlobal()->terminateRequested()) {
		if(mx->getSpace()->isOptimizationProblem()){
			optimumFound = false;
			Warning::warning("Optimization inference interrupted: will continue with the (single!) best model found to date (if any).");
			getGlobal()->reset();
		}else{
			throw IdpException("Solver was terminated");
		}
	}

	// Collect solutions
	std::vector<AbstractStructure*> solutions;
	if (_minimizeterm != NULL) { // Optimizing
		if (mx->getSolutions().size() > 0) {
			Assert(mx->getBestSolutionsFound().size()>0);
			auto list = mx->getBestSolutionsFound();
			auto value = mx->getBestValueFound();
			if (getOption(IntType::VERBOSE_SOLVING) > 0) {
				stringstream ss;
				ss <<"The best value found was " <<value <<"\n";
				ss <<"Solver generated " << list.size() << " models";
				logActionAndTime(ss.str());
			}
			for (auto i = list.cbegin(); i < list.cend(); ++i) {
				solutions.push_back(handleSolution(newstructure, **i, grounding, inputvoc));
			}
		}
	} else {
		auto abstractsolutions = mx->getSolutions();
		if (getOption(IntType::VERBOSE_SOLVING)  > 0) {
			stringstream ss;
			ss <<"Solver generated " << abstractsolutions.size() << " models";
			logActionAndTime(ss.str());
		}
		for (auto model = abstractsolutions.cbegin(); model != abstractsolutions.cend(); ++model) {
			solutions.push_back(handleSolution(newstructure, **model, grounding, inputvoc));
		}
	}

	// Clean up: remove all objects that are only used here.
	grounding->recursiveDelete();
	clonetheory->recursiveDelete();
	getGlobal()->removeTerminationMonitor(terminator);
	delete (terminator);
	delete (newstructure);
	delete (data);
	delete (mx);
	MXResult result;
	result._models = solutions;
	result._optimumfound = optimumFound;
	return result;
}

AbstractStructure* handleSolution(AbstractStructure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, Vocabulary* inputvoc) {
	auto newsolution = structure->clone();
	SolverConnection::addLiterals(model, grounding->translator(), newsolution);
	SolverConnection::addTerms(model, grounding->translator(), newsolution);
	newsolution->changeVocabulary(inputvoc); // Project onto input vocabulary
	newsolution->clean();
	Assert(newsolution->isConsistent());
	return newsolution;
}
