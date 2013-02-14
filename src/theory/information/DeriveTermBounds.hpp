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

//TODO Implement a TermVisitor for this kind of visit functionality...

#include "visitors/TheoryVisitor.hpp"
#include "theory/term.hpp"
#include <vector>

class Structure;
class DomainElement;

/**
 * Derives lower and upper bounds for a term given a structure.
 * Returns <NULL, NULL> when it cannot derive a bound.
 */
class DeriveTermBounds: public TheoryVisitor {
	VISITORFRIENDS()
private:
	const Structure* _structure;
	size_t _level;
	const DomainElement* _minimum;
	const DomainElement* _maximum;
	std::vector<std::vector<const DomainElement*>> _subtermminimums;
	std::vector<std::vector<const DomainElement*>> _subtermmaximums;

	bool _underivable;

public:
	std::vector<const DomainElement*> execute(const Term* t, const Structure* str) {
		Assert(str != NULL);
		_structure = str;
		_level = 0;
		_underivable = false;
		t->accept(this);
		if(_underivable){
			return std::vector<const DomainElement*> { NULL, NULL };
		}else{
			return std::vector<const DomainElement*> { _minimum, _maximum };
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
			if(_underivable){
				return;
			}
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
		throw InternalIdpException("Cannot derive apply derivation of term bounds on Formula.");
	}
	void visit(const EnumSetExpr*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on EnumSetExpr.");
	}
	void visit(const QuantSetExpr*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on QuantSetExpr.");
	}
	void visit(const Theory*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on Theory.");
	}
	void visit(const AbstractGroundTheory*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on AbstractGroundTheory.");
	}
	void visit(const GroundTheory<GroundPolicy>*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on GroundTheory<GroundPolicy>.");
	}
	void visit(const PredForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on PredForm.");
	}
	void visit(const EqChainForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on EqChainForm.");
	}
	void visit(const EquivForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on EquivForm.");
	}
	void visit(const BoolForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on BoolForm.");
	}
	void visit(const QuantForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on QuantForm.");
	}
	void visit(const AggForm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on AggForm.");
	}
	void visit(const Rule*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on Rule.");
	}
	void visit(const Definition*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on Definition.");
	}
	void visit(const FixpDef*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on FixpDef.");
	}
	void visit(const GroundDefinition*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on GroundDefinition.");
	}
	void visit(const PCGroundRule*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on PCGroundRule.");
	}
	void visit(const AggGroundRule*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on AggGroundRule.");
	}
	void visit(const GroundSet*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on GroundSet.");
	}
	void visit(const GroundAggregate*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on GroundAggregate.");
	}
	void visit(const CPReification*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on CPReification.");
	}
	void visit(const CPVarTerm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on CPVarTerm.");
	}
	void visit(const CPSetTerm*) {
		throw InternalIdpException("Cannot derive apply derivation of term bounds on CPWSumTerm.");
	}
};
