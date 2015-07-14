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

#include <cstdlib>
#include <memory>

#include "UnsatCoreExtraction.hpp"

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "utils/ListUtils.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

// TODO add markers for structure information and for function constraints

#include "MinimizeMarkers.hpp"
#include "AddMarkers.hpp"
vector<TheoryComponent*> UnsatCoreExtraction::extractCore(AbstractTheory* atheory, Structure* structure) {
	auto intheory = dynamic_cast<Theory*>(atheory);
	if (intheory == NULL) {
		throw notyetimplemented("Unsatcore extraction for non first-order theories");
	}

	cout << ">>> Generating an unsatisfiable subset of the given theory.\n";

	stringstream ss;
	ss << "unsatcore_voc" << getGlobal()->getNewID();
	auto voc = new Vocabulary(ss.str());
	voc->add(intheory->vocabulary());
	auto newtheory = intheory->clone();
	newtheory->vocabulary(voc);
	auto s = structure->clone();
	s->changeVocabulary(newtheory->vocabulary());

	//	TODO dropping function constraints is not possible as MX is not able to read a model back in that does not satisfy its functions
	//	solution: replace functions with predicate symbols, without adding function constraints!
//	map<Function*, Formula*> func2form;
//	FormulaUtils::addFuncConstraints(newtheory, newtheory->vocabulary(), func2form, getOption(CPSUPPORT));
//	for (auto f2f : func2form) {
//		newtheory->add(f2f.second);
//	}

	auto am = new AddMarkers();
	newtheory = am->execute(newtheory);

	vector<DomainAtom> output = minimizeAssumps(newtheory, s, { { }, am->getMarkers() });

	auto coreresult = am->getComponentsFromMarkers(output);
	delete (am);
	newtheory->recursiveDelete();
	delete (s);
	delete (voc);
	return coreresult;
}
