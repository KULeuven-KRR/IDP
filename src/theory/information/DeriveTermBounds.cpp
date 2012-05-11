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

void DeriveTermBounds::traverse(const Term* t) {
	ElementTuple minsubterms, maxsubterms;
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		(*it)->accept(this);
		minsubterms.push_back(_minimum);
		maxsubterms.push_back(_maximum);
	}
	_subtermminimums = minsubterms;
	_subtermmaximums = maxsubterms;
}

void DeriveTermBounds::traverse(const SetExpr* e) {
	ElementTuple minsubterms, maxsubterms;
	for (auto it = e->subterms().cbegin(); it != e->subterms().cend(); ++it) {
		(*it)->accept(this);
		minsubterms.push_back(_minimum);
		maxsubterms.push_back(_maximum);
	}
	_subtermminimums = minsubterms;
	_subtermmaximums = maxsubterms;
}

void DeriveTermBounds::visit(const DomainTerm* t) {
	_minimum = t->value();
	_maximum = t->value();
}

void DeriveTermBounds::visit(const VarTerm* t) {
	Assert(_structure != NULL && t->sort() != NULL);
	auto domain = _structure->inter(t->sort());
	Assert(domain != NULL);
	if (not domain->approxFinite()) {
		_minimum = NULL;
		_maximum = NULL;
	} else {
		_minimum = domain->first();
		_maximum = domain->last();
	}
}

void DeriveTermBounds::visit(const FuncTerm* t) {
	Assert(_structure != NULL);

	// Derive bounds on subterms
	traverse(t);
	bool cancalculate = true;
	//TODO: is this approach correct? Sometimes min might be calculated when Max = infty;..
	for (auto it = _subtermmaximums.cbegin(); it != _subtermmaximums.cend(); it++) {
		if (*it == NULL) {
			cancalculate = false;
		}
	}
	for (auto it = _subtermminimums.cbegin(); it != _subtermminimums.cend(); it++) {
		if (*it == NULL) {
			cancalculate = false;
		}
	}
	auto function = t->function();
	if (function->builtin() && cancalculate) {
		Assert(function->interpretation(_structure) != NULL);
		auto functable = function->interpretation(_structure)->funcTable();

		if (is(function, STDFUNC::ADDITION)) {
			_minimum = (*functable)[_subtermminimums];
			_maximum = (*functable)[_subtermmaximums];
		} else if (is(function, STDFUNC::SUBSTRACTION)) {
			_minimum = (*functable)[ElementTuple { _subtermminimums[0], _subtermmaximums[1] }];
			_maximum = (*functable)[ElementTuple { _subtermmaximums[0], _subtermminimums[1] }];
		} else if (is(function, STDFUNC::ABS)) {
			_minimum = createDomElem(0);
			_maximum = std::max((*functable)[_subtermminimums], (*functable)[_subtermmaximums]);
		} else if (is(function, STDFUNC::UNARYMINUS)) {
			_minimum = (*functable)[_subtermmaximums];
			_maximum = (*functable)[_subtermminimums];
		} else if (is(function, STDFUNC::PRODUCT)) {
			//It is possible that one of the elements is negative. Hence, we should consider all possible combinations.
			auto allpossibilities = ElementTuple { (*functable)[ElementTuple { _subtermminimums[0], _subtermminimums[1] }], (*functable)[ElementTuple {
					_subtermminimums[0], _subtermmaximums[1] }], (*functable)[ElementTuple { _subtermmaximums[0], _subtermminimums[1] }],
					(*functable)[ElementTuple { _subtermmaximums[0], _subtermmaximums[1] }] };
			_minimum = *(std::min_element(allpossibilities.cbegin(), allpossibilities.cend()));
			_maximum = *(std::max_element(allpossibilities.cbegin(), allpossibilities.cend()));
		} else if (is(function, STDFUNC::MAXELEM)) {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			_maximum = domain->last();
			_minimum = domain->last();

		} else if (is(function, STDFUNC::MINELEM)) {
			auto domain = _structure->inter(t->sort());
			Assert(domain != NULL);
			_maximum = domain->first();
			_minimum = domain->first();
		} else if (is(function, STDFUNC::MODULO)) {
			_maximum = createDomElem(0);
			_minimum = _subtermmaximums[1];
		} else {
			std::stringstream ss;
			ss << "Deriving term bounds for function" << function->name() << ".";
			throw notyetimplemented(ss.str());
			//FIXME: All operations should be covered...
			_minimum = NULL;
			_maximum = NULL;
		}
	} else {
		Assert(t->sort() != NULL);
		auto domain = _structure->inter(t->sort());
		Assert(domain != NULL);
		if (not domain->approxFinite()) { //FIXME: Never return NULL... return infinity thingies.
			_minimum = NULL;
			_maximum = NULL;
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
		_minimum = accumulate(_subtermminimums.cbegin(), _subtermminimums.cend(), zero, sumNegative);
		_maximum = accumulate(_subtermmaximums.cbegin(), _subtermmaximums.cend(), zero, sumPositive);
		if (sametypeid<QuantSetExpr>(*(t->set()))) {
			if (maxsize._type != TST_EXACT) {
				_maximum = NULL; // This means that the upperbound is unknown.
			} else {
				_maximum = domElemProd(maxsizeElem, _maximum);
			}
		} else {
			Assert(sametypeid<EnumSetExpr>(*(t->set())));
		}
		break;
	case AggFunction::PROD:
		if (maxsize._type != TST_EXACT) {
			_minimum = NULL;
			_maximum = NULL;
		} else {
			auto maxsubtermvalue = std::max(domElemAbs(*max_element(_subtermminimums.cbegin(), _subtermminimums.cend(), absCompare)),
					domElemAbs(*max_element(_subtermmaximums.cbegin(), _subtermmaximums.cend(), absCompare)), Compare<DomainElement>());
			_minimum = domElemUmin(domElemPow(maxsubtermvalue, maxsizeElem));
			_maximum = domElemPow(maxsubtermvalue, maxsizeElem);
		}
		break;
	case AggFunction::MIN:
		_minimum = *min_element(_subtermminimums.cbegin(), _subtermminimums.cend());
		_maximum = *min_element(_subtermmaximums.cbegin(), _subtermmaximums.cend());
		break;
	case AggFunction::MAX:
		_minimum = *max_element(_subtermminimums.cbegin(), _subtermminimums.cend());
		_maximum = *max_element(_subtermmaximums.cbegin(), _subtermmaximums.cend());
		break;
	}
}
