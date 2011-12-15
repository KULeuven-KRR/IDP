/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef KERNELORDER_HPP_
#define KERNELORDER_HPP_

class FOBDDDomainTerm;
class FOBDDFuncTerm;
class FOBDDArgument;
class DomainElement;
class FOBDDManager;

#include "common.hpp"

enum AtomKernelType {
	AKT_CT, AKT_CF, AKT_TWOVALUED
};

/**
 *	A kernel order contains two numbers to order kernels (nodes) in a BDD.
 *	Kernels with a higher category appear further from the root than kernels with a lower category
 *	Within a category, kernels are ordered according to the second number.
 */
struct KernelOrder {
	unsigned int _category; //!< The unsigned int
	unsigned int _number; //!< The second number
	KernelOrder(unsigned int c, unsigned int n) :
			_category(c), _number(n) {
	}
	KernelOrder(const KernelOrder& order) :
			_category(order._category), _number(order._number) {
	}
};

template<typename Type>
bool isBddDomainTerm(Type value) {
	return sametypeid<FOBDDDomainTerm>(*value);
}

template<typename Type>
bool isBddFuncTerm(Type value) {
	return sametypeid<FOBDDFuncTerm>(*value);
}

template<typename Type>
const FOBDDDomainTerm* getBddDomainTerm(Type term) {
	return dynamic_cast<const FOBDDDomainTerm*>(term);
}

template<typename Type>
const FOBDDFuncTerm* getBddFuncTerm(Type term) {
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
	static std::string getFuncName() {
		return "+/2";
	}

	static const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do!
	bool operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2);
};

struct Multiplication {
	static std::string getFuncName() {
		return "*/2";
	}

	static const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do!
	bool operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2);
	static bool before(const FOBDDArgument* arg1, const FOBDDArgument* arg2) {
		Multiplication m;
		return m(arg1, arg2);
	}
};

struct TermOrder {
	static bool before(const FOBDDArgument* arg1, const FOBDDArgument* arg2, FOBDDManager* manager);
};

#endif /* KERNELORDER_HPP_ */
