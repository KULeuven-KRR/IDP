/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "testingtools.hpp"

#include "IncludeComponents.hpp"

namespace Tests {

TestingSet1 getTestingSet1() {
	TestingSet1 testingSet;
	testingSet.sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2)); //[-2,2]
	testingSet.sort = new Sort("sort", testingSet.sorttable); // sort  [-2,2]
	testingSet.x = new Variable(testingSet.sort); // variable of [-2,2]
	testingSet.sortterm = new VarTerm(testingSet.x, TermParseInfo()); //varterm of [-2,2]
	testingSet.nul = DomainElementFactory::createGlobal()->create(0);
	testingSet.nulterm = new DomainTerm(testingSet.sort, testingSet.nul, TermParseInfo());
	testingSet.p = new Predicate("P", { testingSet.sort }, false); //predicate P of [-2,2]
	testingSet.q = new Predicate("Q", { testingSet.sort }, false); //predicate Q of [-2,2]
	testingSet.r = new Predicate("R", { testingSet.sort }, false);
	testingSet.vocabulary = new Vocabulary("V");
	testingSet.vocabulary->add(testingSet.sort);
	testingSet.vocabulary->add(testingSet.p);
	testingSet.vocabulary->add(testingSet.q);
	testingSet.vocabulary->add(testingSet.r);
	testingSet.structure = new Structure("S", ParseInfo());
	testingSet.structure->changeVocabulary(testingSet.vocabulary);

	testingSet.px = new PredForm(SIGN::POS, testingSet.p, { testingSet.sortterm }, FormulaParseInfo()); //P(x)
	testingSet.qx = new PredForm(SIGN::POS, testingSet.q, { testingSet.sortterm }, FormulaParseInfo()); //Q(x)
	testingSet.rx = new PredForm(SIGN::POS, testingSet.r, { testingSet.sortterm }, FormulaParseInfo()); //Q(x)
	testingSet.p0 = new PredForm(SIGN::POS, testingSet.p, { testingSet.nulterm }, FormulaParseInfo()); //P(0)
	testingSet.q0 = new PredForm(SIGN::POS, testingSet.q, { testingSet.nulterm }, FormulaParseInfo()); //Q(0)
	testingSet.r0 = new PredForm(SIGN::POS, testingSet.r, { testingSet.nulterm }, FormulaParseInfo()); //Q(0)
	testingSet.xpx = new QuantSetExpr( { testingSet.x }, testingSet.px, testingSet.sortterm, SetParseInfo()); //{x|p(x)}
	testingSet.maxxpx = new AggTerm(new EnumSetExpr({testingSet.xpx}, testingSet.xpx->pi()), AggFunction::MAX, TermParseInfo()); //MAX{x|p(x)}

	testingSet.np0iffq0 = new EquivForm(SIGN::NEG, testingSet.p0, testingSet.q0, FormulaParseInfo()); // ~(P(0) <=> Q(0))
	testingSet.p0vq0 = new BoolForm(SIGN::POS, false, testingSet.p0, testingSet.q0, FormulaParseInfo()); //P(0) | Q(0)
	testingSet.Axpx = new QuantForm(SIGN::POS, QUANT::UNIV, { testingSet.x }, testingSet.px, FormulaParseInfo()); // !x: P(x)
	testingSet.nAxpx = new QuantForm(SIGN::NEG, QUANT::UNIV, { testingSet.x }, testingSet.px, FormulaParseInfo()); // ~!x: P(x)
	testingSet.nExqx = new QuantForm(SIGN::NEG, QUANT::EXIST, { testingSet.x }, testingSet.qx, FormulaParseInfo()); // ?x: Q(x)
	testingSet.xF = new EqChainForm(SIGN::POS, true, testingSet.sortterm, FormulaParseInfo()); // x This is false (empty conjuction)
	testingSet.maxxpxgeq0 = new AggForm(SIGN::POS, testingSet.nulterm, CompType::LEQ, testingSet.maxxpx, FormulaParseInfo()); // MAX{x|P(x)} >= 0
	return testingSet;
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
