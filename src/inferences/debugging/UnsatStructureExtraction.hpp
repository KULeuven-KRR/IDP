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

class TheoryComponent;
class AbstractTheory;
class Structure;
class DomainAtom;


class UnsatStructureExtraction {
public:
    static Structure* extractStructure(AbstractTheory* atheory, Structure* structure, Vocabulary* v);
private:
    static void tableToVector(std::vector<DomainAtom> &assumeNeg, PFSymbol *const symbol, const PredTable *cfTab);
};


