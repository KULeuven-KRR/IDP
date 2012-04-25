/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IntroduceSharedTseitins.hpp"

#include "IncludeComponents.hpp"


IntroduceSharedTseitins::IntroduceSharedTseitins()
		: _manager(), _factory(&_manager), _counter(&_manager), _bddtofo(&_manager, &_counter) {
}

Theory* IntroduceSharedTseitins::execute(Theory* theo) {
	std::cerr << "Executing on " << endl << toString(theo);
	auto clonetheo = theo->clone();
	//For easy counting, we count on the completion.
	FormulaUtils::addCompletion(clonetheo);
	for (auto it = clonetheo->sentences().cbegin(); it != clonetheo->sentences().cend(); ++it) {
		auto bdd = _factory.turnIntoBdd(*it);
		_counter.count(bdd);
	}
	//Now counter has counted everything, the completed theory is no longer needed
	delete clonetheo;
	//Visit the theory:
	std::vector<Formula*>& sentences = theo->sentences();
	for (size_t i = 0; i < sentences.size(); i++) {
		auto bdd = _factory.turnIntoBdd(sentences[i]);
		delete sentences[i];
		sentences[i] = _bddtofo.createFormula(bdd);
	}
	std::cerr << "so far " << endl << toString(theo);

	return _bddtofo.addTseitinConstraints(theo);
	std::cerr << "now " << endl << toString(theo);

	//theo = TheoryMutatingVisitor::visit(theo);

	//theo = addTseitinEquivalences(theo);
	return theo;
	//Add tseitin definitions
}

