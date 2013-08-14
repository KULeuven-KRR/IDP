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

#ifndef KERNELORDER_HPP_
#define KERNELORDER_HPP_

class FOBDDDomainTerm;
class FOBDDFuncTerm;
class FOBDDTerm;
class DomainElement;
class FOBDDManager;
class FOBDDVariable;
class Variable;
struct CompareBDDVars;

#include "common.hpp"
#include "CommonBddTypes.hpp"
#include <utility> // for relational operators (namespace rel_ops)
#include "vocabulary/VarCompare.hpp"
#include "FoBddVariable.hpp"
using namespace std;
using namespace rel_ops;

/**
 *	A kernel order contains two numbers to order kernels (nodes) in a BDD.
 *	Kernels with a higher category appear further from the root than kernels with a lower category
 *	Within a category, kernels are ordered reversed according to the second number.
 *
 *	The reason for this reversed ordering is to avoid too many reorderings
 *	(during several operations (quantify) we create much new
 *	bdds and don't want every of them to be moved to the top of the BDD
 *
 *	kernelAbove and kernelBelow are based on this order!!!
 */
struct KernelOrder {
	KernelOrderCategory _category; //!< The category of this kernel
	unsigned int _number; //!< The second number
	KernelOrder(KernelOrderCategory c, unsigned int n)
			: _category(c), _number(n) {
	}
	KernelOrder(const KernelOrder& order)
			: _category(order._category), _number(order._number) {
	}
	bool operator<(const KernelOrder& ko) const {
		if (_category < ko._category) {
			return true;
		} else if (ko._category < _category) {
			return false;
		} else {
			return _number > ko._number;
		}
	}
};

template<typename Type>
bool isBddDomainTerm(Type value) {
	return isa<FOBDDDomainTerm>(*value);
}

template<typename Type>
bool isBddFuncTerm(Type value) {
	return isa<FOBDDFuncTerm>(*value);
}

template<typename Type>
const FOBDDDomainTerm* castBddDomainTerm(Type term) {
	Assert(isa<const FOBDDDomainTerm>(*term));
	return dynamic_cast<const FOBDDDomainTerm*>(term);
}

template<typename Type>
const FOBDDFuncTerm* castBddFuncTerm(Type term) {
	Assert(isa<const FOBDDFuncTerm>(*term));
	return dynamic_cast<const FOBDDFuncTerm*>(term);
}

template<typename FuncTerm>
bool isAddition(FuncTerm term) {
	return term->func()->name() == "+/2";
}

template<typename FuncTerm>
bool isMultiplication(FuncTerm term) {
	return term->func()->name() == "*/2";
}

struct Addition {
	shared_ptr<FOBDDManager> manager;

	Addition(shared_ptr<FOBDDManager> manager): manager(manager) {

	}

	static std::string getFuncName() {
		return "+/2";
	}

	static const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do!
	bool operator()(const FOBDDTerm* arg1, const FOBDDTerm* arg2);
};

struct Multiplication {
	Multiplication(shared_ptr<FOBDDManager>) {

	}

	static std::string getFuncName() {
		return "*/2";
	}

	static const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do! -> not understood yet!
	bool operator()(const FOBDDTerm* arg1, const FOBDDTerm* arg2);
	static bool before(const FOBDDTerm* arg1, const FOBDDTerm* arg2, std::shared_ptr<FOBDDManager> manager) {
		// TODO MAKE THIS INDEPENDENT OF POINTER ORDERING
		Multiplication m(manager);
		return m(arg1, arg2);
	}
};

struct TermOrder {
	static bool before(const FOBDDTerm* arg1, const FOBDDTerm* arg2, std::shared_ptr<FOBDDManager> manager);
};

template<typename ReturnType, typename T1, typename T2, typename Something, typename ... MoreTypes>
ReturnType* lookup(const std::map<T1, map<T2, Something> >& m, T1 x, T2 y, MoreTypes ... parameters) {
	auto res = m.find(x);
	if (res == m.cend()) {
		return NULL;
	}
	return lookup<ReturnType>(res->second, y, parameters...);
}

template<typename ReturnType, typename T1>
ReturnType* lookup(const std::map<T1, ReturnType*>& m, T1 x) {
	auto res = m.find(x);
	if (res == m.cend()) {
		return NULL;
	}
	return res->second;
}

template<typename FinalType, typename T1, typename T2, typename Something>
void deleteAll(std::map<T1, map<T2, Something> >& m) {
	for (auto i = m.begin(); i != m.end(); ++i) {
		deleteAll<FinalType>((*i).second);
	}
	m.clear();
}

template<typename FinalType, typename T1>
void deleteAll(std::map<T1, FinalType*>& m) {
	for (auto i = m.begin(); i != m.end(); ++i) {
		delete ((*i).second);
	}
	m.clear();
}

template<typename FinalType, typename T1, typename T2, typename Something>
void deleteAllMatching(std::map<T1, map<T2, Something> >& m, T1 k) {
	auto res = m.find(k);
	if (res != m.cend()) {
		deleteAll<FinalType>(res->second);
	}
}

template<typename FinalType, typename T1>
void deleteAllMatching(std::map<T1, FinalType*>& m, T1 k) {
	auto res = m.find(k);
	if (res != m.cend()) {
		delete (res->second);
	}
}

//Returns a the set of FO variables in the given set of FOBDDvariables
varset getFOVariables(const fobddvarset& vars);
#endif /* KERNELORDER_HPP_ */
