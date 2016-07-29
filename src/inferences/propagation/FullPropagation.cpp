#include "FullPropagation.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelIteration/ModelIterator.hpp"
#include "inferences/SolverConnection.hpp"
#include "utils/LogAction.hpp"

#include <algorithm>

namespace {
void addToStructure(Lit literal, Structure* struc, GroundTranslator* translator) {
  int atomnr = (literal > 0) ? literal : (-1) * (literal);
  if (translator->isInputAtom(atomnr)) {
    auto symbol = translator->getSymbol(atomnr);
    const auto& args = translator->getArgs(atomnr);
    if (literal < 0) {
      struc->inter(symbol)->makeFalseAtLeast(args);
    } else {
      struc->inter(symbol)->makeTrueAtLeast(args);
    }
  }
}

std::set<Lit> getLiteralSet(Structure* struc, GroundTranslator* translator) {
  std::set<Lit> set;
  auto symbols = translator->vocabulary()->getNonBuiltinNonOverloadedSymbols();
  for (auto s : symbols) {
    auto ct = struc->inter(s)->ct();
    for (auto el = ct->begin(); !el.isAtEnd(); ++el) {
      set.insert(translator->translateNonReduced(s, *el));
    }
    auto cf = struc->inter(s)->cf();
    for (auto el = cf->begin(); !el.isAtEnd(); ++el) {
      set.insert((-1) * translator->translateNonReduced(s, *el));
    }
  }
  return set;
}

void intersectLiterals(std::set<Lit>& l, Structure* s, GroundTranslator* translator) {
  std::set<Lit> structLits = getLiteralSet(s, translator);
  for(auto ll: structLits){
    l.erase(-ll);
  }
}

}

std::vector<Structure*> FullPropagation::propagateNoAssumps(AbstractTheory* theory, Structure* structure) {
  // TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented
  auto verbosity = std::max(getOption(IntType::VERBOSE_SOLVING),getOption(IntType::VERBOSE_SOLVING_STATISTICS));
  auto storedcp = getOption(CPSUPPORT);
  if (storedcp) {
    Warning::warning("Full propagator does not work with non-propositional grounding yet, so disabling cpsupport.");
  }
  setOption(CPSUPPORT, false);
  setOption(POSTPROCESS_DEFS, false);

  auto realtheo = static_cast<Theory*> (theory);
  MXAssumptions mxas; // TODO: superfluous...
  auto miter = ModelIterator(structure, realtheo, nullptr, nullptr, mxas);
  miter.init();
  //Tuple2AtomMap literals = miter.getLiterals();
  GroundTranslator* translator = miter.translator();
  MXResult model = miter.calculate();
  MAssert(!model.unsat && !model._models.empty());

  std::set<Lit> firstmodel = getLiteralSet(model._models[0], translator);
  std::vector<Lit> intersectionnegation;
  for(auto ll: firstmodel){
    intersectionnegation.push_back(-ll);
  }

  while (!model.unsat) {
    std::set<Lit> structLits =getLiteralSet(model._models[0], translator);
    for(unsigned int i=0; i<intersectionnegation.size(); ++i){
      if(!structLits.count(-intersectionnegation[i])){
        intersectionnegation[i]=intersectionnegation.back();
        intersectionnegation.pop_back();
        --i;
      }
    }
    if (verbosity > 1) {
      logActionAndValue("Models invalidating clause: ", intersectionnegation);
    }
    
    miter.addClause(intersectionnegation);
    model = miter.calculate();
  }

  setOption(CPSUPPORT, storedcp);

  Structure* safe = structure->clone();
  //Translate the result
  for (auto l: intersectionnegation) {
    addToStructure(-l, safe, translator);
  }
  safe->clean();
  MAssert(safe->isConsistent());
  
  return {safe};
}

std::vector<Structure*> FullPropagation::propagate(AbstractTheory* theory, Structure* structure) {
    // TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented
    auto verbosity = std::max(getOption(IntType::VERBOSE_SOLVING),getOption(IntType::VERBOSE_SOLVING_STATISTICS));
    auto storedcp = getOption(CPSUPPORT);
    if (storedcp) {
      Warning::warning("Full propagator does not work with non-propositional grounding yet, so disabling cpsupport.");
    }
    setOption(CPSUPPORT, false);
    setOption(POSTPROCESS_DEFS, false);

    Structure* safe = structure->clone();

    MXAssumptions mxas;
    auto realtheo = static_cast<Theory*> (theory);
    auto miter = ModelIterator(structure, realtheo, nullptr, nullptr, mxas);
    miter.init();
    //Tuple2AtomMap literals = miter.getLiterals();
    GroundTranslator* translator = miter.translator();
    MXResult model = miter.calculate();
    if (model.unsat || model._models.empty()) {
      return std::vector<Structure*>{};
    }

    std::set<Lit> underapproximation = getLiteralSet(safe, translator);
    if (verbosity > 0) {
      logActionAndValue("UnderApproximation", underapproximation);
    }

    Structure* currModel = model._models[0];
    std::set<Lit> overapproximation = getLiteralSet(currModel, translator);
    if (verbosity > 0) {
      logActionAndValue("OverApproximation", overapproximation);
    }

    for (auto it = underapproximation.cbegin(); it != underapproximation.cend(); ++it) {
      overapproximation.erase(*it);
    }
    if (verbosity > 0) {
      logActionAndValue("Over \\ Under", overapproximation);
    }

    while (!overapproximation.empty()) {
      Lit a = *overapproximation.begin();
      if (verbosity > 0) {
        logActionAndValue("Assumption", -a);
      }
      miter.addAssumption(-a);
      MXResult m = miter.calculate();
      if (m.unsat) {
        underapproximation.insert(a);
        overapproximation.erase(a);
        miter.removeAssumption(-a);
      } else {
        currModel = m._models[0];
        intersectLiterals(overapproximation, currModel, translator);
        miter.removeAssumption(-a);
      }

      if (verbosity > 3) {
        logActionAndValue("UnderApproximation", overapproximation);
        logActionAndValue("OverApproximation", overapproximation);
      }
    }

    setOption(CPSUPPORT, storedcp);

    //Translate the result
    for (auto literal = underapproximation.cbegin(); literal != underapproximation.cend(); ++literal) {
      addToStructure(*literal, safe, translator);
    }
    safe->clean();

    if (not safe->isConsistent()) {
      return std::vector<Structure*>{};
    }
    return {safe};
  }