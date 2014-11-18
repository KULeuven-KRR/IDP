/* 
 * File:   ModelIterator.cpp
 * Author: rupsbant
 * 
 * Created on October 3, 2014, 9:56 AM
 */

#include "ModelIterator.hpp"
#include "structure/StructureComponents.hpp"
#include <cstdlib>
#include <bits/stl_algo.h>

ModelIterator::ModelIterator() {
}

ModelIterator::ModelIterator(const ModelIterator& orig) {
    orig;
}

ModelIterator::~ModelIterator() {
}

static ModelIterator* create(Vocabulary* targetVocabulary, Structure* structure) {
    return NULL;
}

int getMXVerbosity() {
    auto mxverbosity = max(getOption(IntType::VERBOSE_SOLVING), getOption(IntType::VERBOSE_SOLVING_STATISTICS));
    return mxverbosity;
}

void ModelIterator::init() {
    _data = SolverConnection::createsolver(1);
    _currentVoc = new Vocabulary(createName());
}

std::pair<Theory*, std::vector<Definition*>> ModelIterator::preprocess(Theory* theory) {
    std::vector<Definition*> postprocessdefs;
    Theory* theoryClone = theory->clone();
    if (getOption(POSTPROCESS_DEFS)) {
        postprocessdefs = simplifyTheoryForPostProcessableDefinitions(theoryClone, _minimizeterm, _structure, _currentVoc, _outputvoc);
    }
    if (getOption(SATISFIABILITYDELAY)) { // Add non-forgotten defs again, as top-down grounding might give a better result
        for (auto def : postprocessdefs) {
            theoryClone->add(def);
        }
        postprocessdefs.clear();
    }
    return
    {
        theoryClone, postprocessdefs
    };
}

void ModelIterator::ground(Theory* theory) {
    std::pair<AbstractGroundTheory*, StructureExtender*> groundingAndExtender = {NULL, NULL};
    try {
        groundingAndExtender = GroundingInference<PCSolver>::createGroundingAndExtender(
                theory, _structure, _outputvoc, _minimizeterm, _tracemonitor, getOption(IntType::NBMODELS) != 1, _data);
    } catch (...) {
        if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("effective-size", groundingAndExtender.first->getSize()); //Grounder::groundedAtoms());
        }
        throw;
    }
    _grounding = groundingAndExtender.first;
    _extender = groundingAndExtender.second;

    litlist assumptions;
    auto trans = _grounding->translator();
    for (auto p : _assumeFalse.assumeAllFalse) {
        for (Tuple2AtomMap& atom : trans->getIntroducedLiteralsFor(p)) { // TODO should be introduced ATOMS
            assumptions.push_back(-abs(atom.second));
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
            assumptions.push_back(-abs(atom));
        }
    }
    for (auto pf : _assumeFalse.assumeFalse) {
        assumptions.push_back(_grounding->translator()->translateNonReduced(pf.symbol, pf.args));
    }
}


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

MXResult ModelIterator::calculate() {
    auto mx = SolverConnection::initsolution(data, 1, assumptions);
    auto startTime = clock();
    if (getMXVerbosity() > 0) {
        logActionAndTime("Starting solving at ");
    }
    bool unsat = false;
    auto terminator = new SolverTermination(mx);
    getGlobal()->addTerminationMonitor(terminator);

    auto t = basicResourceMonitor([]() {
        return getOption(MXTIMEOUT);
    }, []() {
        return getOption(MXMEMORYOUT);
    }, [terminator]() {
        terminator->notifyTerminateRequested();
    });
    tthread::thread time(&resourceMonitorLoop, &t);

    MXResult result;
    try {
        mx->execute(); // FIXME wrap other solver calls also in try-catch
        unsat = mx->getSolutions().size() == 0;
        if (getGlobal()->terminateRequested()) {
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
    } catch (UnsatException& ex) {
        unsat = true;
    } catch (...) {
        t.requestStop();
        time.join();
        throw;
    }

    t.requestStop();
    time.join();

    if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
        logActionAndValue("effective-size", _grounding->getSize());
        if (mx->getNbModelsFound() > 0) {
            logActionAndValue("state", "satisfiable");
        }
        std::clog.flush();
    }

    if (getGlobal()->terminateRequested()) {
        throw IdpException("Solver was terminated");
    }

    result._optimumfound = not result._interrupted;
    result.unsat = unsat;
    if (t.outOfResources()) {
        Warning::warning("Model expansion interrupted: will continue with the (single best) model(s) found to date (if any).");
        result._optimumfound = false;
        result._interrupted = true;
        getGlobal()->reset();
    }

    if (not t.outOfResources() && unsat) {
        MXResult result;
        result.unsat = true;
        for (auto lit : mx->getUnsatExplanation()) {
            auto symbol = _grounding->translator()->getSymbol(lit.getAtom());
            auto args = _grounding->translator()->getArgs(lit.getAtom());
            result.unsat_in_function_of_ct_lits.push_back({symbol, args});
        }
        cleanup;
        if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("state", "unsat");
        }
        return result;
    }
    
    return getStructure(result);
}

MXResult* ModelIterator::getStructure(PCModelExpand* mx, MXResult mx) {
    MXResult result;
    auto mxverbosity = getMXVerbosity();
    std::vector<Structure*> solutions;
    if (result.unsat) {
        if (_minimizeterm != NULL) { // Optimizing
            Assert(mx->getBestSolutionsFound().size() > 0);
            auto list = mx->getBestSolutionsFound();
            for (auto i = list.cbegin(); i < list.cend(); ++i) {
                solutions.push_back(handleSolution(_structure, **i, _grounding, _extender, _outputvoc, postprocessdefs));
            }
            auto bestvalue = evaluate(_minimizeterm->clone(), solutions.front());
            Assert(bestvalue != NULL && bestvalue->type() == DomainElementType::DET_INT);
            result._optimalvalue = bestvalue->value()._int;
            if (mxverbosity > 0) {
                logActionAndValue("bestvalue", result._optimalvalue);
                logActionAndValue("nrmodels", list.size());
                logActionAndTimeSince("total-solving-time", startTime);
            }
            if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
                logActionAndValue("bestvalue", result._optimalvalue);
                if (result._optimumfound) {
                    logActionAndValue("state", "optimal");
                } else {
                    logActionAndValue("state", "satisfiable");
                }
                std::clog.flush();
            }
        } else
            if (getOption(VERBOSE_GROUNDING_STATISTICS) > 0) {
            logActionAndValue("state", "satisfiable");
        }
        auto abstractsolutions = mx->getSolutions();
        if (mxverbosity > 0) {
            logActionAndValue("nrmodels", abstractsolutions.size());
            logActionAndTimeSince("total-solving-time", startTime);
        }
        for (auto model = abstractsolutions.cbegin(); model != abstractsolutions.cend(); ++model) {
            solutions.push_back(handleSolution(_structure, **model, _grounding, _extender, _outputvoc, postprocessdefs));
        }
    }
    return result;
}