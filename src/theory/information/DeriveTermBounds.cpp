/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "DeriveTermBounds.hpp"

#include "IncludeComponents.hpp"
#include "structure/NumericOperations.hpp"

#include <numeric> // for accumulate
#include <algorithm> // for min_element and max_element
using namespace std;

void DeriveTermBounds::visit(const DomainTerm* t) {
	_minimum = t->value();
	_maximum = t->value();
}

void DeriveTermBounds::visit(const VarTerm* t) {
	Assert(_structure != NULL && t->sort() != NULL);
	auto domain = _structure->inter(t->sort());
	Assert(domain != NULL);
	if (not domain->approxFinite()) {
		throw BoundsUnderivableException();
	} else {
		_minimum = domain->first();
		_maximum = domain->last();
	}
}

inline STDFUNC operator++(STDFUNC& x) {
	return x = (STDFUNC) (((int) (x) + 1));
}
inline STDFUNC operator*(STDFUNC& x) {
	return x;
}

STDFUNC getStdFunction(Function* function) {
	Assert(function->builtin());
	for (auto i = STDFUNC::FIRST; i != STDFUNC::LAST; ++i) {
		if (is(function, *i)) {
			return *i;
		}
	}
	throw IdpException("Invalid code path"); // NOTE: should not get here as all builtin functions should be covered
}

void DeriveTermBounds::visit(const FuncTerm* t) {
	Assert(_structure != NULL);

	// Derive bounds on subterms
	traverse(t);
	bool cancalculate = true;
	//TODO: is this approach correct? Sometimes min might be calculated when Max = infty;..
	for (auto it = _subtermmaximums[_level].cbegin(); it != _subtermmaximums[_level].cend(); it++) {
		if (*it == NULL) {
			cancalculate = false;
		}
	}
	for (auto it = _subtermminimums[_level].cbegin(); it != _subtermminimums[_level].cend(); it++) {
		if (*it == NULL) {
			cancalculate = false;
		}
	}
	auto function = t->function();
	if (function->builtin() && cancalculate) {
		Assert(function->interpretation(_structure) != NULL);
		auto functable = function->interpretation(_structure)->funcTable();

		switch (getStdFunction(function)) {
		case STDFUNC::ADDITION:
			_minimum = (*functable)[_subtermminimums[_level]];
			_maximum = (*functable)[_subtermmaximums[_level]];
			break;
		case STDFUNC::SUBSTRACTION:
			_minimum = (*functable)[ElementTuple { _subtermminimums[_level][0], _subtermmaximums[_level][1] }];
			_maximum = (*functable)[ElementTuple { _subtermmaximums[_level][0], _subtermminimums[_level][1] }];
			break;
		case STDFUNC::ABS:
			_minimum = createDomElem(0);
			_maximum = std::max((*functable)[_subtermminimums[_level]], (*functable)[_subtermmaximums[_level]]);
			break;
		case STDFUNC::UNARYMINUS:
			_minimum = (*functable)[_subtermmaximums[_level]];
			_maximum = (*functable)[_subtermminimums[_level]];
			break;
		case STDFUNC::PRODUCT: {
			//It is possible that one of the elements is negative. Hence, we should consider all possible combinations.
			auto allpossibilities = ElementTuple { (*functable)[ElementTuple { _subtermminimums[_level][0], _subtermminimums[_level][1] }],
					(*functable)[ElementTuple { _subtermminimums[_level][0], _subtermmaximums[_level][1] }], (*functable)[ElementTuple {
							_subtermmaximums[_level][0], _subtermminimums[_level][1] }], (*functable)[ElementTuple { _subtermmaximums[_level][0],
							_subtermmaximums[_level][1] }] };
			_minimum = *(std::min_element(allpossibilities.cbegin(), allpossibilities.cend()));
			_maximum = *(std::max_element(allpossibilities.cbegin(), allpossibilities.cend()));
			break;
		}
		case STDFUNC::MAXELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			if (domain->empty()) {
				throw BoundsUnderivableException();
			}
			_maximum = domain->last();
			_minimum = domain->last();
			break;
		}
		case STDFUNC::MINELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			if (domain->empty()) {
				throw BoundsUnderivableException();
			}
			_maximum = domain->first();
			_minimum = domain->first();
			break;
		}
		case STDFUNC::MODULO:
			_maximum = createDomElem(0);
			_minimum = _subtermmaximums[_level][1];
			break;
		case STDFUNC::DIVISION:
			throw BoundsUnderivableException(); // TODO
		case STDFUNC::EXPONENTIAL:
			throw BoundsUnderivableException(); // TODO
		case STDFUNC::SUCCESSOR:
			throw BoundsUnderivableException(); // TODO
		case STDFUNC::PREDECESSOR:
			throw BoundsUnderivableException(); // TODO
		}
	} else {
		Assert(t->sort() != NULL);
		auto domain = _structure->inter(t->sort());
		Assert(domain != NULL);
		if (not domain->approxFinite() || domain->empty()) { //FIXME: Never return NULL... return infinity thingies.
			throw BoundsUnderivableException();
		}
		_minimum = domain->first();
		_maximum = domain->last();
	}
}

const DomainElement* sumPositive(const DomainElement* a, const DomainElement* b) {
	auto zero = createDomElem(0);
	return domElemSum(max(a, zero, Compare<DomainElement>()), max(b, zero, Compare<DomainElement>()));
}

const DomainElement* sumNegative(const DomainElement* a, const DomainElement* b) {
	auto zero = createDomElem(0);
	return domElemSum(min(a, zero, Compare<DomainElement>()), min(b, zero, Compare<DomainElement>()));
}

bool absCompare(const DomainElement* a, const DomainElement* b) {
	return *domElemAbs(a) < *domElemAbs(b);
}

void DeriveTermBounds::visit(const AggTerm* t) {
	Assert(_structure != NULL && t->sort() != NULL);

	storeAndClearLists();

	auto neutral = createDomElem(0);
	if (t->function() == AggFunction::PROD) {
		neutral = createDomElem(1);
	}
	bool start = true;
	auto currentmin = neutral;
	auto currentmax = neutral;

	// Derive bounds of subterms of the set
	for (auto i = t->set()->getSets().cbegin(); i < t->set()->getSets().cend(); ++i) {
		_level++;
		(*i)->getSubTerm()->accept(this);
		// TODO might optimize the value if the condition is already known
		_level--;

		auto maxsize = (*i)->maxSize(_structure); // minsize always 1
		if (maxsize._type != TST_EXACT) {
			throw BoundsUnderivableException();
		}
		auto maxsizeElem = createDomElem(maxsize._size, NumType::CERTAINLYINT);

		switch (t->function()) {
		case AggFunction::CARD:
			currentmax = domElemSum(currentmax, maxsizeElem);
			break;
		case AggFunction::SUM:
			currentmax = domElemSum(currentmax, domElemProd(maxsizeElem, max(_maximum, neutral, Compare<DomainElement>())));
			currentmin = domElemSum(currentmin, domElemProd(maxsizeElem, min(_minimum, neutral, Compare<DomainElement>())));
			break;
		case AggFunction::PROD:
			currentmax = domElemProd(currentmax, domElemPow(maxsizeElem, max(_maximum, neutral, Compare<DomainElement>())));
			currentmin = domElemProd(currentmin, domElemPow(maxsizeElem, min(_minimum, neutral, Compare<DomainElement>())));
			break;
		case AggFunction::MIN:
			if (start) {
				currentmin = _minimum;
			} else {
				currentmin = min(_minimum, currentmin, Compare<DomainElement>());
			}
			break;
		case AggFunction::MAX:
			if (start) {
				currentmax = _maximum;
			} else {
				currentmax = max(_maximum, currentmax, Compare<DomainElement>());
			}
			break;
		}
		start = false;
		_minimum = NULL; // For safety
		_maximum = NULL; // For safety
	}

	_maximum = currentmax;
	_minimum = currentmin;

	if (t->function() == AggFunction::MIN) {
		_maximum = NULL;
		if (start) {
			_minimum = NULL;
		}
	}
	if (t->function() == AggFunction::MAX) {
		_minimum = NULL;
		if (start) {
			_maximum = NULL;
		}
	}
}
