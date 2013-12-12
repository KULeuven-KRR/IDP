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

#include "SimplifyWithBDDs.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/bddvisitors/BddToFormula.hpp"
#include "fobdds/FoBddFactory.hpp"

SimplifyWithBdds::SimplifyWithBdds() {
}

Theory* SimplifyWithBdds::execute(Theory* theo) {
	std::vector<Formula*>& sentences = theo->sentences();
	for (size_t i = 0; i < sentences.size(); i++) {
		auto f = execute(sentences[i]);
		theo->sentence(i, f);
	}
	return theo;

}

Formula* SimplifyWithBdds::execute(Formula* f) {
	auto manager = FOBDDManager::createManager();
	FOBDDFactory factory(manager);
	auto bdd = factory.turnIntoBdd(f);
	f->recursiveDelete();
	BDDToFO back(manager);
	return back.createFormula(bdd);
}

