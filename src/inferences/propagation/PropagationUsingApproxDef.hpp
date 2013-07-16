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
#include "inferences/approximatingdefinition/ApproximatingDefinition.hpp"
#include "inferences/approximatingdefinition/GenerateApproximatingDefinition.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by generating the approximating defintion for TRUE downwards and
 * FALSE upwards, calculating this definition and adjust the structure accordingly.
 */
class PropagationUsingApproxDef {

private:
	ApproximatingDefinition::DerivationTypes* getDerivationTypes();
	void processApproxDef(AbstractStructure*, ApproximatingDefinition*);
	std::vector<AbstractStructure*> propagateUsingAllRules(AbstractTheory*, AbstractStructure*);
	std::vector<AbstractStructure*> propagateUsingCheapRules(AbstractTheory*, AbstractStructure*);
	std::vector<AbstractStructure*> propagateUsingStratification(AbstractTheory*, AbstractStructure*);

public:
	std::vector<AbstractStructure*>  propagate(AbstractTheory*, AbstractStructure*);
};
