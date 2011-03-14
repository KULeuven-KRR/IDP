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
	private:
		Sort*			_sort;
		unsigned int	_index;

		FOBDDDeBruijnIndex(Sort* sort, unsigned int index) :
			_sort(sort), _index(index) { }

	public:

	friend FOBDDDeBruijnIndex* FOBDDManager::addDeBruijnIndex(Sort* sort, unsigned int index);
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

typedef map<Variable*,FOBDDVariable*>			VariableTable;
typedef map<unsigned int,FOBDDDeBruijnIndex*>	MUIDB;
typedef map<Sort*,MUIDB>						DeBruijnIndexTable;

class FOBDDManager {

	private:
		// Leaf nodes
		FOBDD*				_trueleaf;
		FOBDD*				_falseleaf;

		// Global tables
		BDDTable			_bddtable;
		AtomKernelTable		_atomkerneltable;
		QuantKernelTable	_quantkerneltable;
		VariableTable		_variabletable;
		DeBruijnIndexTable	_debruijntable;

		FOBDD*				addBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	addAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	addQuantKernel(Sort* sort, FOBDD* bdd);
		FOBDDVariable*		addVariable(Variable* var);
		FOBDDDeBruijnIndex* addDeBruijnIndex(Sort* sort, unsigned int index);

		FOBDDAtomKernel*	getKernel(PredForm* pf);

	public:

		FOBDDManager();

		FOBDD*				getBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch);
		FOBDDAtomKernel*	getAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args);
		FOBDDQuantKernel*	getQuantKernel(Sort* sort, FOBDD* bdd);
		FOBDDVariable*		getVariable(Variable* var);
		FOBDDDeBruijnIndex* getDeBruijnIndex(Sort* sort, unsigned int index);

		FOBDD*				getBDD(PredForm* pf);
		
};

class FOBDDFactory : public Visitor {

	private:
		FOBDDManager*	_manager;
		
		// Return values
		FOBDD*			_bdd;
		FOBDDKernel*	_kernel;
		FOBDDArgument*	_argument;

	public:
		FOBDDFactory(FOBDDManager* m) : _manager(m) { }

		FOBDD*	bdd() const { return _bdd;	}

		void visit(const PredForm* pf);
	
};

#endif
