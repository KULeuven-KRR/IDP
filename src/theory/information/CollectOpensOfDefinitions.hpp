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

#ifndef COLLECTOPENSOFDEFINITIONS_HPP_
#define COLLECTOPENSOFDEFINITIONS_HPP_

#include <set>

#include "visitors/TheoryVisitor.hpp"

class Definition;
class PFSymbol;

class CollectOpensOfDefinitions: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const Definition* _definition;
	std::set<PFSymbol*> _result;

public:
	const std::set<PFSymbol*>& execute(const Definition* d);
protected:
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* COLLECTOPENSOFDEFINITIONS_HPP_ */
