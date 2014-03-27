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

#pragma once

#include <memory>
#include "lua/luaconnection.hpp"

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
class FOBDDVariable;

namespace Tests {

class DataManager {
public:
	DataManager();
	~DataManager();
};

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
	Predicate* s; //binary predicate S of ([-2,2],[-2,2])
	Vocabulary* vocabulary;
	Structure* structure;

	PredForm* px; //P(x)
	PredForm* qx; //Q(x)
	PredForm* rx; //R(x)
	PredForm* sxx; //S(x,x)
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
	EqChainForm* xF; // 0 =< 0 =< 0
	AggForm* maxxpxgeq0; // MAX{x|P(x)} >= 0
};

struct BDDTestingSet1{
	TestingSet1 ts1;
	std::shared_ptr<FOBDDManager> manager;
	const FOBDD* truebdd;
	const FOBDD* falsebdd;
	const FOBDDVariable* x;
	const FOBDD* px;
	const FOBDD* qx;
	const FOBDD* pxandqx;
	const FOBDD* p0vq0;
	const FOBDD* Axpx; // !x: P(x)
	const FOBDD* nAxpx; // ~!x: P(x)
	const FOBDD* nExqx; // ?x: Q(x)
};

TestingSet1 getTestingSet1();

BDDTestingSet1 getBDDTestingSet1(int pxmin, int pxmax, int qxmin, int qxmax);
void cleanTestingSet1();

}
