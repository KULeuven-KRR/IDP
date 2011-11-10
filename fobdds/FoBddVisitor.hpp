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
class FOBDDArgument;
class FOBDDKernel;

class FOBDDVisitor {
protected:
	FOBDDManager* _manager;
public:
	FOBDDVisitor(FOBDDManager* manager) :
			_manager(manager) {
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
	virtual const FOBDDArgument* change(const FOBDDVariable*);
	virtual const FOBDDArgument* change(const FOBDDDeBruijnIndex*);
	virtual const FOBDDArgument* change(const FOBDDDomainTerm*);
	virtual const FOBDDArgument* change(const FOBDDFuncTerm*);
};

#endif /* FOBDDVISITOR_HPP_ */