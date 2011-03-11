/************************************
	fobdd.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "fobdd.hpp"

/******************
	BDD manager
******************/

FOBDDManager::FOBDDManager() {
	// TODO
}

FOBDD* FOBDDManager::getBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch) {
	// Simplification
	// TODO: extra tests (see old grounder)
	if(falsebranch == truebranch) return falsebranch;

	// Lookup
	BDDTable::const_iterator it = _bddtable.find(kernel);
	if(it != _bddtable.end()) {
		MBDDMBDDBDD::const_iterator jt = it->second.find(falsebranch);
		if(jt != it->second.end()) {
			MBDDBDD::const_iterator kt = jt->second.find(truebranch);
			if(kt != jt->second.end()) {
				return kt->second;
			}
		}
	}
	
	// Lookup failed, create a new bdd
	return addBDD(kernel,falsebranch,truebranch);

}

FOBDD* FOBDDManager::addBDD(FOBDDKernel* kernel,FOBDD* falsebranch,FOBDD* truebranch) {
	FOBDD* newbdd = new FOBDD(kernel,falsebranch,truebranch);
	_bddtable[kernel][falsebranch][truebranch] = newbdd;
	return newbdd;
}

FOBDDAtomKernel* FOBDDManager::getAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args) {
	// TODO: simplification

	// Lookup
	AtomKernelTable::const_iterator it = _atomkerneltable.find(symbol);
	if(it != _atomkerneltable.end()) {
		MVAGAK::const_iterator jt = it->second.find(args);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}

	// Lookup failed, create a new atom kernel
	return addAtomKernel(symbol,args);
}

FOBDDAtomKernel* FOBDDManager::addAtomKernel(PFSymbol* symbol,const vector<FOBDDArgument*>& args) {
	FOBDDAtomKernel* newkernel = new FOBDDAtomKernel(symbol,args);
	_atomkerneltable[symbol][args] = newkernel;
	return newkernel;
}

FOBDDQuantKernel* FOBDDManager::getQuantKernel(Sort* sort,FOBDD* bdd) {
	// TODO: simplification

	// Lookup
	QuantKernelTable::const_iterator it = _quantkerneltable.find(sort);
	if(it != _quantkerneltable.end()) {
		MBDDQK::const_iterator jt = it->second.find(bdd);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}

	// Lookup failed, create a new quantified kernel
	return addQuantKernel(sort,bdd);
}

FOBDDQuantKernel* FOBDDManager::addQuantKernel(Sort* sort,FOBDD* bdd) {
	FOBDDQuantKernel* newkernel = new FOBDDQuantKernel(sort,bdd);
	_quantkerneltable[sort][bdd] = newkernel;
	return newkernel;
}

FOBDDVariable* FOBDDManager::getVariable(Variable* var) {
	// Lookup
	VariableTable::const_iterator it = _variabletable.find(var);
	if(it != _variabletable.end()) {
		return it->second;
	}

	// Lookup failed, create a new variable
	return addVariable(var);
}

FOBDDVariable* FOBDDManager::addVariable(Variable* var) {
	FOBDDVariable* newvariable = new FOBDDVariable(var);
	_variabletable[var] = newvariable;
	return newvariable;
}

FOBDDDeBruijnIndex* FOBDDManager::getDeBruijnIndex(Sort* sort, unsigned int index) {
	// Lookup
	DeBruijnIndexTable::const_iterator it = _debruijntable.find(sort);
	if(it != _debruijntable.end()) {
		MUIDB::const_iterator jt = it->second.find(index);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new De Bruijn index
	return addDeBruijnIndex(sort,index);
}

FOBDDDeBruijnIndex* FOBDDManager::addDeBruijnIndex(Sort* sort, unsigned int index) {
	FOBDDDeBruijnIndex* newindex = new FOBDDDeBruijnIndex(sort,index);
	_debruijntable[sort][index] = newindex;
	return newindex;
}

FOBDDFuncTerm* FOBDDManager::getFuncTerm(Function* func, const vector<FOBDDArgument*>& args) {
	// Lookup
	FuncTermTable::const_iterator it = _functermtable.find(sort);
	if(it != _functermtable.end()) {
		MVAFT::const_iterator jt = it->second.find(args);
		if(jt != it->second.end()) {
			return jt->second;
		}
	}
	// Lookup failed, create a new funcion term
	return addFuncTerm(func,args);
}

FOBDDFuncTerm* FOBDDManager::addFuncTerm(Function* func, const vector<FOBDDArgument*>& args) {
	FOBDDFuncTerm* newarg = new FOBDDFuncTerm(func,args);
	_functermtable[func][args] = newarg;
	return newarg;
}
