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

#ifndef OCCCOUNTER54_HPP_
#define OCCCOUNTER54_HPP_

#include "fobdds/FoBddVisitor.hpp"
/**
 * Counts occurences of bdd's, kernels,...
 */
class CountOccurences: public FOBDDVisitor {
private:
	std::map<const FOBDD*, int> _bddcount;
	std::map<const FOBDDKernel*, int> _kernelcount;

public:
	CountOccurences(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	/*
	 * Counts occurences of all sub-BDDs of bdd. Adds this to the previous count.
	 * Use reset to count only this bdd.
	 */
	void count(const FOBDD* bdd) {
		visit(bdd);
	}
	void reset() {
		_bddcount.clear();
		_kernelcount.clear();
	}
	int getCount(const FOBDD* bdd) {
		auto found = _bddcount.find(bdd);
		if (found == _bddcount.cend()) {
			return 0;
		}
		return (*found).second;
	}
	int getCount(const FOBDDKernel* kernel) {
		auto found = _kernelcount.find(kernel);
		if (found == _kernelcount.cend()) {
			return 0;
		}
		return (*found).second;
	}

private:
	void visit(const FOBDD* bdd) {
		auto negatedbdd = _manager->negation(bdd);
		if (_bddcount.find(bdd) != _bddcount.cend()) {
			_bddcount[bdd]++;
			//std::cerr << "occurs more than once:"<<print(bdd)<<nt();
		} else {
			_bddcount[bdd] = 1;
		}
		if (_bddcount.find(negatedbdd) != _bddcount.cend()) {
			_bddcount[negatedbdd]++;
			//std::cerr << "occurs more than once (neg):"<<print(bdd)<<nt();
		} else {
			_bddcount[negatedbdd] = 1;
		}
		FOBDDVisitor::visit(bdd);
	}
	void visit(const FOBDDAtomKernel* k) {
		addKernel(k);
		FOBDDVisitor::visit(k);
	}
	void visit(const FOBDDQuantKernel* k) {
		addKernel(k);
		FOBDDVisitor::visit(k);

	}
	void visit(const FOBDDAggKernel* k) {
		addKernel(k);
		FOBDDVisitor::visit(k);
	}
	void addKernel(const FOBDDKernel* k) {
		if (_kernelcount.find(k) != _kernelcount.cend()) {
			_kernelcount[k]++;
		} else {
			_kernelcount[k] = 1;
		}
	}

};

#endif /* OCCCOUNTER54_HPP_ */
