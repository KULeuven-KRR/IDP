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
#include "theory.hpp"

class Sort;
class PFSymbol;
class Variable;
class DomainElement;

enum AtomKernelType { AKT_CT, AKT_CF, AKT_TWOVAL };

/*******************
	Kernel order
*******************/

/**
 *	A kernel order contains two numbers to order kernels (nodes) in a BDD. 
 *	Kernels with a higher category appear further from the root than kernels with a lower category
 *	Within a category, kernels are ordered according to the second number.
 */
struct KernelOrder {
	unsigned int	_category;	//!< The category
	unsigned int	_number;	//!< The second number
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

typedef std::map<const FOBDD*, FOBDD*>				MBDDBDD;				
typedef std::map<const FOBDD*,MBDDBDD>				MBDDMBDDBDD;	
typedef std::map<const FOBDDKernel*,MBDDMBDDBDD>	BDDTable;

typedef std::map<std::vector<const FOBDDArgument*>, FOBDDAtomKernel*>	MVAGAK;
typedef std::map<AtomKernelType,MVAGAK>									MAKTMVAGAK;
typedef std::map<PFSymbol*,MAKTMVAGAK>									AtomKernelTable;
typedef std::map<const FOBDD*,FOBDDQuantKernel*>						MBDDQK;
typedef std::map<Sort*,MBDDQK>											QuantKernelTable;

typedef std::map<unsigned int, FOBDDKernel*>		MIK;
typedef std::map<unsigned int, MIK>					KernelTable;

typedef std::map<Variable*,FOBDDVariable*>							VariableTable;
typedef std::map<unsigned int,FOBDDDeBruijnIndex*>					MUIDB;
typedef std::map<Sort*,MUIDB>										DeBruijnIndexTable;
typedef std::map<const DomainElement*,FOBDDDomainTerm*>				MTEDT;
typedef std::map<Sort*,MTEDT>										DomainTermTable;
typedef std::map<std::vector<const FOBDDArgument*>,FOBDDFuncTerm*>	MVAFT;
typedef std::map<Function*,MVAFT>									FuncTermTable;

/**
 * Class to create and manage first-order BDDs
 */
class FOBDDManager {
	private:
		// Leaf nodes
		FOBDD*	_truebdd;	//!< the BDD 'true'
		FOBDD*	_falsebdd;	//!< the BDD 'false'

		// Order
		std::map<unsigned int,unsigned int>	_nextorder;

		KernelOrder newOrder(unsigned int category);
		KernelOrder	newOrder(const std::vector<const FOBDDArgument*>& args);
		KernelOrder newOrder(const FOBDD* bdd);

		// Global tables
		BDDTable			_bddtable;
		AtomKernelTable		_atomkerneltable;
		QuantKernelTable	_quantkerneltable;
		VariableTable		_variabletable;
		DeBruijnIndexTable	_debruijntable;
		FuncTermTable		_functermtable;
		DomainTermTable		_domaintermtable;
		KernelTable			_kernels;

		FOBDD*				addBDD(const FOBDDKernel* kernel,const FOBDD* falsebranch,const FOBDD* truebranch);
		FOBDDAtomKernel*	addAtomKernel(PFSymbol* symbol,AtomKernelType akt, const std::vector<const FOBDDArgument*>& args);
		FOBDDQuantKernel*	addQuantKernel(Sort* sort, const FOBDD* bdd);
		FOBDDVariable*		addVariable(Variable* var);
		FOBDDDeBruijnIndex* addDeBruijnIndex(Sort* sort, unsigned int index);
		FOBDDFuncTerm* 		addFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args);
		FOBDDDomainTerm*	addDomainTerm(Sort* sort, const DomainElement* value);

		const FOBDD*				quantify(Sort* sort, const FOBDD* bdd);

		std::set<const FOBDDVariable*>	variables(const FOBDDKernel*);
		std::set<const FOBDDVariable*>	variables(const FOBDD*);
		std::set<const FOBDDDeBruijnIndex*>	indices(const FOBDDKernel*);
		std::set<const FOBDDDeBruijnIndex*>	indices(const FOBDD*);
		std::map<const FOBDDKernel*,double> kernelUnivs(const FOBDD*, AbstractStructure* structure); 


		std::vector<std::vector<std::pair<bool,const FOBDDKernel*> > >	pathsToFalse(const FOBDD* bdd);
		std::set<const FOBDDKernel*>									nonnestedkernels(const FOBDD* bdd);
		std::set<const FOBDDKernel*>									allkernels(const FOBDD* bdd);
		std::map<const FOBDDKernel*,double> kernelAnswers(const FOBDD*, AbstractStructure*);
		double estimatedChance(const FOBDDKernel*, AbstractStructure*);
		double estimatedChance(const FOBDD*, AbstractStructure*);

	public:
		FOBDDManager();

		const FOBDD*		truebdd()	const	{ return _truebdd;	}
		const FOBDD*		falsebdd()	const	{ return _falsebdd;	}
		
		bool				isTruebdd(const FOBDD* bdd)		const	{ return _truebdd == bdd;	}
		bool				isFalsebdd(const FOBDD* bdd)	const	{ return _falsebdd == bdd;	}

		const FOBDD*				getBDD(const FOBDDKernel* kernel,const FOBDD* truebranch,const FOBDD* falsebranch);
		const FOBDDAtomKernel*		getAtomKernel(PFSymbol*,AtomKernelType, const std::vector<const FOBDDArgument*>&);
		const FOBDDQuantKernel*		getQuantKernel(Sort* sort, const FOBDD* bdd);
		const FOBDDVariable*		getVariable(Variable* var);
		const FOBDDDeBruijnIndex*	getDeBruijnIndex(Sort* sort, unsigned int index);
		const FOBDDFuncTerm* 		getFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args);
		const FOBDDDomainTerm*		getDomainTerm(Sort* sort, const DomainElement* value);

		std::set<const FOBDDVariable*>	getVariables(const std::set<Variable*>& vars);

		const FOBDD*		negation(const FOBDD*);
		const FOBDD*		conjunction(const FOBDD*,const FOBDD*);
		const FOBDD*		disjunction(const FOBDD*,const FOBDD*);
		const FOBDD*		univquantify(const FOBDDVariable*,const FOBDD*);
		const FOBDD*		existsquantify(const FOBDDVariable*,const FOBDD*);
		const FOBDD*		univquantify(const std::set<const FOBDDVariable*>&,const FOBDD*);
		const FOBDD*		existsquantify(const std::set<const FOBDDVariable*>&,const FOBDD*);
		const FOBDD*		ifthenelse(const FOBDDKernel*, const FOBDD* truebranch, const FOBDD* falsebranch);
		const FOBDD*		substitute(const FOBDD*,const std::map<const FOBDDVariable*,const FOBDDVariable*>&);
		const FOBDD*		substitute(const FOBDD*, const FOBDDDeBruijnIndex*, const FOBDDVariable*);
		const FOBDDKernel*	substitute(const FOBDDKernel*, const FOBDDDomainTerm*, const FOBDDVariable*);
		
		int	longestbranch(const FOBDDKernel*);
		int	longestbranch(const FOBDD*);

	 	std::ostream&	put(std::ostream&, const FOBDD*,unsigned int spaces = 0) const;
		std::ostream&	put(std::ostream&, const FOBDDKernel*,unsigned int spaces = 0) const;
		std::ostream&	put(std::ostream&, const FOBDDArgument*) const;

		bool contains(const FOBDDKernel*, Variable*);
		bool contains(const FOBDDKernel*, const FOBDDVariable*);
		bool contains(const FOBDD*, const FOBDDVariable*);
		bool contains(const FOBDDArgument*, const FOBDDVariable*);

		double estimatedNrAnswers(const FOBDDKernel*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);
		double estimatedNrAnswers(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);
		double estimatedCostAll(bool, const FOBDDKernel*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);
		double estimatedCostAll(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);

		void moveDown(const FOBDDKernel*);	//!< Swap the given kernel with its successor in the kernelorder
		void moveUp(const FOBDDKernel*);	//!< Swap the given kernel with its predecessor in the kernelorder
		void optimizequery(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);

		const FOBDD* getBDD(const FOBDD* bdd, FOBDDManager*);	//!< Given a bdd and the manager that created the bdd,
																//!< this function returns the same bdd, but created
																//!< by the manager 'this'

		bool isArithmetic(const FOBDDKernel*);		//!< Returns true iff the kernel is an equation or inequality of
													//!< arithmetic terms
		bool isArithmetic(const FOBDDArgument*);	//!< Returns true iff the argument is an arithmetic term
		const FOBDDAtomKernel*	solve(const FOBDDKernel*, const FOBDDVariable*);		
			//!< Try to rewrite the given arithmetic kernel such that the right-hand side is the given variable,
			//!< and such that the given variable does not occur in the left-hand side.
		const FOBDDAtomKernel*	solve(const FOBDDKernel*, const FOBDDDeBruijnIndex*);
			//!< Try to rewrite the given arithmetic kernel such that the right-hand side is the given index,
			//!< and such that the given index does not occur in the left-hand side.

};

/**
 * Class to transform first-order formulas to BDDs
 */
class FOBDDFactory : public TheoryVisitor {
	private:
		FOBDDManager*	_manager;
		Vocabulary*		_vocabulary;
		
		// Return values
		const FOBDD*			_bdd;
		const FOBDDKernel*	_kernel;
		const FOBDDArgument*	_argument;

	public:
		FOBDDFactory(FOBDDManager* m, Vocabulary* v = 0) : _manager(m), _vocabulary(v) { }

		const FOBDD*			bdd()		const { return _bdd;		}
		const FOBDDArgument*	argument()	const { return _argument;	}

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

class FOBDDVisitor;

class FOBDDArgument {
	public:
		virtual bool containsDeBruijnIndex(unsigned int index)	const = 0;
				bool containsFreeDeBruijnIndex()				const { return containsDeBruijnIndex(0);	}

		virtual void					accept(FOBDDVisitor*)		const = 0;
		virtual const FOBDDArgument*	acceptchange(FOBDDVisitor*)	const = 0;

		virtual Sort*					sort()	const = 0;
};

class FOBDDVariable : public FOBDDArgument {
	private:
		Variable*	_variable;
	
		FOBDDVariable(Variable* var) :
			_variable(var) { }
	
	public:
		bool containsDeBruijnIndex(unsigned int)	const { return false;	}

		Variable*	variable()	const { return _variable;			}
		Sort*		sort()		const;

		void					accept(FOBDDVisitor*)		const;
		const FOBDDArgument*	acceptchange(FOBDDVisitor*)	const;

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

		void					accept(FOBDDVisitor*)		const;
		const FOBDDArgument*	acceptchange(FOBDDVisitor*)	const;

	friend class FOBDDManager;
};

class FOBDDDomainTerm : public FOBDDArgument {
	private:
		Sort*					_sort;
		const DomainElement*	_value;

		FOBDDDomainTerm(Sort* sort, const DomainElement* value) :
			_sort(sort), _value(value) { }

	public:
		bool containsDeBruijnIndex(unsigned int)	const { return false;	}

		Sort*					sort()	const { return _sort;	}	
		const DomainElement*	value()	const { return _value;	}

		void					accept(FOBDDVisitor*)		const;
		const FOBDDArgument*	acceptchange(FOBDDVisitor*)	const;

	friend class  FOBDDManager;
};

class FOBDDFuncTerm : public FOBDDArgument {
	private:
		Function*							_function;
		std::vector<const FOBDDArgument*>	_args;

		FOBDDFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args) :
			_function(func), _args(args) { }

	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		Function*				func()						const	{ return _function;				}
		const FOBDDArgument*	args(unsigned int n)		const	{ return _args[n];				}
		const std::vector<const FOBDDArgument*>&	args()	const	{ return _args;					}
		Sort*					sort()						const;

		void					accept(FOBDDVisitor*)		const;
		const FOBDDArgument*	acceptchange(FOBDDVisitor*)	const;

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
				unsigned int number()						const { return _order._number;				}

		void replacenumber(unsigned int n)	{ _order._number = n;	}

		bool operator<(const FOBDDKernel&) const;
		bool operator>(const FOBDDKernel&) const;

		virtual void				accept(FOBDDVisitor*)		const {					}
		virtual const FOBDDKernel*	acceptchange(FOBDDVisitor*)	const { return this;	}
};

class FOBDDAtomKernel : public FOBDDKernel {
	private:
		PFSymbol*							_symbol;
		AtomKernelType						_type;
		std::vector<const FOBDDArgument*>	_args;

		FOBDDAtomKernel(PFSymbol* symbol, AtomKernelType akt, const std::vector<const FOBDDArgument*>& args, const KernelOrder& order) :
			FOBDDKernel(order), _symbol(symbol), _type(akt), _args(args) { }

	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		PFSymbol*				symbol()				const { return _symbol;		}
		AtomKernelType			type()					const { return _type;		}
		const FOBDDArgument*	args(unsigned int n)	const { return _args[n];	}

		const std::vector<const FOBDDArgument*>&	args()	const { return _args;	}

		void				accept(FOBDDVisitor*)		const;
		const FOBDDKernel*	acceptchange(FOBDDVisitor*)	const;

	friend class FOBDDManager;
};

class FOBDDQuantKernel : public FOBDDKernel {
	private:
		Sort*			_sort;	
		const FOBDD*	_bdd;

		FOBDDQuantKernel(Sort* sort, const FOBDD* bdd, const KernelOrder& order) :
			FOBDDKernel(order), _sort(sort), _bdd(bdd) { }
	
	public:
		bool containsDeBruijnIndex(unsigned int index)	const;

		Sort*			sort()	const { return _sort;	}
		const FOBDD*	bdd()	const { return _bdd;	}

		void				accept(FOBDDVisitor*)		const;
		const FOBDDKernel*	acceptchange(FOBDDVisitor*)	const;

	friend class FOBDDManager;
};

/***********
	BDDs
***********/

class FOBDD {
	private:
		const FOBDDKernel*	_kernel;
		const FOBDD*		_truebranch;
		const FOBDD*		_falsebranch;

		void replacefalse(const FOBDD* f)			{ _falsebranch = f;	}
		void replacetrue(const FOBDD* t)			{ _truebranch = t;	}
		void replacekernel(const FOBDDKernel* k)	{ _kernel = k;		}

		FOBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch) :
			_kernel(kernel), _truebranch(truebranch), _falsebranch(falsebranch) { }

	public:
		bool containsFreeDeBruijnIndex()				const { return containsDeBruijnIndex(0);	}
		bool containsDeBruijnIndex(unsigned int index)	const;

		const FOBDDKernel*	kernel()		const { return _kernel;			}
		const FOBDD*		falsebranch()	const { return _falsebranch;	}
		const FOBDD*		truebranch()	const { return _truebranch;		}

	friend class FOBDDManager;
};

/**************
	Visitor
**************/

class FOBDDVisitor {
	protected:
		FOBDDManager*	_manager;
	public:
		FOBDDVisitor(FOBDDManager* manager) : _manager(manager) { }

		virtual void visit(const FOBDD*);
		virtual void visit(const FOBDDAtomKernel*);
		virtual void visit(const FOBDDQuantKernel*);
		virtual void visit(const FOBDDVariable*);
		virtual void visit(const FOBDDDeBruijnIndex*);
		virtual void visit(const FOBDDDomainTerm*);
		virtual void visit(const FOBDDFuncTerm*);

		virtual const FOBDD*			change(const FOBDD*);
		virtual const FOBDDKernel*		change(const FOBDDAtomKernel*);
		virtual const FOBDDKernel*		change(const FOBDDQuantKernel*);
		virtual const FOBDDArgument*	change(const FOBDDVariable*);
		virtual const FOBDDArgument*	change(const FOBDDDeBruijnIndex*);
		virtual const FOBDDArgument*	change(const FOBDDDomainTerm*);
		virtual const FOBDDArgument*	change(const FOBDDFuncTerm*);
};

#endif
