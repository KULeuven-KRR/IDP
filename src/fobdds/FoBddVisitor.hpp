/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FOBDDVISITOR_HPP_
#define FOBDDVISITOR_HPP_

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

class FOBDDVisitor {
protected:
	FOBDDManager* _manager;
public:
	FOBDDVisitor(FOBDDManager* manager)
			: _manager(manager) {
	}
	virtual ~FOBDDVisitor() {
	}

	virtual void visit(const FOBDD*);
	virtual void visit(const FOBDDAtomKernel*);
	virtual void visit(const FOBDDQuantKernel*);
	virtual void visit(const FOBDDVariable*);
	virtual void visit(const FOBDDDeBruijnIndex*);
	virtual void visit(const FOBDDDomainTerm*);
	virtual void visit(const FOBDDFuncTerm*);

	virtual const FOBDD* change(const FOBDD*);
	virtual const FOBDDKernel* change(const FOBDDAtomKernel*);
	virtual const FOBDDKernel* change(const FOBDDQuantKernel*);
	virtual const FOBDDTerm* change(const FOBDDVariable*);
	virtual const FOBDDTerm* change(const FOBDDDeBruijnIndex*);
	virtual const FOBDDTerm* change(const FOBDDDomainTerm*);
	virtual const FOBDDTerm* change(const FOBDDFuncTerm*);
};

#endif /* FOBDDVISITOR_HPP_ */
