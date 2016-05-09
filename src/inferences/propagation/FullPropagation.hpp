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

#pragma once

#include "IncludeComponents.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/modelIteration/ModelIterator.hpp"
#include "inferences/SolverConnection.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class FullPropagation {
public:
  std::vector<Structure*> propagate(AbstractTheory* theory, Structure* structure) {
    // TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented
    auto verbosity = max(getOption(IntType::VERBOSE_SOLVING),getOption(IntType::VERBOSE_SOLVING_STATISTICS));
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
    if (model.unsat) {
      return std::vector<Structure*>{};
    }
    if (model._models.empty()) {
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
        miter.addAssumption(a);
                // add literal a to structure safe
      } else {
        miter.removeAssumption(-a);
        currModel = m._models[0];
        intersectLiterals(overapproximation, currModel, translator);
      }

      if (verbosity > 0) {
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
    std::set<Lit> l1 = getLiteralSet(s, translator);
    for (auto it = l1.cbegin(); it != l1.cend(); ++it) {
      l.erase((-1)*(*it));
    }
  }
};
