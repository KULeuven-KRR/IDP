/*
 * InstantiationGrounder.hpp
 *
 *  Created on: Oct 29, 2012
 *      Author: broes
 */

#pragma once

#include "FormulaGrounders.hpp"

class InstantiationGrounder: public FormulaGrounder {
private:
	std::vector<std::pair<const DomElemContainer*, const DomainElement*> > instantiation;
	FormulaGrounder* subgrounder;

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request){
		for(auto insttuple : instantiation){
			*insttuple.first = insttuple.second;
		}
		subgrounder->run(literals, request);
	}

public:
	InstantiationGrounder(AbstractGroundTheory* grounding, std::vector<std::pair<const DomElemContainer*, const DomainElement*> > instantiation, FormulaGrounder* subgrounder, const GroundingContext& context):
		FormulaGrounder(grounding, context), instantiation(instantiation), subgrounder(subgrounder){

		addAll(_varmap, subgrounder->getVarmapping());

		setFormula(subgrounder->getFormula()->cloneKeepVars()); // TODO variable instantiation?

		setMaxGroundSize(subgrounder->getMaxGroundSize());
	}
};
