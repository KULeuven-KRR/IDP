/************************************
	builtin.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUILTIN_H
#define BUILTIN_H

#include "structure.hpp"
#include "vocabulary.hpp"

namespace Builtin {
	
	Sort* intsort();	// integer sort
	Sort* charsort();	// character sort
	Sort* floatsort();	// floating point number sort
	Sort* stringsort();	// string sort

	Sort*				sort(const string& n);	// return builtin sort with name n
	Predicate*			pred(const string& n);	// return builtin predicate with name n
	Function*			func(const string& n);	// return builtin function with name n
	vector<Predicate*>	pred_no_arity(const string& n);	
	vector<Function*>	func_no_arity(const string& n);

	void	initialize();
	void	deleteAll();

	SortTable*	inter(Sort* s);
	PredInter*	inter(Predicate* p,const vector<SortTable*>&);
	FuncInter*	inter(Function* f,const vector<SortTable*>&);

}

#endif
