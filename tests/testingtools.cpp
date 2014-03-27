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

#include "testingtools.hpp"

#include "IncludeComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "structure/StructureComponents.hpp"
#include "lua/luaconnection.hpp"

namespace Tests {

DataManager::DataManager() {
	LuaConnection::makeLuaConnection();
}
DataManager::~DataManager() {
	LuaConnection::closeLuaConnection();
	GlobalData::close();
}

TestingSet1 getTestingSet1() {
	TestingSet1 testingSet;
	testingSet.sorttable = TableUtils::createSortTable(-2, 2); //[-2,2]
	testingSet.sort = new Sort("sort", testingSet.sorttable); // sort  [-2,2]
	testingSet.x = new Variable(testingSet.sort); // variable of [-2,2]
	testingSet.sortterm = new VarTerm(testingSet.x, TermParseInfo()); //varterm of [-2,2]
	testingSet.nul = DomainElementFactory::createGlobal()->create(0);
	testingSet.nulterm = new DomainTerm(testingSet.sort, testingSet.nul, TermParseInfo());
	testingSet.p = new Predicate("P", { testingSet.sort }, {}, false); //predicate P of [-2,2]
	testingSet.q = new Predicate("Q", { testingSet.sort }, {}, false); //predicate Q of [-2,2]
	testingSet.r = new Predicate("R", { testingSet.sort }, {}, false);
	testingSet.s = new Predicate("S", { testingSet.sort,testingSet.sort }, {}, false);
	testingSet.vocabulary = new Vocabulary("V");
	testingSet.vocabulary->add(testingSet.sort);
	testingSet.vocabulary->add(testingSet.p);
	testingSet.vocabulary->add(testingSet.q);
	testingSet.vocabulary->add(testingSet.r);
	testingSet.vocabulary->add(testingSet.s);
	testingSet.structure = new Structure("S", ParseInfo());
	testingSet.structure->changeVocabulary(testingSet.vocabulary);

	testingSet.px = new PredForm(SIGN::POS, testingSet.p, { testingSet.sortterm }, FormulaParseInfo()); //P(x)
	testingSet.qx = new PredForm(SIGN::POS, testingSet.q, { testingSet.sortterm }, FormulaParseInfo()); //Q(x)
	testingSet.rx = new PredForm(SIGN::POS, testingSet.r, { testingSet.sortterm }, FormulaParseInfo()); //Q(x)
	testingSet.sxx = new PredForm(SIGN::POS, testingSet.s, { testingSet.sortterm,testingSet.sortterm->clone() }, FormulaParseInfo()); //S(x,x)
	testingSet.p0 = new PredForm(SIGN::POS, testingSet.p, { testingSet.nulterm }, FormulaParseInfo()); //P(0)
	testingSet.q0 = new PredForm(SIGN::POS, testingSet.q, { testingSet.nulterm }, FormulaParseInfo()); //Q(0)
	testingSet.r0 = new PredForm(SIGN::POS, testingSet.r, { testingSet.nulterm }, FormulaParseInfo()); //Q(0)
	testingSet.xpx = new QuantSetExpr( { testingSet.x }, testingSet.px, testingSet.sortterm, SetParseInfo()); //{x|p(x)}
	testingSet.maxxpx = new AggTerm(new EnumSetExpr( { testingSet.xpx }, testingSet.xpx->pi()), AggFunction::MAX, TermParseInfo()); //MAX{x|p(x)}

	testingSet.np0iffq0 = new EquivForm(SIGN::NEG, testingSet.p0, testingSet.q0, FormulaParseInfo()); // ~(P(0) <=> Q(0))
	testingSet.p0vq0 = new BoolForm(SIGN::POS, false, testingSet.p0, testingSet.q0, FormulaParseInfo()); //P(0) | Q(0)
	testingSet.Axpx = new QuantForm(SIGN::POS, QUANT::UNIV, { testingSet.x }, testingSet.px, FormulaParseInfo()); // !x: P(x)
	testingSet.nAxpx = new QuantForm(SIGN::NEG, QUANT::UNIV, { testingSet.x }, testingSet.px, FormulaParseInfo()); // ~!x: P(x)
	testingSet.nExqx = new QuantForm(SIGN::NEG, QUANT::EXIST, { testingSet.x }, testingSet.qx, FormulaParseInfo()); // ?x: Q(x)
	testingSet.xF = new EqChainForm(SIGN::POS, true, {testingSet.nulterm, testingSet.nulterm, testingSet.nulterm}, {CompType::LEQ, CompType::LEQ}, FormulaParseInfo()); // 0 =< 0 =< 0
	testingSet.maxxpxgeq0 = new AggForm(SIGN::POS, testingSet.nulterm, CompType::LEQ, testingSet.maxxpx, FormulaParseInfo()); // MAX{x|P(x)} >= 0
	return testingSet;
}

BDDTestingSet1 getBDDTestingSet1(int pxmin, int pxmax, int qxmin, int qxmax) {
	BDDTestingSet1 result;
	auto manager = FOBDDManager::createManager();
	auto ts1 = getTestingSet1();
	result.ts1 = ts1;
	result.manager = manager;
	auto factory = FOBDDFactory(manager);
	result.truebdd = manager->truebdd();
	result.falsebdd = manager->falsebdd();
	result.x = manager->getVariable(ts1.x);
	result.px = factory.turnIntoBdd(ts1.px);
	result.qx = factory.turnIntoBdd(ts1.qx);
	result.pxandqx = manager->conjunction(result.px,result.qx);

	result.p0vq0 = factory.turnIntoBdd(ts1.p0vq0);
	result.Axpx = factory.turnIntoBdd(ts1.Axpx);
	result.nAxpx = factory.turnIntoBdd(ts1.nAxpx);
	result.nExqx = factory.turnIntoBdd(ts1.nExqx);

	auto prange = new IntRangeInternalSortTable(pxmin, pxmax);
	Universe u = Universe( { ts1.sorttable });
	auto ptp = new PredTable(prange, u);
	auto pxinter = new PredInter(ptp, true);

	auto qrange = new IntRangeInternalSortTable(qxmin, qxmax);
	auto ptq = new PredTable(qrange, u);
	auto qxinter = new PredInter(ptq, true);

	ts1.structure->changeInter(ts1.p, pxinter);
	ts1.structure->changeInter(ts1.q, qxinter);

	return result;
}

void cleanTestingSet1() {
	//FIXME: memory management: what should be deleted and what not?  Gives segmentation errors at the moment
	//delete ts.sorttable;
	//delete sort; Should not be done, will be deleted when it has no more vocabularies
	//delete ts.sortterm;

	//delete ts.x;
	//delete ts.nul;
	//delete ts.nulterm;
	//delete ts.p;Should not be done, will be deleted when it has no more vocabularies
	//delete ts.q;Should not be done, will be deleted when it has no more vocabularies
	//delete ts.vocabulary;

//	delete ts.structure;
//
//	delete ts.px;
//	delete ts.qx;
//	delete ts.p0;
//	delete ts.q0;
//	delete ts.xpx;
//	delete ts.maxxpx;
//
//	delete ts.np0iffq0;
//	delete ts.p0vq0;
//	delete ts.Axpx;
//	delete ts.nAxpx;
//	delete ts.nExqx;
//	delete ts.xF;
//	delete ts.maxxpxgeq0;
}

}
