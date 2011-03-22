/************************************
	fobdd.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FOBDD_HPP
#define FOBDD_HPP

#include <vector>
#include <map>
#include <string>

#include "vocabulary.hpp" //FIXME: we need this include for struct TypedElement
#include "visitor.hpp"

class PFSymbol;
class Variable;

/*******************
	Kernel order
*******************/

struct KernelOrder {
	unsigned int	_category;
	unsigned int	_number;
	KernelOrder(unsigned int c, unsigned int n) : _category(c), _number(n) { }
	KernelOrder(const KernelOrder& order) : _category(order._category), _number(order._number) { }
};

/******************
	BDD manager
******************/

class FOBDDArgument;
class FOBDDVariable;
class FOBDDFuncTerm;
class FOBDDDeBruijnIndex;
class FOBDDDomainTerm;

class FOBDDKernel;
class FOBDDAtomKernel;
class FOBDDQuantKernel;
class FOBDD;

typedef std::map<FOBDD*,FOBDD*>				MBDDBDD;				
typedef std::map<FOBDD*,MBDDBDD>			MBDDMBDDBDD;	
typedef std::map<FOBDDKernel*,MBDDMBDDBDD>	BDDTable;

typedef std::map<std::vector<FOBDDArgument*>,FOBDDAtomKernel*>	MVAGAK;
typedef std::map<PFSymbol*,MVAGAK>								AtomKernelTable;
typedef std::map<FOBDD*,FOBDDQuantKernel*>						MBDDQK;
typedef std::map<Sort*,MBDDQK>									QuantKernelTable;

typedef std::map<Variable*,FOBDDVariable*>						VariableTable;
typedef std::map<unsigned int,FOBDDDeBruijnIndex*>				MUIDB;
typedef std::map<Sort*,MUIDB>									DeBruijnIndexTable;
typedef std::map<TypedElement,FOBDDDomainTerm*>					MTEDT;
typedef std::map<Sort*,MTEDT>									DomainTermTable;
typedef std::map<std::vector<FOBDDArgument*>,FOBDDFuncTerm*>	MVAFT;
typedef std::map<Function*,MVAFT>								FuncTermTable;

class FOBDDManager {
	private:
		// Leaf nodes
		FOBDD*	_truebdd;
		FOBDD*	_falsebdd;

		// Order
		std::map<unsigned int,unsigned int>	_nextorder;

		KernelOrder newOrder(unsigned int category);
		KernelOrder	newOrder(const std::vector<FOBDDArgument*>& args);
		KernelOrder newOrder(FOBDD* bdd);

		// Global tables
		BDDTable			_bddtable;
		AtomKernelTable		_atomkerneltable;
		QuantKernelTable	_quantkerneltable;
		VariableTable		_variabletable;
		DeBruijnIndexTable	_debruijntable;
		FuncTermTable		_functermtable;
		DomainTermTable		_domaintermtable;

		FOBDD*				addBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	addAtomKernel(PFSymbol* symbol,const std::vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	addQuantKernel(Sort* sort, FOBDD* bdd);
		FOBDDVariable*		addVariable(Variable* var);
		FOBDDDeBruijnIndex* addDeBruijnIndex(Sort* sort, unsigned int index);
		FOBDDFuncTerm* 		addFuncTerm(Function* func, const std::vector<FOBDDArgument*>& args);
		FOBDDDomainTerm*	addDomainTerm(Sort* sort, TypedElement value);

		FOBDD*				quantify(Sort* sort, FOBDD* bdd);
		FOBDD*				bump(FOBDDVariable* var, FOBDD* bdd, unsigned int depth = 0);
		FOBDDKernel*		bump(FOBDDVariable* var, FOBDDKernel* kernel, unsigned int depth);
		FOBDDArgument*		bump(FOBDDVariable* var, FOBDDArgument* arg, unsigned int depth);

	public:
		FOBDDManager();

		FOBDD*				truebdd()	const	{ return _truebdd;	}
		FOBDD*				falsebdd()	const	{ return _falsebdd;	}

		FOBDD*				getBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	getAtomKernel(PFSymbol* symbol,const std::vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	getQuantKernel(Sort* sort, FOBDD* bdd);
		FOBDDVariable*		getVariable(Variable* var);
		FOBDDDeBruijnIndex* getDeBruijnIndex(Sort* sort, unsigned int index);
		FOBDDFuncTerm* 		getFuncTerm(Function* func, const std::vector<FOBDDArgument*>& args);
		FOBDDDomainTerm*	getDomainTerm(Sort* sort, TypedElement value);

		std::vector<FOBDDVariable*>	getVariables(const std::vector<Variable*>& vars);

		FOBDD*	negation(FOBDD*);
		FOBDD*	conjunction(FOBDD*,FOBDD*);
		FOBDD*	disjunction(FOBDD*,FOBDD*);
		FOBDD*	univquantify(FOBDDVariable*,FOBDD*);
		FOBDD*	existsquantify(FOBDDVariable*,FOBDD*);
		FOBDD*	univquantify(const std::vector<FOBDDVariable*>&,FOBDD*);
		FOBDD*	existsquantify(const std::vector<FOBDDVariable*>&,FOBDD*);
		FOBDD*	ifthenelse(FOBDDKernel*, FOBDD* truebranch, FOBDD* falsebranch);
		FOBDD*	substitute(FOBDD*,const std::map<FOBDDVariable*,FOBDDVariable*>&);
		
		FOBDDKernel*	substitute(FOBDDKernel*,const std::map<FOBDDVariable*,FOBDDVariable*>&);
		FOBDDArgument*	substitute(FOBDDArgument*,const std::map<FOBDDVariable*,FOBDDVariable*>&);

		std::string	to_string(FOBDD*,unsigned int spaces = 0) const;
		std::string	to_string(FOBDDKernel*,unsigned int spaces = 0) const;
		std::string	to_string(FOBDDArgument*) const;

};

class FOBDDFactory : public Visitor {
	private:
		FOBDDManager*	_manager;
		Vocabulary*		_vocabulary;
		
		// Return values
		FOBDD*			_bdd;
		FOBDDKernel*	_kernel;
		FOBDDArgument*	_argument;

	public:
		FOBDDFactory(FOBDDManager* m, Vocabulary* v = 0) : _manager(m), _vocabulary(v) { }

		FOBDD*	bdd() const { return _bdd;	}

		void	visit(const VarTerm* vt);
		void	visit(const DomainTerm* dt);
		void	visit(const FuncTerm* ft);
		void	visit(const AggTerm* at);

		void	visit(const PredForm* pf);
		void	visit(const BoolForm* bf);
		void	visit(const QuantForm* qf);
		void	visit(const EqChainForm* ef);
		void	visit(const AggForm* af);
	
};


/************
	Terms
************/

class FOBDDArgument {
	public:
		virtual bool containsDeBruijnIndex(unsigned int index)	const = 0;
				bool containsFreeDeBruijnIndex()				const { return containsDeBruijnIndex(0);	}
};

class FOBDDVariable : public FOBDDArgument {
	private:
		Variable*	_variable;
	
		FOBDDVariable(Variable* var) :
			_variable(var) { }
	
	public:
		bool containsDeBruijnIndex(unsigned int)	const { return false;	}

		Variable*	variable()	const { return _variable;	}

	friend class FOBDDManager;
};

class FOBDDDeBruijnIndex : public FOBDDArgument {
	private:
		Sort*			_sort;
		unsigned int	_index;

		FOBDDDeBruijnIndex(Sort* sort, unsigned int index) :
			_sort(sort), _index(index) { }

	public:
		bool containsDeBruijnIndex(unsigned int index)	const { return _index == index;	}

		Sort*			sort()	const { return _sort;	}
		unsigned int	index()	const { return _index;	}

	friend class FOBDDManager;
};

class FOBDDDomainTerm : public FOBDDArgument {
	private:
		Sort*			_sort;
		TypedElement	_value;

		FOBDDDomainTerm(Sort* sort, TypedElement value) :
			_sort(sort), _value(value) { }

	public:
		bool containsDeBruijnIndex(unsigned int)	const { return false;	}

		Sort*			sort()	const { return _sort;	}	
		TypedElement	value()	const { return _value;	}

	friend class  FOBDDManager;
};

class FOBDDFuncTerm : public FOBDDArgument {
	private:
		Function*					_function;
		std::vector<FOBDDArgument*>	_args;

		FOBDDFuncTerm(Function* func, const std::vector<FOBDDArgument*>& args) :
			_function(func), _args(args) { }

	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		Function*		func()					const	{ return _function;		}
		FOBDDArgument*	args(unsigned int n)	const	{ return _args[n];		}

	friend class FOBDDManager;
};

/**************
	Kernels
**************/

class FOBDDKernel {
	private:
		KernelOrder	_order;
	public:
		FOBDDKernel(const KernelOrder& order) : _order(order) { }

				bool containsFreeDeBruijnIndex()			const { return containsDeBruijnIndex(0);	}
		virtual bool containsDeBruijnIndex(unsigned int)	const { return false;						}
				unsigned int category()						const { return _order._category;			}

		bool operator<(const FOBDDKernel&) const;
		bool operator>(const FOBDDKernel&) const;
};

class FOBDDAtomKernel : public FOBDDKernel {
	private:
		PFSymbol*					_symbol;
		std::vector<FOBDDArgument*>	_args;

		FOBDDAtomKernel(PFSymbol* symbol, const std::vector<FOBDDArgument*>& args, const KernelOrder& order) :
			FOBDDKernel(order), _symbol(symbol), _args(args) { }

	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		PFSymbol*		symbol()				const { return _symbol;		}
		FOBDDArgument*	args(unsigned int n)	const { return _args[n];	}

	friend class FOBDDManager;
};

class FOBDDQuantKernel : public FOBDDKernel {
	private:
		Sort*	_sort;	
		FOBDD*	_bdd;

		FOBDDQuantKernel(Sort* sort, FOBDD* bdd, const KernelOrder& order) :
			FOBDDKernel(order), _sort(sort), _bdd(bdd) { }
	
	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		Sort*	sort()	const { return _sort;	}
		FOBDD*	bdd()	const { return _bdd;	}

	friend class FOBDDManager;
};

/***********
	BDDs
***********/

class FOBDD {
	private:
		FOBDDKernel*	_kernel;
		FOBDD*			_truebranch;
		FOBDD*			_falsebranch;

		FOBDD(FOBDDKernel* kernel, FOBDD* truebranch, FOBDD* falsebranch) :
			_kernel(kernel), _truebranch(truebranch), _falsebranch(falsebranch) { }

	public:
		bool containsFreeDeBruijnIndex()				const { return containsDeBruijnIndex(0);	}
		bool containsDeBruijnIndex(unsigned int index)	const;

		FOBDDKernel*	kernel()		const { return _kernel;			}
		FOBDD*			falsebranch()	const { return _falsebranch;	}
		FOBDD*			truebranch()	const { return _truebranch;		}

	friend class FOBDDManager;
};

#endif
