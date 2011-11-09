#ifndef KERNELORDER_HPP_
#define KERNELORDER_HPP_

class FOBDDDomainTerm;
class FOBDDFuncTerm;
class FOBDDArgument;
class DomainElement;

#include "common.hpp"

enum AtomKernelType {
	AKT_CT, AKT_CF, AKT_TWOVAL
};

/**
 *	A kernel order contains two numbers to order kernels (nodes) in a BDD.
 *	Kernels with a higher category appear further from the root than kernels with a lower category
 *	Within a category, kernels are ordered according to the second number.
 */
struct KernelOrder {
	unsigned int _category; //!< The category
	unsigned int _number; //!< The second number
	KernelOrder(unsigned int c, unsigned int n) :
			_category(c), _number(n) {
	}
	KernelOrder(const KernelOrder& order) :
			_category(order._category), _number(order._number) {
	}
};

template<typename Type>
bool isBddDomainTerm(Type value){
	return sametypeid<FOBDDDomainTerm>(*value);
}

template<typename Type>
bool isBddFuncTerm(Type value){
	return sametypeid<FOBDDFuncTerm>(*value);
}

template<typename Type>
const FOBDDDomainTerm* getBddDomainTerm(Type term){
	return dynamic_cast<const FOBDDDomainTerm*>(term);
}

template<typename Type>
const FOBDDFuncTerm* getBddFuncTerm(Type term){
	return dynamic_cast<const FOBDDFuncTerm*>(term);
}

template<typename FuncTerm>
bool isAddition(FuncTerm term){
	return term->func()->name() == "+/2";
}

template<typename FuncTerm>
bool isMultiplication(FuncTerm term){
	return term->func()->name() == "*/2";
}


struct Addition {
	static std::string getFuncName(){
		return "+/2";
	}

	const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do!
	bool operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2);
};

struct Multiplication {
	static std::string getFuncName(){
		return "*/2";
	}

	const DomainElement* getNeutralElement();

	// Ordering method: true if ordered before
	// TODO comment and check what they do!
	bool operator()(const FOBDDArgument* arg1, const FOBDDArgument* arg2);
};

// TODO used?
/*bool TermSWOrdering(const FOBDDArgument* arg1, const FOBDDArgument* arg2,FOBDDManager* manager) {
 FlatMult fa(manager);
 std::vector<const FOBDDArgument*> flat1 = fa.run(arg1);
 std::vector<const FOBDDArgument*> flat2 = fa.run(arg2);
 if(flat1.size() < flat2.size()) { return true; }
 else if(flat1.size() > flat2.size()) { return false; }
 else {
 for(size_t n = 1; n < flat1.size(); ++n) {
 if(FactorSWOrdering(flat1[n],flat2[n])) { return true; }
 else if(FactorSWOrdering(flat2[n],flat1[n])) { return false; }
 }
 return false;
 }
 }*/

#endif /* KERNELORDER_HPP_ */
