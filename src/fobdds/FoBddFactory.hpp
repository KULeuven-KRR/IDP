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
class FOBDDSetExpr;
class Structure;
class FOBDDEnumSetExpr;
class FOBDDQuantSetExpr;

/**
 * Class to transform first-order formulas to BDDs
 */
class FOBDDFactory: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::shared_ptr<FOBDDManager> _manager;
	Vocabulary* _vocabulary;

	// Return values
	const FOBDD* _bdd;
	const FOBDDKernel* _kernel;
	const FOBDDTerm* _term;
	const FOBDDEnumSetExpr* _enumset;
	const FOBDDQuantSetExpr* _quantset;

	void visit(const VarTerm* vt);
	void visit(const DomainTerm* dt);
	void visit(const FuncTerm* ft);
	void visit(const AggTerm* at);
	void visit(const EnumSetExpr* se);
	void visit(const QuantSetExpr* se);

	void visit(const PredForm* pf);
	void visit(const BoolForm* bf);
	void visit(const QuantForm* qf);
	void visit(const EqChainForm* ef);
	void visit(const AggForm* af);
	void visit(const EquivForm* af);

public:
	FOBDDFactory(std::shared_ptr<FOBDDManager> m, Vocabulary* v = NULL)
			: _manager(m), _vocabulary(v) {
	}

	// if a structure is given, this can be used for efficiently unnesting terms (deriving term bounds)
	const FOBDD* turnIntoBdd(const Formula* f, Structure* s = NULL);
	const FOBDDTerm* turnIntoBdd(const Term* t);
};

#endif /* FOBDDFACTORY_HPP_ */
