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

#include "common.hpp"
#include <set>

class PFSymbol;
class Function;
class PredForm;
class FuncTerm;
class AggTerm;
class Term;
class Vocabulary;
class Structure;

namespace CPSupport {

bool eligibleForCP(const Function*, const Vocabulary*);
bool eligibleForCP(const PredForm*, const Vocabulary*);
bool eligibleForCP(const FuncTerm*, const Vocabulary*);
bool eligibleForCP(const AggFunction&);

bool allSymbolsEligible(const Term* t, const Structure* str);
bool eligibleForCP(const Term*, const Structure*);

}
