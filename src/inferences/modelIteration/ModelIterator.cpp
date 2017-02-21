#include "ModelIterator.hpp"
#include "inferences/modelexpansion/DefinitionPostProcessing.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/StructureComponents.hpp"
#include <cstdlib>
#include <bits/stl_algo.h>
#include "Weight.hpp"
#include "utils/UniqueNames.hpp"
#include "theory/Query.hpp"
#include "inferences/querying/Query.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "inferences/grounding/Grounding.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/SolverConnection.hpp"
#include "utils/ResourceMonitor.hpp"
#include "vocabulary/vocabulary.hpp"

#include "../../../lib/minisatid/src/external/Lit.hpp"
#include <algorithm>

using namespace std;

ModelIterator::ModelIterator(Structure* structure, Theory* theory, Vocabulary* targetvoc, TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse) {
	_structure = structure -> clone();
	_theory = theory->clone();
	_outputvoc = targetvoc != nullptr ? targetvoc : theory->vocabulary();
	_tracemonitor = tracemonitor;
	_assumeFalse = assumeFalse;
}

ModelIterator::~ModelIterator() {
	_grounding->recursiveDelete();
	_theory->recursiveDelete();
	delete (_extender);
	delete (_assumptions);
	delete (_structure);
	delete (_currentVoc);
	delete (_data);
	delete (_mx);
}

shared_ptr<ModelIterator> createIterator(AbstractTheory* theory, Structure* structure, Vocabulary* targetVocabulary,
		TraceMonitor* tracemonitor, const MXAssumptions& assumeFalse) {
	if (theory == nullptr || structure == nullptr) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	auto t = dynamic_cast<Theory*>(theory); // TODO handle other cases
	if (t == nullptr) {
		throw notyetimplemented("Modeliteration of already ground theories");
	}
	if(structure->vocabulary()!=theory->vocabulary()){
		if(VocabularyUtils::isSubVocabulary(structure->vocabulary(), theory->vocabulary())){
			structure->changeVocabulary(theory->vocabulary());
		}else {
			throw IdpException("Modeliteration requires that the structure interprets (a subvocabulary of) the vocabulary of the theory.");
		}
	}
	auto m = make_shared<ModelIterator>(structure, t, targetVocabulary, tracemonitor, assumeFalse);
	if (getGlobal()->getOptions()->symmetryBreaking() != SymmetryBreaking::NONE && getOption(NBMODELS) != 1) {
		Warning::warning("Cannot generate models symmetrical to models already found! More models might exist.");
	}
	return m;
}

static int getMXVerbosity() {
    auto mxverbosity = max(getOption(IntType::VERBOSE_SOLVING), getOption(IntType::VERBOSE_SOLVING_STATISTICS));
    return mxverbosity;
}

void ModelIterator::init() {
    _data = SolverConnection::createsolver(1);
    _currentVoc = new Vocabulary(createName());
	_currentVoc->add(_structure->vocabulary());
	_currentVoc->add(_theory->vocabulary());
	_structure->changeVocabulary(_currentVoc);
	_theory->vocabulary(_currentVoc);
	_assumptions = new litlist();
	ground(_theory);
	prepareSolver();
}

/**
 * Modifies theory, assumes cloned theory.
 */
std::vector<Definition*> ModelIterator::preprocess(Theory* theory) {
    std::vector<Definition*> postprocessdefs;
    if (getOption(POSTPROCESS_DEFS)) {
        postprocessdefs = simplifyTheoryForPostProcessableDefinitions(theory, nullptr, _structure, _currentVoc, _outputvoc);
    }
    if (getOption(SATISFIABILITYDELAY)) { // Add non-forgotten defs again, as top-down grounding might give a better result
        for (auto def : postprocessdefs) {
            theory->add(def);
        }
        postprocessdefs.clear();
    }
    return postprocessdefs;
}

void ModelIterator::ground(Theory* theory) {
    std::pair<AbstractGroundTheory*, StructureExtender*> groundingAndExtender = {nullptr, nullptr};
    try {
        groundingAndExtender = GroundingInference<PCSolver>::createGroundingAndExtender(
                theory, _structure, _outputvoc, nullptr, _tracemonitor, true, _data);
    } catch (...) {
        if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("effective-size", groundingAndExtender.first->getSize()); //Grounder::groundedAtoms());
        }
        throw;
    }
    _grounding = groundingAndExtender.first;
    _extender = groundingAndExtender.second;

    auto trans = _grounding->translator();
    for (auto p : _assumeFalse.assumeAllFalse) {
        for (auto atom : trans->getIntroducedLiteralsFor(p)) { // TODO should be introduced ATOMS
            _assumptions->push_back(-abs(atom.second));
        }
        std::vector<Variable*> vars;
        std::vector<Term*> varterms;
        for (uint i = 0; i < p->arity(); ++i) {
            vars.push_back(new Variable(p->sorts()[i]));

            varterms.push_back(new VarTerm(vars.back(), {}));
        }

        PredTable* table = Querying::doSolveQuery(new Query("", vars, new PredForm(SIGN::POS, p, varterms,{}), {}), trans->getConcreteStructure(), trans->getSymbolicStructure());
        for (auto i = table->begin(); not i.isAtEnd(); ++i) {
            auto atom = _grounding->translator()->translateNonReduced(p, *i);
            _assumptions->push_back(-abs(atom));
        }
    }
    for (auto pf : _assumeFalse.assumeFalse) {
        _assumptions->push_back(_grounding->translator()->translateNonReduced(pf.symbol, pf.args));
    }
}

class SolverTermination: public TerminateMonitor {
private:
	MinisatID::ModelIterationTask* solver;
public:
	SolverTermination(MinisatID::ModelIterationTask* solver)
			: solver(solver) {
	}
	void notifyTerminateRequested() {
		solver->notifyTerminateRequested();
	}
};

void ModelIterator::prepareSolver() {
    _data->finishParsing();
    _mx = SolverConnection::createIteratorSolution(_data, 1, *_assumptions);
	_mx->initialise();
}

MXResult ModelIterator::calculateMonitor() {
	auto terminator = std::make_shared<SolverTermination>(_mx);
    getGlobal()->addTerminationMonitor(terminator.get());
    auto t = basicResourceMonitor(
				[]() { return getOption(MXTIMEOUT); }, 
				[]() { return getOption(MXMEMORYOUT);}, 
				[terminator]() {terminator->notifyTerminateRequested(); });
	tthread::thread time(&resourceMonitorLoop, &t);
	try{
		auto result = calculate();
		t.requestStop();
		time.join();
		getGlobal()->removeTerminationMonitor(terminator.get());
		if (t.outOfResources()) {
			result._optimumfound = false;
			result._interrupted = true;
			getGlobal()->reset();
		}
		return result;
	} catch (...) {
		t.requestStop();
		time.join();
		getGlobal()->removeTerminationMonitor(terminator.get());
		throw;
	}
}

MXResult ModelIterator::calculate() {
    auto startTime = clock();
    if (getMXVerbosity() > 0) {
        logActionAndTime("Starting solving at ");
    }
    MXResult result;
		std::shared_ptr<MinisatID::Model> model = nullptr;
    try {
				model = _mx->findNext();
        result.unsat = (model == nullptr);
        if (getGlobal()->terminateRequested()) {
            result._interrupted = true;
            getGlobal()->reset();
        }
    } catch (MinisatID::idpexception& error) {
        std::stringstream ss;
        ss << "Solver was aborted with message \"" << error.what() << "\"";
        throw IdpException(ss.str());
    } catch (UnsatException& ex) {
        result.unsat = true;
		}

    if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
        logActionAndValue("effective-size", _grounding->getSize());
        if (not result.unsat) {
            logActionAndValue("state", "satisfiable");
        }
        std::clog.flush();
    }

    if (getGlobal()->terminateRequested()) {
        throw IdpException("Solver was terminated");
    }
    result._optimumfound = not result._interrupted;
    if (result.unsat) {
        if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("state", "unsat");
        }
        return result;
    }
    result = getStructure(result, startTime, model);
    return result;
}

std::pair<Atom,bool> getAtomSign(const Lit l){
  Atom a = (l>= 0) ? l : (-1)*l;
  return {a,(l<0)};
}
void ModelIterator::addAssumption(const Lit l) {
    auto atomsign = getAtomSign(l);
    _mx->addAssumption(atomsign.first,atomsign.second);
}
void ModelIterator::removeAssumption(const Lit l){
    auto atomsign = getAtomSign(l);
    _mx->removeAssumption(atomsign.first,atomsign.second);
}
void ModelIterator::addClause(const std::vector<Lit>& lits){
    std::vector<std::pair<Atom,bool> > atomsigns;
    for(auto l: lits){
      atomsigns.push_back(getAtomSign(l));
    }
    _mx->addClause(atomsigns);
}

GroundTranslator* ModelIterator::translator() {
    return _grounding->translator();
}


Structure* handleSolution(Structure const * const structure, const MinisatID::Model& model, AbstractGroundTheory* grounding, StructureExtender* extender,
		Vocabulary* outputvoc, const std::vector<Definition*>& defs);

MXResult ModelIterator::getStructure(MXResult result, clock_t startTime, std::shared_ptr<MinisatID::Model> model) {
    auto mxverbosity = getMXVerbosity();
    std::vector<Structure*> solutions;
    if (not result.unsat) {
		MAssert(model != nullptr);
		if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("state", "satisfiable");
        }
        if (mxverbosity > 0) {
            logActionAndValue("nrmodels", 1);
            logActionAndTimeSince("total-solving-time", startTime);
        }
			auto solution = handleSolution(_structure, *model, _grounding, 
				_extender, _outputvoc, postprocessdefs);
			solutions.push_back(solution);
			result._models = solutions;
    }
    return result;
}
