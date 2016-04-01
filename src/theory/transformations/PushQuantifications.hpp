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

#include "visitors/TheoryMutatingVisitor.hpp"

class PushQuantifications : public TheoryMutatingVisitor {

	VISITORFRIENDS()
public:
	// NOTE: requires pushed quantifications, which IS guaranteed when going through theoryUtils.
	// NOTE: non-recursive
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Rule* visit(Rule* rule);
	Formula* visit(QuantForm*);
};

class PushQuantificationsCompletely : public TheoryMutatingVisitor {
	// NOTE: requires pushed equivalences, pushed negations and pushed quantifications

	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Rule* visit(Rule* rule);
	Formula* visit(QuantForm*);
};