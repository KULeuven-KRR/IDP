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
	if (not domain->approxFinite() or domain->empty()) {
		_underivable = true;
		return;
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
	if(_underivable){
		return;
	}
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
	if (Vocabulary::std()->contains(function) && cancalculate) {
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
		case STDFUNC::ABS: {
			_minimum = createDomElem(0);
			auto absmin = (*functable)[_subtermminimums[_level]];
			auto absmax = (*functable)[_subtermminimums[_level]];
			_maximum = *absmin > *absmax? absmin:absmax;
			//IMPORTANT! do not use std::max on the pointers, else pointer arithmetic will be used instead of the domainelement compare
			break;
		}
		case STDFUNC::UNARYMINUS:
			_minimum = (*functable)[_subtermmaximums[_level]];
			_maximum = (*functable)[_subtermminimums[_level]];
			break;
		case STDFUNC::PRODUCT: {
			//It is possible that one of the elements is negative. Hence, we should consider all possible combinations.
			auto min0 = _subtermminimums[_level][0];
			auto min1 = _subtermminimums[_level][1];
			auto max0 = _subtermmaximums[_level][0];
			auto max1 = _subtermmaximums[_level][1];
			auto minmin = (*functable)[ElementTuple { min0, min1 }];
			auto minmax = (*functable)[ElementTuple { min0, max1 }];
			auto maxmin = (*functable)[ElementTuple { max0, min1 }];
			auto maxmax = (*functable)[ElementTuple { max0, max1 }];
			if(minmin!=NULL && minmax!=NULL && maxmin!=NULL && maxmax!=NULL){
				auto allpossibilities = std::vector<DomainElement> { *minmin, *minmax, *maxmin, *maxmax};
				auto minimumDE = *(std::min_element(allpossibilities.cbegin(), allpossibilities.cend()));
				auto maximumDE = *(std::max_element(allpossibilities.cbegin(), allpossibilities.cend()));
				if(minimumDE.type() == DomainElementType::DET_INT){
					_minimum = createDomElem(minimumDE.value()._int);
				} else {
					Assert(minimumDE.type() == DomainElementType::DET_DOUBLE);
					_minimum = createDomElem(minimumDE.value()._double);
				}
				if (maximumDE.type() == DomainElementType::DET_INT) {
					_maximum = createDomElem(maximumDE.value()._int);
				} else {
					Assert(maximumDE.type() == DomainElementType::DET_DOUBLE);
					_maximum = createDomElem(maximumDE.value()._double);
				}
			}else{
				_underivable = true;
				return;
			}
			break;
		}
		case STDFUNC::MAXELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			if (domain->empty()) {
				_underivable = true;
				return;
			}
			_maximum = domain->last();
			_minimum = domain->last();
			break;
		}
		case STDFUNC::MINELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			if (domain->empty()) {
				_underivable = true;
				return;
			}
			_maximum = domain->first();
			_minimum = domain->first();
			break;
		}
		case STDFUNC::MODULO:
			_maximum = createDomElem(0);
			_minimum = _subtermmaximums[_level][1];
			break;
		case STDFUNC::DIVISION: // TODO handle
			_underivable = true;
			return;
		case STDFUNC::EXPONENTIAL: // TODO handle
			_underivable = true;
			return;
		case STDFUNC::SUCCESSOR: // TODO handle
			_underivable = true;
			return;
		case STDFUNC::PREDECESSOR: // TODO handle
			_underivable = true;
			return;
		}
	} else {
		Assert(t->sort() != NULL);
		auto domain = _structure->inter(t->sort());
		Assert(domain != NULL);
		if (not domain->approxFinite() or domain->empty()) { //FIXME: Never return NULL... return infinity thingies.
			_underivable = true;
			return;
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


	const DomainElement* neutral;
	if (t->function() == AggFunction::PROD) {
		neutral = createDomElem(1);
	} else {
		neutral  = createDomElem(0);
	}

	bool start = true;
	auto currentmin = neutral;
	auto currentmax = neutral;

	// Derive bounds of subterms of the set
	for (auto i = t->set()->getSets().cbegin(); i < t->set()->getSets().cend(); ++i) {
		_level++;
		(*i)->getSubTerm()->accept(this);
		if(_underivable){
			return;
		}
		// TODO might optimize the value if the condition is already known
		_level--;

		auto maxsize = (*i)->maxSize(_structure); // minsize always 1
		if (maxsize._type != TST_EXACT) {
			_underivable = true;
			return;
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
		case AggFunction::PROD: {
			Assert(neutral == createDomElem(1));
			//If we have a set with maxsize 4, and maximum 10, we want the max to be 10^4
			//In case negative values appear, we should incorporate them, i.e. reason on the absolute value. Therefor, we take the biggest element of
			// |maximum|, neutral and |minimum|. However, this is the same as the biggest of maximum, neutral and -minimum
			// (since neutral is one and maximum >= minimum, hence -minimum>=-maximum)
			auto absmax = max( { _maximum, neutral, domElemUmin(_minimum) }, Compare<DomainElement>());
			//Now, this value can occur maximum maxsizeElem times hence take the power of absmax with maxsizeElem
			auto maxOfThisSet = domElemPow(absmax, maxsizeElem);
			currentmax = domElemProd(currentmax, maxOfThisSet);
			//In case all values are positive, currentmin is zero Otherwise, the minimum of a product is minus the maximum:
			auto minOfThisSet = isPositive(_minimum) ? createDomElem(0) : domElemUmin(maxOfThisSet);
			//Also multiply current with this new currentmin. BUT be careful with negative values!
			//If both values are negative, watch out for not making currentmin positive!
			if (isNegative(currentmin) && isNegative(minOfThisSet)) {
				currentmin = domElemProd(currentmin, domElemUmin(minOfThisSet));
			} else {
				currentmin = domElemProd(currentmin, minOfThisSet);
			}
			break;
		}
		case AggFunction::MIN:
		case AggFunction::MAX:
			if (start) {
				currentmin = _minimum;
			} else {
				currentmin = min(_minimum, currentmin, Compare<DomainElement>());
			}
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

	// FIXME change MAX and MIN occurrences such that empty sets are handled externally, such that the solver does not need infinite just to handle empty sets.
/*	if (t->function() == AggFunction::MIN) {
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
	}*/
}
