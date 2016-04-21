#pragma once
class DomainAtom;
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"


class AlreadySatisfiableException: public IdpException {
public:
    AlreadySatisfiableException() : IdpException("Cannot get core of theory which is already satisfiable") { }
};

namespace MinimizeMarkers {

MXAssumptions minimizeAssumps(AbstractTheory *newtheory, Structure *s, MXAssumptions markers);

bool minimizeSubArray(AbstractTheory *newtheory, Structure *s, std::vector <DomainAtom> &curArr,
                      MXAssumptions &core, uint goal);

};