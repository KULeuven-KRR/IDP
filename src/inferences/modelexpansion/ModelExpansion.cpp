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

#include "structure/StructureComponents.hpp"

#include "inferences/grounding/Grounding.hpp"
#include "inferences/SolverInclude.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "errorhandling/error.hpp"
#include "creation/cppinterface.hpp"
#include "utils/Timer.hpp"

using namespace std;

MXResult ModelExpansion::doModelExpansion(AbstractTheory* theory, Structure* structure, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse) {
	auto m = createMX(theory, structure, NULL, outputvoc, tracemonitor, assumeFalse);
	return m->expand();
}
MXResult ModelExpansion::doMinimization(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse) {
	if (term == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (not term->freeVars().empty()) {
		throw IdpException("Cannot minimized over term with free variables.");
	}
	auto m = createMX(theory, structure, term, outputvoc, tracemonitor, assumeFalse);
	return m->expand();
}

shared_ptr<ModelExpansion> ModelExpansion::createMX(AbstractTheory* theory, Structure* structure, Term* term, Vocabulary* outputvoc,
		TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse) {
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
	auto m = shared_ptr<ModelExpansion>(new ModelExpansion(t, structure, term, tracemonitor, assumeFalse));
	m->setOutputVocabulary(outputvoc);
	if (getGlobal()->getOptions()->symmetryBreaking() != SymmetryBreaking::NONE && getOption(NBMODELS) != 1) {
		Warning::warning("Cannot generate models symmetrical to models already found! More models might exist.");
	}
	return m;
}

ModelExpansion::ModelExpansion(Theory* theory, Structure* structure, Term* minimize, TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse)
		: 	_theory(theory),
			_structure(structure),
			_tracemonitor(tracemonitor),
			_minimizeterm(minimize),
			_outputvoc(NULL),
			_assumeFalse(assumeFalse){
}

void ModelExpansion::setOutputVocabulary(Vocabulary* v) {
	if (not VocabularyUtils::isSubVocabulary(v, _theory->vocabulary())) {
		throw IdpException("The output-vocabulary of model expansion can only be a subvocabulary of the theory.");
	}
	_outputvoc = v;
}

Structure* handleSolution(Structure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, StructureExtender* extender,
		Vocabulary* outputvoc);

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

#define cleanup \
		grounding->recursiveDelete();\
		clonetheory->recursiveDelete();\
		getGlobal()->removeTerminationMonitor(terminator);\
		delete (extender);\
		delete (terminator);\
		delete (newstructure);\
		delete (voc); \
		delete (data);\
		delete (mx);

MXResult ModelExpansion::expand() const {
	auto data = SolverConnection::createsolver(getOption(IntType::NBMODELS));
	auto targetvoc = _outputvoc == NULL ? _theory->vocabulary() : _outputvoc;
	auto clonetheory = _theory->clone();
	auto newstructure = _structure->clone();
	stringstream ss;
	ss <<"mx_voc" <<getGlobal()->getNewID();
	auto voc = new Vocabulary(ss.str());
	voc->add(clonetheory->vocabulary());
	voc->add(newstructure->vocabulary());
	newstructure->changeVocabulary(voc);
	clonetheory->vocabulary(voc);

	//TODO See Issue #523 DefinitionUtils::splitDefinitions(clonetheory);
	std::pair<AbstractGroundTheory*, StructureExtender*> groundingAndExtender = {NULL, NULL};
	try{
		groundingAndExtender = GroundingInference<PCSolver>::createGroundingAndExtender(clonetheory, newstructure, _outputvoc, _minimizeterm, _tracemonitor, getOption(IntType::NBMODELS) != 1, data);
	}catch(...){
		if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
			logActionAndValue("effective-size", Grounder::groundedAtoms());
		}
		throw;
	}
	auto grounding = groundingAndExtender.first;
	auto extender = groundingAndExtender.second;

	litlist assumptions;
	for(auto p: _assumeFalse.assumeAllFalse){
		for(auto atom: grounding->translator()->getIntroducedLiteralsFor(p)){ // TODO should be introduced ATOMS
			assumptions.push_back(-abs(atom.second));
		}
	}
	for(auto pf : _assumeFalse.assumeFalse){
		assumptions.push_back(grounding->translator()->translateNonReduced(pf.symbol, pf.args));
	}

	if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
		logActionAndValue("maxsize", toDouble(Grounder::getFullGroundingSize()));
	}

	// Run solver
	auto mx = SolverConnection::initsolution(data, getOption(NBMODELS), assumptions);
	if (getOption(IntType::VERBOSE_SOLVING) > 0) {
		logActionAndTime("Starting solving at ");
	}
	bool unsat = false;
	auto terminator = new SolverTermination(mx);
	getGlobal()->addTerminationMonitor(terminator);

	auto t = basicTimer([](){return getOption(MXTIMEOUT);},[terminator](){terminator->notifyTerminateRequested();});
	thread time(&basicTimer::time, &t);

	MXResult result;
	try {
		mx->execute(); // FIXME wrap other solver calls also in try-catch
		unsat = mx->getSolutions().size()==0;
		if(getGlobal()->terminateRequested()){
			result._interrupted = true;
			getGlobal()->reset();
		}
	} catch (MinisatID::idpexception& error) {
		std::stringstream ss;
		ss << "Solver was aborted with message \"" << error.what() << "\"";
		t.requestStop();
		time.join();
		cleanup;
		throw IdpException(ss.str());
	} catch(UnsatException& ex){
		unsat = true;
	}catch(...){
		t.requestStop();
		time.join();
		throw;
	}

	t.requestStop();
	time.join();

	if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
		auto stats = mx->getStats();
		logActionAndValue("decisions", stats.decisions);
		logActionAndValue("first_decision", stats.time_of_first_decision);
		logActionAndValue("effective-size", Grounder::groundedAtoms());
		// TODO solving time
	}

	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	result._optimumfound = true;
	result.unsat = unsat;
	if(t.hasTimedOut()){
		Warning::warning("Model expansion interrupted: will continue with the (single best) model(s) found to date (if any).");
		result._optimumfound = false;
		result._interrupted = true;
		getGlobal()->reset();
	}

	if(not t.hasTimedOut() && unsat){
		MXResult result;
		result.unsat = true;
		for(auto lit: mx->getUnsatExplanation()){
			auto symbol = grounding->translator()->getSymbol(lit.getAtom());
			auto args = grounding->translator()->getArgs(lit.getAtom());
			result.unsat_in_function_of_ct_lits.push_back({symbol, args});
		}
		cleanup;
		return result;
	}

	// Collect solutions
	std::vector<Structure*> solutions;
	if (_minimizeterm != NULL) { // Optimizing
		if (not unsat) {
			Assert(mx->getBestSolutionsFound().size()>0);
			auto list = mx->getBestSolutionsFound();
			auto bestvalue = mx->getBestValueFound();
			result._optimalvalue = bestvalue;
			if (getOption(IntType::VERBOSE_SOLVING) > 0) {
				stringstream ss;
				ss <<"The best value found was " <<bestvalue <<"\n";
				ss <<"Solver generated " << list.size() << " model(s): ";
				logActionAndTime(ss.str());
			}
			for (auto i = list.cbegin(); i < list.cend(); ++i) {
				solutions.push_back(handleSolution(newstructure, **i, grounding, extender, targetvoc));
			}
		}
	} else if(not unsat){
		auto abstractsolutions = mx->getSolutions();
		if (getOption(IntType::VERBOSE_SOLVING) > 0) {
			stringstream ss;
			ss <<"Solver generated " << abstractsolutions.size() << " model(s): ";
			logActionAndTime(ss.str());
		}
		for (auto model = abstractsolutions.cbegin(); model != abstractsolutions.cend(); ++model) {
			solutions.push_back(handleSolution(newstructure, **model, grounding, extender, targetvoc));
		}
	}

	// Clean up: remove all objects that are only used here.
	cleanup;
	result._models = solutions;

	if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
		logActionAndTime("total-mx-time");
	}

	return result;
}

Structure* handleSolution(Structure* structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, StructureExtender* extender,
		Vocabulary* outputvoc) {
	auto newsolution = structure->clone();
	SolverConnection::addLiterals(model, grounding->translator(), newsolution);
	SolverConnection::addTerms(model, grounding->translator(), newsolution);
	if (extender != NULL && useLazyGrounding()) {
		/*if(getOption(VERBOSE_GROUNDING)>0){
			clog <<"Structure before extension = \n" <<toString(newsolution) <<"\n";
		}*/
		extender->extendStructure(newsolution);
		newsolution->clean();
		if(getOption(VERBOSE_GROUNDING)>1){
			clog <<"Extended structure = \n" <<toString(newsolution) <<"\n";
		}
	}
	newsolution->changeVocabulary(outputvoc);
	newsolution->clean();
	Assert(newsolution->isConsistent());
	return newsolution;
}
