#pragma once

#include "utils/UniqueNames.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
extern void parsefile(const std::string&);

std::vector<Definition*> simplifyTheoryForPostProcessableDefinitions(Theory*, Term*, Structure*, Vocabulary*, Vocabulary*);
void computeRemainingDefinitions(const std::vector<Definition*>, Structure*, Vocabulary* );