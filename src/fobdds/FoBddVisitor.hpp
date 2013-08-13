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

#include <memory>

class FOBDDManager;
class FOBDD;
class FOBDDAtomKernel;
class FOBDDQuantKernel;
class FOBDDVariable;
class FOBDDDeBruijnIndex;
class FOBDDDomainTerm;
class FOBDDFuncTerm;
class FOBDDTerm;
class FOBDDKernel;
class FOBDDAggKernel;
class FOBDDAggTerm;
class FOBDDQuantSetExpr;
class FOBDDEnumSetExpr;
class FOBDDSetExpr;

class FOBDDVisitor {
protected:
	std::shared_ptr<FOBDDManager> _manager;
public:
	FOBDDVisitor(std::shared_ptr<FOBDDManager> manager)
			: _manager(manager) {
	}
	virtual ~FOBDDVisitor() {
	}

	virtual void visit(const FOBDD*);
	virtual void visit(const FOBDDAtomKernel*);
	virtual void visit(const FOBDDQuantKernel*);
	virtual void visit(const FOBDDAggKernel*);
	virtual void visit(const FOBDDVariable*);
	virtual void visit(const FOBDDDeBruijnIndex*);
	virtual void visit(const FOBDDDomainTerm*);
	virtual void visit(const FOBDDFuncTerm*);
	virtual void visit(const FOBDDAggTerm*);
	virtual void visit(const FOBDDQuantSetExpr*);
	virtual void visit(const FOBDDEnumSetExpr*);

	virtual const FOBDD* change(const FOBDD*);
	virtual const FOBDDKernel* change(const FOBDDAtomKernel*);
	virtual const FOBDDKernel* change(const FOBDDQuantKernel*);
	virtual const FOBDDKernel* change(const FOBDDAggKernel*);
	virtual const FOBDDTerm* change(const FOBDDVariable*);
	virtual const FOBDDTerm* change(const FOBDDDeBruijnIndex*);
	virtual const FOBDDTerm* change(const FOBDDDomainTerm*);
	virtual const FOBDDTerm* change(const FOBDDFuncTerm*);
	virtual const FOBDDTerm* change(const FOBDDAggTerm*);
	virtual const FOBDDQuantSetExpr* change(const FOBDDQuantSetExpr*);
	virtual const FOBDDEnumSetExpr* change(const FOBDDEnumSetExpr*);
};
