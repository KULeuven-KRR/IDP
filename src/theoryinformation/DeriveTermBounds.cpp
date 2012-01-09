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

#include "common.hpp"
#include "term.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"

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

	// Derive bounds on subterms of the set
	traverse(t);

	auto function = t->function();
	if (function->builtin()) {
		Assert(function->interpretation(_structure) != NULL);
		auto functable = function->interpretation(_structure)->funcTable();
		if (function->name() == "+/2") {
			_minimum = (*functable)[_subtermminimums];
			_maximum = (*functable)[_subtermmaximums];
		} else if (function->name() == "-/2") {
			_minimum = (*functable)[ElementTuple { _subtermminimums[0], _subtermmaximums[1] }];
			_maximum = (*functable)[ElementTuple { _subtermmaximums[0], _subtermminimums[1] }];
		} else if (function->name() == "abs/1") {
			_minimum = createDomElem(0);
			_maximum = std::max((*functable)[_subtermminimums], (*functable)[_subtermmaximums]);
		} else if (function->name() == "-/1") {
			_minimum = (*functable)[_subtermmaximums];
			_maximum = (*functable)[_subtermminimums];
		} else {
			_minimum = NULL;
			_maximum = NULL;
		}
	} else {
		Assert(t->sort() != NULL);
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
