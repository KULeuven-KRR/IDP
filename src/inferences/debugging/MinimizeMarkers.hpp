#pragma once
class DomainAtom;
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"


class AlreadySatisfiableException: public IdpException {
public:
    AlreadySatisfiableException() : IdpException("Cannot get core of theory which is already satisfiable") { }
};

std::vector<DomainAtom> minimizeAssumps(AbstractTheory *newtheory, Structure *s, MXAssumptions markers);
