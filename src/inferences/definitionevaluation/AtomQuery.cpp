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
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "theory/TheoryUtils.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "theory/TheoryUtils.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

bool AtomQuerying::queryAtom(Query* query, Theory* theory, Structure* structure) const {
	auto pf = dynamic_cast<PredForm*>(query->query());

	ElementTuple tuple;
	for(auto term:pf->subterms()){
		if(term->type()!=TermType::DOM){
			throw notyetimplemented("Querying of formulas that are not a domain atom.");
		}
		tuple.push_back(dynamic_cast<DomainTerm*>(term)->value());
	}
	if (pf == NULL) {
		throw notyetimplemented("Querying of formulas that are not a domain atom.");
	}

	auto symbol = pf->symbol();

//#ifdef WITHXSB
//	// check if we can solve it using XSB
//	Definition* d = NULL;
//	for (auto def : theory->definitions()) {
//		if (def->defsymbols().find(symbol) != def->defsymbols().end()) {
//			auto calculatable = true;
//			auto opens = DefinitionUtils::opens(def);
//			for (auto osym = opens.begin(); osym != opens.end(); ++osym) {
//				if (!structure->inter(*osym)->approxTwoValued()) {
//					calculatable = false;
//				}
//			}
//			if (calculatable) {
//				d = def;
//			}
//		}
//	}
//	if (d != NULL && getOption(XSB)) {
//		auto xsb = XSBInterface::instance();
//		xsb->setStructure(structure);
//		xsb->loadDefinition(d);
//		auto result = xsb->query(symbol, atom2tuple(pf, structure));
//		xsb->exit();
//		return result;
//	}
//#endif

	// Default: evaluate using MX
	auto result = false;
	auto old = getOption(IntType::NBMODELS);
	setOption(IntType::NBMODELS, 0);
	// model expansion
	auto models = ModelExpansion::doModelExpansion(theory, structure, NULL)._models;
	for (auto model : models) {
		if (model->inter(symbol)->isTrue(tuple)) {
			result = true;
		}
	}

	setOption(IntType::NBMODELS, old);
	return result;
}
