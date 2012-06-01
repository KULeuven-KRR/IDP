/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ADDFUNCCON_HPP_
#define ADDFUNCCON_HPP_

#include "visitors/TheoryVisitor.hpp"
#include <set>
#include <cstdio>

class Function;
class Vocabulary;

class AddFuncConstraints: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::set<Function*> _symbols;
	std::set<const Function*> _cpfuncsymbols;
	const Vocabulary* _vocabulary;
	bool _cpsupport;

public:
	// Returns a new theory containing the func constraints
	template<class T>
	Theory* execute(const T* t, const Vocabulary* v, bool cpsupport){
		_vocabulary = v;
		_cpsupport = cpsupport;
		t->accept(this);
		return createTheory(t->pi());
	}

protected:
	Theory* createTheory(const ParseInfo& pi) const;
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* ADDFUNCCON_HPP_ */
