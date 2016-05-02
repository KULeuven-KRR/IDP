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
	 std::vector<Structure*>  propagate(AbstractTheory* theory, Structure* structure) {
		 // TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)

		 auto storedcp = getOption(CPSUPPORT);
         if(storedcp){
             Warning::warning("Optimal propagator does not work with non-propositional grounding yet, so disabling cpsupport.");
         }
		 setOption(CPSUPPORT, false);

		 Structure* safe = structure->clone();

		 MXAssumptions mxas;
		 auto realtheo = static_cast<Theory*>(theory);
		 auto miter = ModelIterator(structure, realtheo, nullptr, nullptr, mxas);
		 miter.init();
         //Tuple2AtomMap literals = miter.getLiterals();
         GroundTranslator* translator = miter.translator();
		 MXResult model = miter.calculate();
		 if(model.unsat){
			 return std::vector<Structure*> { };
		 }
		 if (model._models.empty()) {
			 return std::vector<Structure*> { };
		 }

         std::set<Lit> underapproximation = getLiteralSet(safe, translator);

         Structure* currModel = *(model._models).cbegin();

		 // Contains all literals contained in every model seen so far, except for those in underapproximation.
		 std::set<Lit> overapproximation = getLiteralSet(currModel, translator);

		 // Take the intersection of all models
		 for (auto it = underapproximation.cbegin(); it != underapproximation.cend(); ++it) {
			 overapproximation.erase(*it);
		 }

		 while(! overapproximation.empty()) {
			 Lit a = *(overapproximation.cbegin());
			 miter.addAssumption(a, false);
			 MXResult m = miter.calculate();
			 if(m.unsat) {
                 underapproximation.insert(a);
                 overapproximation.erase(a);
                 // add literal a to structure safe
             }
			 else {
				 currModel = *(m._models.cbegin());
				 intersectLiterals(&overapproximation, currModel, translator);
			 }
			 miter.removeAssumption(a, false);
		 }

		setOption(CPSUPPORT, storedcp);

		//Translate the result
		for (auto literal = underapproximation.cbegin(); literal != underapproximation.cend(); ++literal) {
			int atomnr = (*literal > 0) ? *literal : (-1) * (*literal);
			if (translator->isInputAtom(atomnr)) {
				auto symbol = translator->getSymbol(atomnr);
				const auto& args = translator->getArgs(atomnr);
				if (*literal < 0) {
					safe->inter(symbol)->makeFalseAtLeast(args);
				} else {
					safe->inter(symbol)->makeTrueAtLeast(args);
				}
			}
		}
		safe->clean();

		if(not safe->isConsistent()){
			return std::vector<Structure*> { };
		}
		return {safe};
	}

    std::set<Lit> getLiteralSet(Structure* struc, GroundTranslator* translator) {
        std::set<Lit> set;
        auto symbols = translator->vocabulary()->getNonBuiltinNonOverloadedSymbols();
        for(auto s : symbols) {
            auto l = translator->getIntroducedLiteralsFor(s);
            cout << l ;
            for (auto p = l.cbegin(); p != l.cend(); ++p) {
                if (struc->inter(s)->isFalse(p->first))
                    set.insert((-1) * (p->second));
                if (struc->inter(s)->isTrue(p->first))
                    set.insert(p->second);
            }
        }
        return set;
    }

	void intersectLiterals(std::set<Lit>* l, Structure* s, GroundTranslator* translator) {
        std::set<Lit> l1 = getLiteralSet(s, translator);
		for (auto it = l1.cbegin(); it != l1.cend(); ++it) {
			l->erase((-1)*(*it));
		}
	}


};
