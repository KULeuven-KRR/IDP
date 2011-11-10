#ifndef FOBDDQUANTKERNEL_HPP_
#define FOBDDQUANTKERNEL_HPP_

#include "fobdds/FoBddKernel.hpp"

class FOBDDManager;
class Sort;
class FOBDDVisitor;
class FOBDD;

class FOBDDQuantKernel: public FOBDDKernel {
private:
	friend class FOBDDManager;

	Sort* _sort;
	const FOBDD* _bdd;

	FOBDDQuantKernel(Sort* sort, const FOBDD* bdd, const KernelOrder& order) :
			FOBDDKernel(order), _sort(sort), _bdd(bdd) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const;

	Sort* sort() const {
		return _sort;
	}
	const FOBDD* bdd() const {
		return _bdd;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDKernel* acceptchange(FOBDDVisitor*) const;
};

#endif /* FOBDDQUANTKERNEL_HPP_ */
