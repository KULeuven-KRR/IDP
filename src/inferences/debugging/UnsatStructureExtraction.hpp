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

class TheoryComponent;
class AbstractTheory;
class Structure;
class DomainAtom;

struct UnsatStructureResult {
    bool succes;
    Structure* core;
};


class UnsatStructureExtraction {
public:
    static UnsatStructureResult extractStructure(AbstractTheory* atheory, Structure* structure, Vocabulary* v);
};


