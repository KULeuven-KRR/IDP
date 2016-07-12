/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "creation/cppinterface.hpp"
#include "vocabulary/vocabulary.hpp"

typedef TypedInference<LIST(Term*)> NegateTermInferenceBase;
class NegateTerm : public NegateTermInferenceBase {
public:
    NegateTerm()
            : NegateTermInferenceBase("negateterm", "Get the negation (minus) of a term") {
        setNameSpace(getInternalNamespaceName());
    }

    InternalArgument execute(const std::vector<InternalArgument>& args) const {
        auto t = get<0>(args);
        Term* term = dynamic_cast<Term*>(t);
        auto result = &Gen::functerm(getStdFunc(STDFUNC::UNARYMINUS,{term->sort()},term->vocabulary()) ,{term});
        result->name("minus"+term->name());
        result->vocabulary(term->vocabulary());
        return InternalArgument(result);
    }
};
