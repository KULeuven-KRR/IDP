/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TESTINGTOOLS_HPP_
#define TESTINGTOOLS_HPP_

class Vocabulary;
class Predicate;
class Sort;
class Variable;

class Structure;
class DomainElement;
class SortTable;

class AggForm;
class BoolForm;
class EqChainForm;
class EquivForm;
class PredForm;
class QuantForm;

class AggTerm;
class DomainTerm;
class QuantSetExpr;
class VarTerm;

class FOBDD;
class FOBDDManager;

namespace Tests {

struct TestingSet1 {
	SortTable* sorttable; //[-2,2]
	Sort* sort; // sort  [-2,2]
	Variable* x; // variable of [-2,2]
	VarTerm* sortterm; //varterm of [-2,2]
	const DomainElement* nul; // the element 0
	DomainTerm* nulterm; // the term 0
	Predicate* p; //predicate P of [-2,2]
	Predicate* q; //predicate Q of [-2,2]
	Predicate* r; //predicate R of [-2,2]
	Vocabulary* vocabulary;
	Structure* structure;

	PredForm* px; //P(x)
	PredForm* qx; //Q(x)
	PredForm* rx; //R(x)
	PredForm* p0; //P(0)
	PredForm* q0; //Q(0)
	PredForm* r0; //R(0)
	QuantSetExpr* xpx; //{x|p(x)}
	AggTerm* maxxpx; //MAX{x|p(x)}

	EquivForm* np0iffq0; // ~(P(0) <=> Q(0))
	BoolForm* p0vq0; //P(0) | Q(0)
	QuantForm* Axpx; // !x: P(x)
	QuantForm* nAxpx; // ~!x: P(x)
	QuantForm* nExqx; // ?x: Q(x)
	EqChainForm* xF; // x This is false (empty conjuction)
	AggForm* maxxpxgeq0; // MAX{x|P(x)} >= 0
};

struct BDDTestingSet1{
	TestingSet1 ts1;
	FOBDDManager* manager;
	const FOBDD* truebdd;
	const FOBDD* falsebdd;
	const FOBDD* px;
	const FOBDD* qx;
	const FOBDD* p0vq0;
	const FOBDD* Axpx; // !x: P(x)
	const FOBDD* nAxpx; // ~!x: P(x)
	const FOBDD* nExqx; // ?x: Q(x)
};

TestingSet1 getTestingSet1();

BDDTestingSet1 getBDDTestingSet1(int pxmin, int pxmax, int qxmin, int qxmax);
void cleanTestingSet1();

}

#endif /* TESTINGTOOLS_HPP_ */
