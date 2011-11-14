#ifndef FOBDDDOMAINTERM_HPP_
#define FOBDDDOMAINTERM_HPP_

#include "fobdds/FoBddTerm.hpp"

class DomainElement;
class FOBDDManager;

class FOBDDDomainTerm: public FOBDDArgument {
private:
	friend class FOBDDManager;

	Sort* _sort;
	const DomainElement* _value;

	FOBDDDomainTerm(Sort* sort, const DomainElement* value) :
			_sort(sort), _value(value) {
	}

public:
	bool containsDeBruijnIndex(unsigned int) const {
		return false;
	}

	Sort* sort() const {
		return _sort;
	}
	const DomainElement* value() const {
		return _value;
	}

	void accept(FOBDDVisitor*) const;
	const FOBDDArgument* acceptchange(FOBDDVisitor*) const;
};

const FOBDDDomainTerm* add(FOBDDManager* manager, const FOBDDDomainTerm* d1, const FOBDDDomainTerm* d2);

#endif /* FOBDDDOMAINTERM_HPP_ */
