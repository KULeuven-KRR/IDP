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

#include "Query.hpp"

#include "IncludeComponents.hpp"
#include "theory/Query.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "theory/TheoryUtils.hpp"
#include "creation/cppinterface.hpp"

PredTable* Querying::solveQuery(Query* q, Structure const * const structure) const {
	std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure;
	auto alltwoval = true;
	for(auto s: FormulaUtils::collectSymbols(q->query())){
		if(not structure->inter(s)->approxTwoValued()){
			alltwoval = false;
		}
	}
	if(not alltwoval){
		symbolicstructure = generateNonLiftedBounds(new Theory("",structure->vocabulary(),ParseInfo()), structure);
	}
	return solveQuery(q,structure,symbolicstructure);
}

PredTable* Querying::solveQuery(Query* q, Structure const * const structure, std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure) const {
	if(not VocabularyUtils::isSubVocabulary(q->vocabulary(), structure->vocabulary())){
		throw IdpException("The structure of the query does not interpret all symbols in the query.");
	}
	// translate the formula to a bdd
	std::shared_ptr<FOBDDManager> manager;
	const FOBDD* bdd = NULL;
	auto newquery = q->query()->clone();
	newquery = FormulaUtils::simplify(newquery,structure);
	auto alltwoval = true;
	for(auto s: FormulaUtils::collectSymbols(q->query())){
		if(not structure->inter(s)->approxTwoValued()){
			alltwoval = false;
		}
	}
	if(not alltwoval){
		// Note: first graph, because generateBounds is currently incorrect in case of three-valued function terms.
		newquery = FormulaUtils::graphFuncsAndAggs(newquery,structure,{}, true,false);
		bdd = symbolicstructure->evaluate(newquery, TruthType::CERTAIN_TRUE, structure);
		manager = symbolicstructure->obtainManager();
	} else {
		//When working two-valued, we can simply turn formula to BDD
		manager = FOBDDManager::createManager();
		FOBDDFactory factory(manager);
		bdd = factory.turnIntoBdd(newquery);
	}
	newquery->recursiveDelete();

	return solveBdd(q->variables(), manager, bdd, structure);
}

PredTable* Querying::solveBdd(const std::vector<Variable*>& vars, std::shared_ptr<FOBDDManager> manager, const FOBDD* bdd, Structure const * const structure) const {
	if(not structure->isConsistent()){
		throw IdpException("Querying cannot be applied to inconsistent structures");
	}
	if(not structure->approxTwoValued()){
		Warning::warning("Querying for three-valued structures can behave incorrectly.");
	}

	Assert(bdd != NULL);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-BDD:" << "\n" << print(bdd) << "\n";
	}
	Assert(manager != NULL);

	varset setvars(vars.cbegin(), vars.cend());
	auto bddvars = manager->getVariables(setvars);
	fobddindexset bddindices;

	Assert(bdd != NULL);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	std::map<Variable*,const DomElemContainer*> varsToDomElemContainers;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		data.pattern.push_back(Pattern::OUTPUT);
		auto dec = varsToDomElemContainers.find(*it);
		if (dec == varsToDomElemContainers.cend()) {
			auto res = new const DomElemContainer();
			varsToDomElemContainers[*it] = res;
			data.vars.push_back(res);

		} else {
			data.vars.push_back(dec->second);
		}
		data.bddvars.push_back(manager->getVariable(*it));
		data.universe.addTable(structure->inter((*it)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-Generator:" << "\n" << print(generator) << "\n";
	}

	// Create an empty table
	std::vector<SortTable*> vst;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}

	// Execute the query
	Universe univ(vst);
	auto result = TableUtils::createPredTable(univ);
	ElementTuple currtuple(vars.size());
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < vars.size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete generator;
	return result;
}

PredTable* Querying::solveBDDQuery(const FOBDD* bdd, Structure const * const structure) const {
	auto manager= bdd->manager();
	Assert(bdd != NULL);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "FOBDD-Query-BDD:" << "\n" << print(bdd) << "\n";
	}
	Assert(manager != NULL);

	auto bddvars = variables(bdd,manager);

	fobddindexset bddindices;

	Assert(bdd != NULL);

	// Create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	std::map<Variable*,const DomElemContainer*> varsToDomElemContainers;
	for (auto it: bddvars) {
		auto var = it->variable();
		data.pattern.push_back(Pattern::OUTPUT);
		auto dec = varsToDomElemContainers.find(var);
		if (dec == varsToDomElemContainers.cend()) {
			auto res = new const DomElemContainer();
			varsToDomElemContainers[var] = res;
			data.vars.push_back(res);
		} else {
			data.vars.push_back(dec->second);
		}
		data.bddvars.push_back(manager->getVariable(var));
		data.universe.addTable(structure->inter((var)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "FOBDD-Query-Generator:" << "\n" << print(generator) << "\n";
	}

	// Create an empty table
	std::vector<SortTable*> vst;
	for (auto it:bddvars) {
		auto var = it->variable();
		vst.push_back(structure->inter((var)->sort()));
	}

	// Execute the query
	Universe univ(vst);
	auto result = TableUtils::createPredTable(univ);
	ElementTuple currtuple(bddvars.size());
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < bddvars.size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete generator;
	return result;
}

bool evaluate(TheoryComponent* comp, const Structure* structure){
	if(not structure->isConsistent() || not structure->satisfiesFunctionConstraints()){
		return false;
	}
	auto form = dynamic_cast<Formula*>(comp);
	if(form!=NULL){
		for(auto s: FormulaUtils::collectSymbols(form)){
			if(not structure->inter(s)->approxTwoValued()){
				throw notyetimplemented("Cannot evaluate a formula in a three-valued structure");
			}
		}
		if(not form->freeVars().empty()){
			throw IdpException("The input formula had free variables");
		}

		Query q("Eval", {}, form, {});
		auto result = Querying::doSolveQuery(&q, structure);
		if(result->empty()){ // No answers => false
			return false;
		}else{ // Empty tuple => true
			return true;
		}
	}
	auto def = dynamic_cast<Definition*>(comp);
	if(def!=NULL){
		def = def->clone();
		auto newstruct = structure->clone();
		auto split = getOption(SPLIT_DEFS);
		auto xsb = getOption(XSB);
		setOption(SPLIT_DEFS, false);
		setOption(XSB, true);
		auto success = CalculateDefinitions::doCalculateDefinition(def, newstruct)._hasModel;
		setOption(SPLIT_DEFS, split);
		setOption(XSB, xsb);
		def->recursiveDelete();
		delete(newstruct);
		return success;
	}
	throw IdpException("Component not supported in evaluate");
}

const DomainElement* evaluate(Term* term, const Structure* structure){
	for(auto s: FormulaUtils::collectSymbols(term)){
		if(not structure->inter(s)->approxTwoValued()){
			throw notyetimplemented("Cannot evaluate a term in a three-valued structure");
		}
	}
	if(not term->freeVars().empty()){
		throw IdpException("The input term had free variables");
	}

	auto var = Gen::var(term->sort());
	auto& pf = Gen::operator ==(*term->clone(), *new VarTerm(var,{}));

	Query q("Eval", {var}, &pf, {});
	auto result = Querying::doSolveQuery(&q, structure);
	pf.recursiveDelete();
	if(result->empty()){
		return NULL; // partial
	}else{
		return result->begin().operator *()[0];
	}
}
