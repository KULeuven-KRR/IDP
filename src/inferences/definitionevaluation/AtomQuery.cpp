/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "AtomQuery.hpp"

#include "IncludeComponents.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/querying/xsb/xsbinterface.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "theory/TheoryUtils.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "theory/TheoryUtils.hpp"

bool AtomQuerying::queryAtom(Query* p, Theory* theory, AbstractStructure* structure) const {

	auto q = dynamic_cast<PredForm*>(p->query());
	if(p == NULL){
		throw new IdpException("Formula should be ground atom");
	}

	auto tuple = atom2tuple(q,structure);
	auto symbol = q->symbol();
	// check if we can solve it using XSB
	Definition* d = NULL;
	for(auto def = theory->definitions().begin(); def != theory->definitions().end(); ++def){
		if((*def)->defsymbols().find(symbol) != (*def)->defsymbols().end()){
			bool calculatable = true;
			auto opens = DefinitionUtils::opens(*def);
			for(auto osym = opens.begin();osym != opens.end();++osym){
				if(!structure->inter(*osym)->approxTwoValued()){
					calculatable = false;
				}
			}
			if(calculatable){
				d = *def;
			}
		}
	}


	// translate the formula to a bdd

	bool result = false;

	if( d!= NULL && getOption(XSB)){
		auto xsb = XSBInterface::instance();
		xsb->setStructure(structure);
		xsb->loadDefinition(d);
		result = xsb->query(symbol,tuple);
		xsb->exit();
	} else {
		auto old = getOption(IntType::NBMODELS);
		setOption(IntType::NBMODELS,0);
		// model expansion
		auto models = ModelExpansion::doModelExpansion(theory,structure,NULL)._models;
		for(auto model = models.begin(); model != models.end();++model ){
			if((*model)->inter(symbol)->isTrue(tuple)){
				result = true;
			}
		}

		setOption(IntType::NBMODELS,old);
	}
	return result;
}
