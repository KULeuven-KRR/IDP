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

#include "commandinterface.hpp"
#include "theory/TheoryUtils.hpp"

#include "inferences/propagation/GroundingPropagation.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/propagation/OptimalPropagation.hpp"
#include "inferences/propagation/FullPropagation.hpp"

InternalArgument postProcess(std::vector<Structure*> sols) {
	if (sols.size() == 0) {
		return InternalArgument();
	}
	Assert(sols.size()==1);
	auto structure = sols[0];
	structure->materialize();
	structure->clean();
	return InternalArgument(structure);
}

InternalArgument doSymbolicPropagation(AbstractTheory* t, Structure* s) {
	vector<Structure*> sols;
	if (getGlobal()->getOptions()->approxDef() != ApproxDef::NONE && getOption(BoolType::XSB)) {
		PropagationUsingApproxDef* propagator = new PropagationUsingApproxDef();
		sols = propagator->propagate(t, s);
	} else {
		SymbolicPropagation propagator;
		sols = propagator.propagate(t, s);
	}
	return postProcess(sols);
}

/**
 * Implements symbolic propagation, followed by an evaluation of the BDDs to obtain a concrete structure
 */
class PropagateInference: public TheoryStructureBase {
public:
	PropagateInference()
			: TheoryStructureBase("propagate",
					"Return a structure, made more precise than the input by doing symbolic propagation on the theory. Returns nil when propagation results in an inconsistent structure") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return doSymbolicPropagation(get<0>(args), get<1>(args));
	}
};

class PropagateDefinitionsInference : public TheoryStructureBase {
public:
	PropagateDefinitionsInference()
			: TheoryStructureBase("propagatedefinitions",
					"Return a structure, made more precise than the input by doing symbolic propagation on the completion of the definitions in the theory. Returns nil when propagation results in an inconsistent structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		vector<Structure*> sols;
		Theory* inputTheory = dynamic_cast<Theory*>(get<0>(args));
		auto newTheory = new Theory("", inputTheory->vocabulary(), ParseInfo());
		auto structure = get<1>(args);
		for (auto definition : inputTheory->definitions()) {
			newTheory->add(definition->clone());
		}
		FormulaUtils::replaceDefinitionsWithCompletion(newTheory,structure);
		return doSymbolicPropagation(newTheory, structure);
	}
};

/**
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundPropagateInference: public TheoryStructureBase {
public:
	GroundPropagateInference()
			: TheoryStructureBase("groundpropagate",
					"Return a structure, made more precise than the input by grounding and unit propagation on the theory. Returns nil when propagation results in an inconsistent structure") {
		setNameSpace(getInternalNamespaceName());

	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		GroundingPropagation propagator;
		auto sols = propagator.propagate(get<0>(args), get<1>(args));
		return postProcess(sols);

	}
};

/**
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */

class OptimalPropagateInference: public TheoryStructureBase {
public:
	OptimalPropagateInference()
			: TheoryStructureBase("optimalpropagate",
					"Return a structure, made more precise than the input by generating all models and checking which literals always have the same truth value.\nThis propagation is complete: everything that can be derived from the theory will be derived. Returns nil when propagation results in an inconsistent structure") {
		setNameSpace(getInternalNamespaceName());
	}
    
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
        std::vector<Structure*> sols;
        if(getGlobal()->getOptions()->fullPropagation()==FullProp::ENUMERATION){
            OptimalPropagation propagator;
            sols = propagator.propagate(get<0>(args), get<1>(args));
        }else if(getGlobal()->getOptions()->fullPropagation()==FullProp::ASSUMPTIONS){
            FullPropagation propagator;
            sols = propagator.propagate(get<0>(args), get<1>(args));
        }else if(getGlobal()->getOptions()->fullPropagation()==FullProp::INTERSECTION){
            FullPropagation propagator;
            sols = propagator.propagateNoAssumps(get<0>(args), get<1>(args));
        }else{
            Assert(false); // all options have been exhausted
        }
        return postProcess(sols);
    }
};