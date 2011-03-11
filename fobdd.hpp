/************************************
	fobdd.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FOBDD
#define FOBDD

/************
	Terms
************/

class FOBDDArgument {
};

class FOBDDVariable : public FOBDDArgument {
	private:
		Variable*	_variable;
	
		FOBDDVariable(Variable* var) :
			_variable(var) { }
	
	public:

	friend FOBDDVariable* FOBDDManager::addVariable(Variable* var);
};

class FOBDDDeBruijnIndex : public FOBDDArgument {

};


/**************
	Kernels
**************/

class FOBDDKernel {
};

class FOBDDAtomKernel : public FOBDDKernel {

	private:
		PFSymbol*				_symbol;
		vector<FOBDDArgument*>	_args;

		FOBDDAtomKernel(PFSymbol* symbol, const vector<FOBDDArgument*>& args) :
			_symbol(symbol), _args(args) { }

	public:
	
	friend FOBDDAtomKernel*	FOBDDManager::addAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args);
};

class FOBDDQuantKernel : public FOBDDKernel {
	private:
		Sort*	_sort;	
		FOBDD*	_bdd;

		FOBDDQuantKernel(Sort* sort, FOBDD* bdd) :
			_sort(sort), _bdd(bdd) { }
	
	public:

	friend FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort, FOBDD* bdd);
};

/***********
	BDDs
***********/

class FOBDD {

	private:
		FOBDDKernel*	_kernel;
		FOBDD*			_falsebranch;
		FOBDD*			_truebranch;

		FOBDD(FOBDDKernel* kernel, FOBDD* falsebranch, FOBDD* truebranch) :
			_kernel(kernel), _falsebranch(falsebranch), _truebranch(truebranch) { }

	public:

	friend FOBDD* FOBDDManager::addBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
};

/******************
	BDD manager
******************/

typedef map<FOBDD*,FOBDD*>				MBDDBDD;				
typedef map<FOBDD*,MBDDMBDD>			MBDDMBDDBDD;	
typedef map<FOBDDKernel*,MBDDMBDDBDD>	BDDTable;

typedef map<vector<FOBDDArgument*>,FOBDDAtomKernel*>	MVAGAK;
typedef map<PFSymbol*,MVAGAK>							AtomKernelTable;
typedef map<FOBDD*,FOBDDQuantKernel*>					MBDDQK;
typedef map<Sort*,MBDDQK>								QuantKernelTable;

class FOBDDManager {

	private:
		BDDTable			_bddtable;
		AtomKernelTable		_atomkerneltable;
		QuantKernelTable	_quantkerneltable;

		FOBDD*				addBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	addAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	addQuantKernel(Sort* sort, FOBDD* bdd);

	public:

		FOBDD*				getBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	getAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	getQuantKernel(Sort* sort, FOBDD* bdd);
		
};

#endif
