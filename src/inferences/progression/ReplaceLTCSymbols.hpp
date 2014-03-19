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
#include <vector>

class Vocabulary;
struct LTCVocInfo;
class Term;
class Formula;
class PredForm;
class FuncTerm;
class PFSymbol;

class ReplaceLTCSymbols: public TheoryMutatingVisitor {
	VISITORFRIENDS()

public:
	template<class T>
	static T* replaceSymbols(T* t, Vocabulary* ltcVoc, bool replaceByNext) {
		auto g = ReplaceLTCSymbols(ltcVoc, replaceByNext);
		return t->accept(&g);
	}
private:
	Vocabulary* _ltcVoc;
	bool _replacyeByNext;
	const LTCVocInfo* _ltcVocInfo;

	ReplaceLTCSymbols(Vocabulary* ltcVoc, bool replaceByNext);
	~ReplaceLTCSymbols();

	virtual Formula* visit(PredForm*);
	virtual Term* visit(FuncTerm*);

	bool shouldReplace(PFSymbol* pf);
	bool isNextTime( Term* t);

	template<class T>
	PFSymbol* getNewSymbolAndSubterms(T* pf, std::vector<Term*>& newsubterms, PFSymbol* symbol);
};
