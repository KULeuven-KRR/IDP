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
					(*functable)[ElementTuple { _subtermminimums[_level][0], _subtermmaximums[_level][1] }],
					(*functable)[ElementTuple { _subtermmaximums[_level][0], _subtermminimums[_level][1] }],
					(*functable)[ElementTuple { _subtermmaximums[_level][0], _subtermmaximums[_level][1] }] };
			_minimum = *(std::min_element(allpossibilities.cbegin(), allpossibilities.cend()));
			_maximum = *(std::max_element(allpossibilities.cbegin(), allpossibilities.cend()));
			break;
		}
		case STDFUNC::MAXELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			_maximum = domain->last();
			_minimum = domain->last();
			break;
		}
		case STDFUNC::MINELEM: {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
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
		if (not domain->approxFinite()) { //FIXME: Never return NULL... return infinity thingies.
			throw BoundsUnderivableException();
		} else {
			_minimum = domain->first();
			_maximum = domain->last();
		}
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

	// Derive bounds of subterms of the set (NOTE: do not do this when dealing with CARD)
	if (t->function() != AggFunction::CARD) {
		traverse(t->set());
	}

	auto maxsize = t->set()->maxSize(_structure);
	auto maxsizeElem = createDomElem(maxsize._size, NumType::CERTAINLYINT);
	auto zero = createDomElem(0);

	// TODO in all below, what if subterm contains NULL???

	switch (t->function()) {
	case AggFunction::CARD:
		_minimum = zero;
		if (maxsize._type != TST_EXACT) {
			_maximum = NULL; // This means that the upperbound is unknown.
		} else {
			_maximum = maxsizeElem;
		}
		break;
	case AggFunction::SUM:
		_minimum = accumulate(_subtermminimums[_level].cbegin(), _subtermminimums[_level].cend(), zero, sumNegative);
		_maximum = accumulate(_subtermmaximums[_level].cbegin(), _subtermmaximums[_level].cend(), zero, sumPositive);
		if (isa<QuantSetExpr>(*(t->set()))) {
			if (maxsize._type != TST_EXACT) {
				_maximum = NULL; // This means that the upperbound is unknown.
			} else {
				_maximum = domElemProd(maxsizeElem, _maximum);
			}
		} else {
			Assert(isa<EnumSetExpr>(*(t->set())));
		}
		break;
	case AggFunction::PROD:
		if (maxsize._type != TST_EXACT) {
			throw BoundsUnderivableException();
		} else {
			auto maxsubtermvalue = std::max(domElemAbs(*max_element(_subtermminimums[_level].cbegin(), _subtermminimums[_level].cend(), absCompare)),
					domElemAbs(*max_element(_subtermmaximums[_level].cbegin(), _subtermmaximums[_level].cend(), absCompare)), Compare<DomainElement>());
			_minimum = domElemUmin(domElemPow(maxsubtermvalue, maxsizeElem));
			_maximum = domElemPow(maxsubtermvalue, maxsizeElem);
		}
		break;
	case AggFunction::MIN:
		_minimum = *min_element(_subtermminimums[_level].cbegin(), _subtermminimums[_level].cend());
		_maximum = *min_element(_subtermmaximums[_level].cbegin(), _subtermmaximums[_level].cend());
		break;
	case AggFunction::MAX:
		_minimum = *max_element(_subtermminimums[_level].cbegin(), _subtermminimums[_level].cend());
		_maximum = *max_element(_subtermmaximums[_level].cbegin(), _subtermmaximums[_level].cend());
		break;
	}
}
