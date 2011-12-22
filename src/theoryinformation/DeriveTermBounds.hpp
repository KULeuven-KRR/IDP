/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef DERIVETERMBOUNDS_HPP_
#define DERIVETERMBOUNDS_HPP_

//TODO Implement a TermVisitor for this kind of visit functionality...

#include "visitors/TheoryVisitor.hpp"
#include <vector>

class AbstractStructure;
class DomainElement;

/**
 * Derives lower and upper bounds for a term given a structure.
 * Returns NULL when it cannot derive a bound.
 */
class DeriveTermBounds: public TheoryVisitor {
	VISITORFRIENDS()
private:
	const AbstractStructure* _structure;
	const DomainElement* _minimum;
	const DomainElement* _maximum;
	std::vector<const DomainElement*> _subtermminimums;
	std::vector<const DomainElement*> _subtermmaximums;

public:
	template<typename T>
	std::vector<const DomainElement*> execute(T t, const AbstractStructure* str) {
		Assert(str != NULL);
		_structure = str;
		t->accept(this);
		return std::vector<const DomainElement*>{ _minimum, _maximum };
	}

	const DomainElement* getMinimum() { return _minimum; }
	const DomainElement* getMaximum() { return _maximum; }

protected:
	void traverse(const Term*);
	void traverse(const SetExpr*);
	void visit(const DomainTerm*);
	void visit(const VarTerm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);

	// Disable all other visit methods..
	void traverse(const Formula*) {}

	void visit(const EnumSetExpr*) {}
	void visit(const QuantSetExpr*) {}

	void visit(const Theory*) {}
	void visit(const AbstractGroundTheory*) {}
	void visit(const GroundTheory<GroundPolicy>*) {}

	void visit(const PredForm*) {}
	void visit(const EqChainForm*) {}
	void visit(const EquivForm*) {}
	void visit(const BoolForm*) {}
	void visit(const QuantForm*) {}
	void visit(const AggForm*) {}

	void visit(const Rule*) {}
	void visit(const Definition*) {}
	void visit(const FixpDef*) {}

	void visit(const GroundDefinition*) {}
	void visit(const PCGroundRule*) {}
	void visit(const AggGroundRule*) {}
	void visit(const GroundSet*) {}
	void visit(const GroundAggregate*) {}

	void visit(const CPReification*) {}
	void visit(const CPVarTerm*) {}
	void visit(const CPWSumTerm*) {}
	void visit(const CPSumTerm*) {}
};

#endif /* DERIVEBOUNDS_HPP_ */
