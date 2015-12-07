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

#include <vector>
#include <vocabulary/vocabulary.hpp>
#include <structure/MainStructureComponents.hpp>
#include "AddMarkers.hpp"

class TheoryComponent;
class AbstractTheory;
class Structure;
class DomainAtom;


class UnsatExtraction {
public:
    static pair<Structure*,Theory*> extractCore(bool assumeStruc, bool assumeTheo, AbstractTheory* intheory, Structure* structure, Vocabulary* vAssume);
private:
    static void tableToVector(std::vector<DomainAtom> &assumeNeg, PFSymbol *const symbol, const PredTable *cfTab);

    static void assumifyStructure(const Structure *structure, const Vocabulary *vAssume, const Theory *newtheory,
                                  Structure *&emptyStruc, std::vector<DomainAtom> &assumeNeg,
                                  std::vector<DomainAtom> &assumePos);

    static void assumifyTheory(Theory * &newtheory, vector<Predicate*> &assumeAllFalse, AddMarkers * &am);

    static void outputStructure(const AbstractTheory *intheory, const Structure *emptyStruc,
                                MXAssumptions &coreresult);

    static Theory* outputTheory(  const AddMarkers *am,
                                  MXAssumptions &theoryMarkers,
                                  Vocabulary* voc) ;

};


