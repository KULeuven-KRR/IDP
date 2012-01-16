/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDDFACTORY_HPP_
#define FOBDDFACTORY_HPP_

#include "visitors/TheoryVisitor.hpp"

class Formula;
class Term;
class FOBDD;
class FOBDDTerm;
class FOBDDKernel;
class VarTerm;
class DomainTerm;
class FuncTerm;
class AggTerm;
class BoolForm;
class PredForm;
class QuantForm;
class EqChainForm;
class AggForm;
class Vocabulary;
class FOBDDManager;

/**
 * Class to transform first-order formulas to BDDs
 */
class FOBDDFactory: public TheoryVisitor {
	VISITORFRIENDS()
private:
	FOBDDManager* _manager;
	Vocabulary* _vocabulary;

	// Return values
	const FOBDD* _bdd;
	const FOBDDKernel* _kernel;
	const FOBDDTerm* _argument;

	void visit(const VarTerm* vt);
	void visit(const DomainTerm* dt);
	void visit(const FuncTerm* ft);
	void visit(const AggTerm* at);

	void visit(const PredForm* pf);
	void visit(const BoolForm* bf);
	void visit(const QuantForm* qf);
	void visit(const EqChainForm* ef);
	void visit(const AggForm* af);
	void visit(const EquivForm* af);

public:
	FOBDDFactory(FOBDDManager* m, Vocabulary* v = 0)
			: _manager(m), _vocabulary(v) {
	}

	const FOBDD* turnIntoBdd(const Formula* f);
	const FOBDDTerm* turnIntoBdd(const Term* t);
};

#endif /* FOBDDFACTORY_HPP_ */
