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

#include <cmath>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "IncludeComponents.hpp"
#include "generators/ComparisonGenerator.hpp"
#include "generators/SortGenAndChecker.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "generators/TableCheckerAndGenerators.hpp"
#include "structure/StructureComponents.hpp"


using namespace std;

template<class T, class T2>
ostream& operator<<(ostream& stream, pair<T, T2> p){
	stream <<"(" <<p.first <<", "<<p.second <<")";
	return stream;
}

template<typename T>
const DomainElement* domelem(T t){
	return createDomElem(t);
}

namespace Tests{
TEST(BddGenerator, PredForm){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto variable = new Variable(sort);
		auto sortterm = new VarTerm(variable, TermParseInfo()); //var x
		auto symbol = new Predicate("P", {sort}, {}, false);
		Formula* formula = new PredForm(SIGN::POS, symbol, {sortterm}, FormulaParseInfo()); //P(x)
		auto vocabulary = new Vocabulary("V");
		vocabulary->add(sort);
		vocabulary->add(symbol);
		auto structure = new Structure("S", ParseInfo());
		structure->changeVocabulary(vocabulary);
		auto predinter = structure->inter(symbol);

		//P<ct> = {-2,1}
		//P<cf> = {2}
		//P<u>  = {-1,0}
		predinter->makeTrueExactly({createDomElem(1)});
		predinter->makeFalseExactly({createDomElem(2)});
		predinter->makeTrueExactly({createDomElem(-2)});

		auto manager = FOBDDManager::createManager();
		FOBDDFactory bddfactory(manager, NULL);

		BddGeneratorData data;

		data.bdd = bddfactory.turnIntoBdd(formula);
		auto bddset = manager->getVariables({variable});
		ASSERT_EQ(bddset.size(), (uint)1);
		data.bddvars = vector<const FOBDDVariable*>(bddset.cbegin(), bddset.cend());

		BDDToGenerator genfactory(manager);
		auto var = new DomElemContainer();
		Universe u({sorttable});
		data.pattern = {Pattern::OUTPUT};
		data.vars = {var};
		data.structure = structure;
		data.universe = u;
		auto gen = genfactory.create(data);

		set<int> genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var->get()->value()._int;
			genvalues.insert(value);
		}
		ASSERT_EQ(genvalues.size(), (uint)2);
		ASSERT_TRUE(genvalues.find(-2)!=genvalues.end());
		ASSERT_TRUE(genvalues.find(1)!=genvalues.end());
	}

	TEST(BddGenerator, PredFormSmallerThan){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto variable = new Variable(sort);
		auto sortterm = new VarTerm(variable, TermParseInfo()); //var x
		auto vocabulary = new Vocabulary("V");
		vocabulary->add(sort);
		auto less = get(STDPRED::LT, sort);
		auto zero = createDomElem(0);
		auto zeroterm = new DomainTerm(sort, zero, TermParseInfo());
		Formula* formula = new PredForm(SIGN::POS, less, {sortterm, zeroterm}, FormulaParseInfo()); //x<0
		auto structure = new Structure("S", ParseInfo());
		structure->changeVocabulary(vocabulary);

		auto manager = FOBDDManager::createManager();
		FOBDDFactory bddfactory(manager, NULL);

		BddGeneratorData data;

		data.bdd = bddfactory.turnIntoBdd(formula);
		auto bddset = manager->getVariables({variable});
		ASSERT_EQ(bddset.size(), (uint)1);
		data.bddvars = vector<const FOBDDVariable*>(bddset.cbegin(), bddset.cend());

		BDDToGenerator genfactory(manager);
		auto var = new DomElemContainer();
		Universe u({sorttable});
		data.pattern = {Pattern::OUTPUT};
		data.vars = {var};
		data.structure = structure;
		data.universe = u;
		auto gen = genfactory.create(data);

		set<int> genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var->get()->value()._int;
			genvalues.insert(value);
		}
		std::cerr << toString(gen);
		ASSERT_EQ(genvalues.size(), (uint)2);
		ASSERT_TRUE(genvalues.find(-2)!=genvalues.end());
		ASSERT_TRUE(genvalues.find(-1)!=genvalues.end());
	}


	// TODO move to nonbdd test
	TEST(Vocabulary, CreatePTInter){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto symbol = new Predicate("P", {sort}, {}, false);
		auto derivedsymbol = symbol->derivedSymbol(ST_PT);
		auto vocabulary = new Vocabulary("V");
		vocabulary->add(sort);
		EXPECT_NO_THROW(vocabulary->add(derivedsymbol));
		EXPECT_TRUE(vocabulary->contains(symbol));
		EXPECT_TRUE(vocabulary->contains(derivedsymbol));
	}

	TEST(BddGenerator, PTPredForm){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto variable = new Variable(sort);
		auto sortterm = new VarTerm(variable, TermParseInfo());
		auto symbol = new Predicate("P", {sort}, {}, false);
		auto derivedsymbol = symbol->derivedSymbol(ST_PT);
		auto formula = new PredForm(SIGN::POS, derivedsymbol, {sortterm}, FormulaParseInfo());
		auto manager = FOBDDManager::createManager();
		FOBDDFactory bddfactory(manager, NULL);

		BddGeneratorData data;

		data.bdd = bddfactory.turnIntoBdd(formula);
		auto bddset = manager->getVariables({variable});
		Assert(bddset.size()==1);
		data.bddvars = vector<const FOBDDVariable*>(bddset.cbegin(), bddset.cend());

		BDDToGenerator genfactory(manager);
		auto var = new DomElemContainer();
		Universe u({sorttable});

		auto vocabulary = new Vocabulary("V");
		vocabulary->add(sort);
		vocabulary->add(symbol);
		auto structure = new Structure("S", ParseInfo());
		structure->changeVocabulary(vocabulary);
		auto predinter = structure->inter(symbol);
		predinter->makeTrueExactly({createDomElem(1)});
		predinter->makeFalseExactly({createDomElem(2)});
		predinter->makeTrueExactly({createDomElem(-2)});
		data.pattern = {Pattern::OUTPUT};
		data.vars = {var};
		data.structure = structure;
		data.universe = u;
		auto gen = genfactory.create(data);

		set<int> genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var->get()->value()._int;
			genvalues.insert(value);
		}
		ASSERT_EQ(genvalues.size(), (uint)4);
		ASSERT_TRUE(genvalues.find(2)==genvalues.end());
	}

	TEST(BddGenerator, ExistsPredFormBddConstructionOperators){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto variable = new Variable(sort);
		auto sortterm = new VarTerm(variable, TermParseInfo());
		auto symbol = new Predicate("P", {sort}, {}, false);
		Formula* formula = new PredForm(SIGN::POS, symbol, {sortterm}, FormulaParseInfo());
		formula = new QuantForm(SIGN::POS, QUANT::EXIST, {variable}, formula, FormulaParseInfo());
		auto manager = FOBDDManager::createManager();
		FOBDDFactory bddfactory(manager, NULL);
		auto bdd = bddfactory.turnIntoBdd(formula);

		auto bddvar = manager->getVariable(variable);
		auto predkernel = manager->getAtomKernel(symbol, AtomKernelType::AKT_TWOVALUED, vector<const FOBDDTerm*>{bddvar});
		auto testbdd = manager->ifthenelse(predkernel, manager->truebdd(), manager->falsebdd());
		testbdd = manager->existsquantify(bddvar, testbdd);

		ASSERT_EQ(testbdd, bdd);
	}
}
