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

#include "visitors/TheoryVisitor.hpp"

class Variable;
class QuantForm;
class QuantSetExpr;
class Rule;

enum class QuantType{
	UNIV,
	EXISTS,
	SET
};


/**
 * Class to collect all quantified variables of a formula.
 * Returns a map from variables to quantsorts depending on how these variables are quantified.
 * This class will not take context into account!
 *
 * If _recursive is true, will collect variables from the entire formula/theory/...
 * If not, it will not check nested quantifications, only quantifications not appearing in other quantifications
 */
class CollectQuantifiedVariables: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::map<Variable*, QuantType> _quantvars;
	bool _recursive;
	QuantType _currentQuant;

public:
	template<typename T>
	std::map<Variable*, QuantType> execute(T f, bool recursive) {
		_recursive = recursive;
		f->accept(this);
		return _quantvars;
	}

protected:
	void visit(const QuantForm*);
	void visit(const QuantSetExpr*);
	void visit(const Rule*);
};

