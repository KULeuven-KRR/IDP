/************************************
	domainiterator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DOMAINITERATOR_HPP_
#define DOMAINITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class DomainIteratorInference: public Inference {
public:
	DomainIteratorInference(): Inference("domainIterator") {
		add(AT_DOMAIN);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		const SortTable* st = args[0]._value._domain;
		SortIterator* it = new SortIterator(st->sortbegin());
		InternalArgument ia; ia._type = AT_DOMAINITERATOR;
		ia._value._sortiterator = it;
		return ia;
	}
};

#endif /* DOMAINITERATOR_HPP_ */
