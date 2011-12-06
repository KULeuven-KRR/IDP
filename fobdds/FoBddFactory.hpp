#ifndef FOBDDFACTORY_HPP_
#define FOBDDFACTORY_HPP_

#include "visitors/TheoryVisitor.hpp"

class Formula;
class Term;
class FOBDD;
class FOBDDArgument;
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
	const FOBDDArgument* _argument;

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
	FOBDDFactory(FOBDDManager* m, Vocabulary* v = 0) :
			_manager(m), _vocabulary(v) {
	}

	const FOBDD* turnIntoBdd(const Formula* f);
	const FOBDDArgument* turnIntoBdd(const Term* t);
};

#endif /* FOBDDFACTORY_HPP_ */
