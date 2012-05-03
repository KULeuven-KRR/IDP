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
	_bddtofo.setVocabulary(theo->vocabulary());
	for (auto it = theo->sentences().cbegin(); it != theo->sentences().cend(); ++it) {
		auto bdd = _factory.turnIntoBdd(*it);
		_counter.count(bdd);
	}
	for (auto def = theo->definitions().cbegin(); def != theo->definitions().cend(); ++def) {
		for (auto rule = (*def)->rules().cbegin(); rule != (*def)->rules().cend(); ++rule) {
			auto bdd = _factory.turnIntoBdd((*rule)->body());
			auto bddvars = _manager.getVariables((*rule)->quantVars());
			bdd = _manager.replaceFreeVariablesByIndices(bddvars, bdd);
			_counter.count(bdd);
		}
	}
	//Go over the theory again, and tseitinify everything that occurs a lot.
	//First go over the definitions, because every definition defines all tseitins it uses.
	//When this is done, go over the sentences.

	for (auto def = theo->definitions().cbegin(); def != theo->definitions().cend(); ++def) {
		_bddtofo.startDefinition();
		for (auto rule = (*def)->rules().cbegin(); rule != (*def)->rules().cend(); ++rule) {
			auto bdd = _factory.turnIntoBdd((*rule)->body());
			auto bddvars = _manager.getVariables((*rule)->quantVars());
			bdd = _manager.replaceFreeVariablesByIndices(bddvars, bdd);
			//TODO: set debruyn mapping
			std::cerr << toString(bdd) << endl << endl;
			auto newbody = _bddtofo.createFormulaWithFreeVars(bdd, bddvars);
			//(*rule)->body()->recursiveDeleteKeepVars();
			(*rule)->body(newbody);
		}
		_bddtofo.finishDefinitionAndAddConstraints(*def);
	}

	std::cerr << "After defs " << endl << toString(theo);

	std::vector<Formula*>& sentences = theo->sentences();
	for (size_t i = 0; i < sentences.size(); i++) {
		auto bdd = _factory.turnIntoBdd(sentences[i]);
		//sentences[i]->recursiveDelete();
		std::cerr << toString(bdd) << endl << endl;
		auto newsentence = _bddtofo.createFormula(bdd);
		theo->sentence(i, newsentence);
	}
	std::cerr << "so far " << endl << toString(theo);

	theo = _bddtofo.addTseitinConstraints(theo);
	std::cerr << "now " << endl << toString(theo);

	return theo;
}

