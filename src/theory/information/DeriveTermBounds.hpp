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
#include <exception>

class AbstractStructure;
class DomainElement;

class BoundsUnderivableException: public std::exception {

};

/**
 * Derives lower and upper bounds for a term given a structure.
 * Returns <NULL, NULL> when it cannot derive a bound.
 */
class DeriveTermBounds: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const AbstractStructure* _structure;
	size_t _level;
	const DomainElement* _minimum;
	const DomainElement* _maximum;
	std::vector<std::vector<const DomainElement*>> _subtermminimums;
	std::vector<std::vector<const DomainElement*>> _subtermmaximums;

public:
	template<typename T>
	std::vector<const DomainElement*> execute(T t, const AbstractStructure* str) {
		Assert(str != NULL);
		_structure = str;
		_level = 0;
		try {
			t->accept(this);
			return std::vector<const DomainElement*> { _minimum, _maximum };
		} catch (const BoundsUnderivableException& e) {
			return std::vector<const DomainElement*> { NULL, NULL };
		}
	}

protected:
	void storeAndClearLists() {
		if (_level >= _subtermminimums.size()) {
			_subtermminimums.push_back(ElementTuple { });
			_subtermmaximums.push_back(ElementTuple { });
		}
		_subtermminimums[_level].clear();
		_subtermmaximums[_level].clear();
	}
	template<typename T>
	void traverse(const T* t) {
		storeAndClearLists();
		for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
			_level++;
			(*it)->accept(this);
			_level--;
			_subtermminimums[_level].push_back(_minimum);
			_subtermmaximums[_level].push_back(_maximum);
		}
	}

	void visit(const DomainTerm*);
	void visit(const VarTerm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);

	// Disable all other visit methods..
	void traverse(const Formula*) {
		Assert(false);
	}

	void visit(const EnumSetExpr*) {
		Assert(false);
	}
	void visit(const QuantSetExpr*) {
		Assert(false);
	}

	void visit(const Theory*) {
		Assert(false);
	}
	void visit(const AbstractGroundTheory*) {
		Assert(false);
	}
	void visit(const GroundTheory<GroundPolicy>*) {
		Assert(false);
	}

	void visit(const PredForm*) {
		Assert(false);
	}
	void visit(const EqChainForm*) {
		Assert(false);
	}
	void visit(const EquivForm*) {
		Assert(false);
	}
	void visit(const BoolForm*) {
		Assert(false);
	}
	void visit(const QuantForm*) {
		Assert(false);
	}
	void visit(const AggForm*) {
		Assert(false);
	}

	void visit(const Rule*) {
		Assert(false);
	}
	void visit(const Definition*) {
		Assert(false);
	}
	void visit(const FixpDef*) {
		Assert(false);
	}

	void visit(const GroundDefinition*) {
		Assert(false);
	}
	void visit(const PCGroundRule*) {
		Assert(false);
	}
	void visit(const AggGroundRule*) {
		Assert(false);
	}
	void visit(const GroundSet*) {
		Assert(false);
	}
	void visit(const GroundAggregate*) {
		Assert(false);
	}

	void visit(const CPReification*) {
		Assert(false);
	}
	void visit(const CPVarTerm*) {
		Assert(false);
	}
	void visit(const CPWSumTerm*) {
		Assert(false);
	}
	void visit(const CPWProdTerm*) {
		Assert(false);
	}
};

#endif /* DERIVEBOUNDS_HPP_ */
