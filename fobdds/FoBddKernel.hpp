#ifndef FOBDDKERNEL_HPP_
#define FOBDDKERNEL_HPP_

#include "fobdds/FoBddUtils.hpp"

class FOBDDVisitor;

class FOBDDKernel {
private:
	KernelOrder _order;
public:
	FOBDDKernel(const KernelOrder& order) :
			_order(order) {
	}
	virtual ~FOBDDKernel() {
	}

	bool containsFreeDeBruijnIndex() const {
		return containsDeBruijnIndex(0);
	}
	virtual bool containsDeBruijnIndex(unsigned int) const {
		return false;
	}
	unsigned int category() const {
		return _order._category;
	}
	unsigned int number() const {
		return _order._number;
	}

	void replacenumber(unsigned int n) {
		_order._number = n;
	}

	bool operator<(const FOBDDKernel& k) const {
		if (_order._category < k._order._category) {
			return true;
		} else if (_order._category > k._order._category) {
			return false;
		} else {
			return _order._number < k._order._number;
		}
	}

	bool operator>(const FOBDDKernel& k) const {
		if (_order._category > k._order._category) {
			return true;
		} else if (_order._category < k._order._category) {
			return false;
		} else {
			return _order._number > k._order._number;
		}
	}

	virtual void accept(FOBDDVisitor*) const {
	}
	virtual const FOBDDKernel* acceptchange(FOBDDVisitor*) const {
		return this;
	}
};

#endif /* FOBDDKERNEL_HPP_ */