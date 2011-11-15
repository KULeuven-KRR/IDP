#ifndef FOBDDINDEX_HPP_
#define FOBDDINDEX_HPP_

#include "fobdds/FoBddTerm.hpp"

class FOBDDManager;

class FOBDDDeBruijnIndex: public FOBDDArgument {
private:
	friend class FOBDDManager;

	Sort* _sort;
	unsigned int _index;

	FOBDDDeBruijnIndex(Sort* sort, unsigned int index) :
			_sort(sort), _index(index) {
	}

public:
	bool containsDeBruijnIndex(unsigned int index) const {
		return _index == index;
	}

	Sort* sort() const {
		return _sort;
	}
	unsigned int index() const {
		return _index;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDArgument* acceptchange(FOBDDVisitor*) const;
};


#endif /* FOBDDINDEX_HPP_ */
