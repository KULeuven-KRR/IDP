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

#include <dirent.h>
#include <exception>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "IncludeComponents.hpp"
#include "generators/ComparisonGenerator.hpp"
#include "generators/SortGenAndChecker.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "TestUtils.hpp"
#include "utils/FileManagement.hpp"
#include "generators/TableCheckerAndGenerators.hpp"

using namespace std;

template<typename T>
const DomainElement* domelem(T t) {
	return createDomElem(t);
}

namespace Tests {

vector<string> generateListOfPropagationFiles() {
	vector<string> testdirs {"propagate/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

// TODO need bdds
/*TEST(PropagationTest, PredFormBound) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
	auto sort = new Sort("x", sorttable);
	auto variable = new Variable(sort);
	auto x = new VarTerm(variable, TermParseInfo());
	auto p = new Predicate("p", { sort }, false);
	auto q = new Predicate("q", { sort }, false);
	auto px = new PredForm(SIGN::POS, p, { x }, FormulaParseInfo());
	auto qx = new PredForm(SIGN::POS, q, { x }, FormulaParseInfo());
	auto notpximplqx = new BoolForm(SIGN::POS, false, px, qx, FormulaParseInfo());
	auto forallpximplqx = new QuantForm(SIGN::POS, QUANT::UNIV, { variable }, notpximplqx, FormulaParseInfo());
	auto vocabulary = new Vocabulary("V");
	vocabulary->add(p);
	vocabulary->add(q);
	auto theory = new Theory("T", vocabulary, ParseInfo());
	auto structure = new Structure("S", ParseInfo());
	structure->vocabulary(vocabulary);
	auto pinter = structure->inter(p);
	pinter->cf(new PredTable(new FullInternalPredTable(), pinter->universe()));
	auto propagator = generateApproxBounds(theory, structure);
	auto bdd = propagator->evaluate(qx, TruthType::CERTAIN_TRUE);
	BDDToGenerator generatorfactory(propagator->manager());
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	data.universe = pinter->universe();
	data.vars = {new DomElemContainer()};
	data.bddvars = {propagator->manager()->getVariable(variable)};
	data.pattern = {Pattern::OUTPUT};
	auto gen = generatorfactory.create(data);
	int counter = 0;
	for (gen->begin(); not gen->isAtEnd(); gen->operator ++()) {
		counter++;
	}
	ASSERT_EQ(5, counter);
}*/

class LuaPropagationTest: public ::testing::TestWithParam<string> {
};

TEST_P(LuaPropagationTest, ThreeDifferentPropagations) {
	string testfile(getTestDirectory() + "propagatetest.idp");
	cerr << "Testing " << GetParam() << "\n";
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( { GetParam(), testfile }););
	ASSERT_EQ(Status::SUCCESS,result);
}

INSTANTIATE_TEST_CASE_P(Propagation, LuaPropagationTest,  ::testing::ValuesIn(generateListOfPropagationFiles()));

/*TEST(PropagationTest, EquivForm){
 auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
 auto sort = new Sort("x", sorttable);
 auto variable = new Variable(sort);
 auto x = new VarTerm(variable, TermParseInfo());
 auto p = new Predicate("p", {sort}, false);
 auto q = new Predicate("q", {sort}, false);
 auto notpx = new PredForm(SIGN::NEG, p, {x}, FormulaParseInfo());
 auto qx = new PredForm(SIGN::POS, q, {x}, FormulaParseInfo());
 auto pxeqqx = new EquivForm(SIGN::POS, notpx, qx, FormulaParseInfo());
 auto forallpxeqqx = new QuantForm(SIGN::POS, QUANT::UNIV, {variable}, pxeqqx, FormulaParseInfo());
 auto vocabulary = new Vocabulary("V");
 vocabulary->add(p);
 vocabulary->add(q);
 auto theory = new Theory("T", vocabulary, ParseInfo());
 auto structure = new Structure("S", ParseInfo());
 structure->vocabulary(vocabulary);
 auto pinter = structure->inter(p);
 pinter->pf(new PredTable(new FullInternalPredTable(), pinter->universe()));
 auto propagator = generateApproxBounds(theory, structure);
 //cerr <<"Derived bounds: \n";
 //propagator->put(std::cerr);
 auto bdd = propagator->evaluate(forallpxeqqx, TruthType::CERTAIN_TRUE);
 // TODO
 }*/

// TODO
/*TEST(PropagationTest, INF) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
	auto sort = new Sort("x", sorttable);
	auto variable = new Variable(sort);
	auto x = new VarTerm(variable, TermParseInfo());
	auto p = new Predicate("p", { sort }, false);
	auto q = new Predicate("q", { sort }, false);
	auto r = new Predicate("r", { sort }, false);
	auto notpx = new PredForm(SIGN::NEG, p, { x }, FormulaParseInfo());
	auto qx = new PredForm(SIGN::POS, q, { x }, FormulaParseInfo());
	auto notqx = new PredForm(SIGN::NEG, q, { x }, FormulaParseInfo());
	auto rx = new PredForm(SIGN::POS, r, { x }, FormulaParseInfo());
	auto pximplqx = new BoolForm(SIGN::POS, true, notpx, qx, FormulaParseInfo());
	auto qximplrx = new BoolForm(SIGN::POS, true, notqx, rx, FormulaParseInfo());
	auto forallpximplqx = new QuantForm(SIGN::POS, QUANT::UNIV, { variable }, pximplqx, FormulaParseInfo());
	auto forallqximplrx = new QuantForm(SIGN::POS, QUANT::UNIV, { variable }, qximplrx, FormulaParseInfo());
	auto formula = new BoolForm(SIGN::POS, true, forallpximplqx, forallqximplrx, FormulaParseInfo());
	auto vocabulary = new Vocabulary("V");
	vocabulary->add(p);
	vocabulary->add(q);
	vocabulary->add(r);
	auto theory = new Theory("T", vocabulary, ParseInfo());
	theory->add(formula);
	auto structure = new Structure("S", ParseInfo());
	structure->vocabulary(vocabulary);
	auto pinter = structure->inter(p);
	pinter->ct(new PredTable(new FullInternalPredTable(), pinter->universe()));
	auto propagator = generateApproxBounds(theory, structure);
	auto bdd = propagator->evaluate(rx, TruthType::CERTAIN_TRUE);
	BDDToGenerator generatorfactory(propagator->manager());
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	data.universe = pinter->universe();
	data.vars = {new DomElemContainer()};
	data.bddvars = {propagator->manager()->getVariable(variable)};
	data.pattern = {Pattern::OUTPUT};
	auto gen = generatorfactory.create(data);
	int counter = 0;
	for (gen->begin(); not gen->isAtEnd(); gen->operator ++()) {
		counter++;
	}
	ASSERT_EQ(5, counter);
}*/
}
