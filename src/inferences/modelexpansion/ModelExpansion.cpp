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

#include "theory/Query.hpp"
#include "inferences/querying/Query.hpp"
#include "inferences/grounding/Grounding.hpp"
#include "inferences/SolverInclude.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "errorhandling/error.hpp"
#include "creation/cppinterface.hpp"
#include "utils/ResourceMonitor.hpp"
#include "DefinitionPostProcessing.hpp"

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
		if(VocabularyUtils::isSubVocabulary(structure->vocabulary(), theory->vocabulary())){
			structure->changeVocabulary(theory->vocabulary());
		}else {
			throw IdpException("Modelexpansion requires that the structure interprets (a subvocabulary of) the vocabulary of the theory.");
		}
	}
	if(term!=NULL && not VocabularyUtils::isSubVocabulary(term->vocabulary(), theory->vocabulary())){
		throw IdpException("Modelexpansion requires that the minimization term ranges over (a subvocabulary of) the vocabulary of the theory.");
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

Structure* handleSolution(Structure const * const structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, StructureExtender* extender,
		Vocabulary* outputvoc, const std::vector<Definition*>& defs);

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
	auto mxverbosity = max(getOption(IntType::VERBOSE_SOLVING),getOption(IntType::VERBOSE_SOLVING_STATISTICS));
	auto data = SolverConnection::createsolver(getOption(IntType::NBMODELS));
	auto targetvoc = _outputvoc == NULL ? _theory->vocabulary() : _outputvoc;
	auto clonetheory = _theory->clone();
	auto newstructure = _structure->clone();
	auto voc = new Vocabulary(createName());
	voc->add(clonetheory->vocabulary());
	voc->add(newstructure->vocabulary());
	newstructure->changeVocabulary(voc);
	clonetheory->vocabulary(voc);

	//TODO See Issue #523 DefinitionUtils::splitDefinitions(clonetheory); // TODO implement with same meta-level approach as below!

	std::vector<Definition*> postprocessdefs;
	if(getOption(POSTPROCESS_DEFS)){
		postprocessdefs = simplifyTheoryForPostProcessableDefinitions(clonetheory, _minimizeterm, _structure, voc, targetvoc);
	}
	if(getOption(SATISFIABILITYDELAY)){ // Add non-forgotten defs again, as top-down grounding might give a better result
		for(auto def: postprocessdefs){
			clonetheory->add(def);
		}
		postprocessdefs.clear();
	}

	std::pair<AbstractGroundTheory*, StructureExtender*> groundingAndExtender = {NULL, NULL};
	try{
		groundingAndExtender = GroundingInference<PCSolver>::createGroundingAndExtender(clonetheory, newstructure, _outputvoc, _minimizeterm, _tracemonitor, getOption(IntType::NBMODELS) != 1, data);
	}catch(...){
		if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
			logActionAndValue("effective-size", groundingAndExtender.first->getSize()); //Grounder::groundedAtoms());
		}
		throw;
	}
	auto grounding = groundingAndExtender.first;
	auto extender = groundingAndExtender.second;

	litlist assumptions;
	auto trans = grounding->translator();
	for(auto p: _assumeFalse.assumeAllFalse){
		for(auto atom: trans->getIntroducedLiteralsFor(p)){ // TODO should be introduced ATOMS
			assumptions.push_back(-abs(atom.second));
		}
		std::vector<Variable*> vars;
		std::vector<Term*> varterms;
		for(uint i=0; i<p->arity(); ++i){
			vars.push_back(new Variable(p->sorts()[i]));
			varterms.push_back(new VarTerm(vars.back(), {}));
		}
		auto table = Querying::doSolveQuery(new Query("", vars, new PredForm(SIGN::POS, p, varterms, {}), {}), trans->getConcreteStructure(), trans->getSymbolicStructure());
		for(auto i=table->begin(); not i.isAtEnd(); ++i){
			auto atom = grounding->translator()->translateNonReduced(p, *i);
			assumptions.push_back(-abs(atom));
		}
	}
	for(auto pf : _assumeFalse.assumeFalse){
		assumptions.push_back(grounding->translator()->translateNonReduced(pf.symbol, pf.args));
	}

	// Run solver
	auto mx = SolverConnection::initsolution(data, getOption(NBMODELS), assumptions);
	auto startTime = clock();
	if (mxverbosity > 0) {
		logActionAndTime("Starting solving at ");
	}
	bool unsat = false;
	auto terminator = new SolverTermination(mx);
	getGlobal()->addTerminationMonitor(terminator);

	auto t = basicResourceMonitor([](){return getOption(MXTIMEOUT);}, [](){return getOption(MXMEMORYOUT);},[terminator](){terminator->notifyTerminateRequested();});
	tthread::thread time(&resourceMonitorLoop, &t);

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
		logActionAndValue("effective-size", groundingAndExtender.first->getSize());
		if(mx->getNbModelsFound()>0){
			logActionAndValue("state", "satisfiable");
		}
	//	if (_minimizeterm != NULL && mx->getBestSolutionsFound().size()>0){
	//		logActionAndValue("bestvalue", mx->getBestValueFound());
	//	}
		std::clog.flush();
		// TODO solving time
	}

	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	result._optimumfound = not result._interrupted;
	result.unsat = unsat;
	if(t.outOfResources()){
		Warning::warning("Model expansion interrupted: will continue with the (single best) model(s) found to date (if any).");
		result._optimumfound = false;
		result._interrupted = true;
		getGlobal()->reset();
	}

	if(not t.outOfResources() && unsat){
		MXResult result;
		result.unsat = true;
		for(auto lit: mx->getUnsatExplanation()){
			auto symbol = grounding->translator()->getSymbol(lit.getAtom());
			auto args = grounding->translator()->getArgs(lit.getAtom());
			result.unsat_in_function_of_ct_lits.push_back({symbol, args});
		}
		cleanup;
		if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
			logActionAndValue("state", "unsat");
		}
		return result;
	}

	// Collect solutions
	std::vector<Structure*> solutions;
	if (_minimizeterm != NULL) { // Optimizing
		if (not unsat) {
			Assert(mx->getBestSolutionsFound().size()>0);
			auto list = mx->getBestSolutionsFound();
			for (auto i = list.cbegin(); i < list.cend(); ++i) {
				solutions.push_back(handleSolution(newstructure, **i, grounding, extender, targetvoc, postprocessdefs));
			}
			auto bestvalue = evaluate(_minimizeterm, solutions.front());
			Assert(bestvalue!=NULL && bestvalue->type()==DomainElementType::DET_INT);
			result._optimalvalue = bestvalue->value()._int;
			if (mxverbosity > 0) {
				logActionAndValue("bestvalue", bestvalue);
				logActionAndValue("nrmodels", list.size());
				logActionAndTimeSince("total-solving-time", startTime);
			}
			if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
				logActionAndValue("bestvalue", result._optimalvalue);
				if(result._optimumfound){
					logActionAndValue("state", "optimal");
				}else{
					logActionAndValue("state", "satisfiable");
				}
				std::clog.flush();
			}
		}
	} else if(not unsat){
		if(getOption(VERBOSE_GROUNDING_STATISTICS) > 0){
			logActionAndValue("state", "satisfiable");
		}
		auto abstractsolutions = mx->getSolutions();
		if (mxverbosity > 0) {
			logActionAndValue("nrmodels", abstractsolutions.size());
			logActionAndTimeSince("total-solving-time", startTime);
		}
		for (auto model = abstractsolutions.cbegin(); model != abstractsolutions.cend(); ++model) {
			solutions.push_back(handleSolution(newstructure, **model, grounding, extender, targetvoc, postprocessdefs));
		}
	}

	// Clean up: remove all objects that are only used here.
	cleanup;
	result._models = solutions;
	return result;
}

Structure* handleSolution(Structure const * const structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, StructureExtender* extender,
		Vocabulary* outputvoc, const std::vector<Definition*>& postprocessdefs) {
	auto newsolution = structure->clone();
	SolverConnection::addLiterals(model, grounding->translator(), newsolution);
	SolverConnection::addTerms(model, grounding->translator(), newsolution);
	auto defs = postprocessdefs;
	if (extender != NULL && useLazyGrounding()) {
		auto moredefs = extender->extendStructure(newsolution);
		insertAtEnd(defs, moredefs);
		newsolution->clean();
	}
	computeRemainingDefinitions(defs, newsolution, outputvoc);
	newsolution->changeVocabulary(outputvoc);
	newsolution->clean();
	Assert(newsolution->isConsistent());
	return newsolution;
}
