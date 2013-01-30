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

#ifndef CHECKPARTIALTERM_HPP_
#define CHECKPARTIALTERM_HPP_

#include "visitors/TheoryVisitor.hpp"

/**
 * Class to implement TermUtils::isPartial
 */
class CheckPartialTerm: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool _result;
public:
	bool execute(Term* t) {
		_result = false;
		t->accept(this);
		return _result;
	}

protected:
	void visit(const VarTerm*) {
	}
	void visit(const DomainTerm*) {
	}
	void visit(const AggTerm*) {
	} // NOTE: we are not interested whether contains
	  // partial functions. So we don't visit it recursively.

	void visit(const FuncTerm* ft) {
		if (ft->function()->partial()) {
			_result = true;
			return;
		} else {
			for (size_t argpos = 0; argpos < ft->subterms().size(); ++argpos) {
				if (not SortUtils::isSubsort(ft->subterms()[argpos]->sort(), ft->function()->insort(argpos))) {
					_result = true;
					return;
				}
			}
			TheoryVisitor::traverse(ft);
		}
	}
};

#endif /* CHECKPARTIALTERM_HPP_ */
