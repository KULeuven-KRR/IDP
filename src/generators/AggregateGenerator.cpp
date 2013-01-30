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

#include "AggregateGenerator.hpp"
#include "common.hpp"
#include "structure/NumericOperations.hpp"
#include "structure/MainStructureComponents.hpp"

AggGenerator::AggGenerator(const DomElemContainer* left, AggFunction func, std::vector<InstGenerator*> formulagenerators,
		std::vector<InstGenerator*> termgenerators, std::vector<const DomElemContainer*> terms)
		: _left(left), _func(func), _formulagenerators(formulagenerators), _termgenerators(termgenerators), _terms(terms), _result(0), _reset(true) {
	Assert(left!=NULL);
	Assert(_formulagenerators.size()==_termgenerators.size());
	Assert(_termgenerators.size() == _terms.size());
}

//Executes the next on the ith formulagenerator,...
//Returns false if no value is possible (if this one determines the generator to be at end).
bool AggGenerator::next(unsigned int i) {
	bool goOn = true;
	for (_formulagenerators[i]->begin(); not _formulagenerators[i]->isAtEnd() && goOn; _formulagenerators[i]->operator ++()) {
		_termgenerators[i]->begin();
		if (_termgenerators[i]->isAtEnd()) {
			//TODO: what with partial functions?
			continue;
		}
		auto domelem = (*_terms[i]).get();
		if (domelem->type() == DomainElementType::DET_INT) {
			goOn = doOperation(domelem->value()._int);
		} else {
			Assert(domelem->type() == DomainElementType::DET_DOUBLE);
			goOn = doOperation(domelem->value()._double);
		}
	}
	return goOn;
}
double AggGenerator::getEmptySetValue() {
	switch (_func) {
	case AggFunction::CARD:
	case AggFunction::SUM:
		return 0;
	case AggFunction::PROD:
		return 1;
	case AggFunction::MAX:
		return getMinElem<double>();
	case AggFunction::MIN:
		return getMaxElem<double>();
	}
	Assert(false);
	return 42;
}

bool AggGenerator::setValue(const DomainElement* d) {
	if (d == NULL) {
		notifyAtEnd();
		return false;
	}
	if (d->type() == DomainElementType::DET_INT) {
		_result = d->value()._int;
	} else {
		Assert(d->type() == DomainElementType::DET_DOUBLE);
		_result = d->value()._double;
	}
	return true;
}

bool AggGenerator::doOperation(double d) {
	switch (_func) {
	case AggFunction::CARD:
		return setValue(sum<int>(_result, 1));
	case AggFunction::SUM:
		return setValue(sum<double>(_result, d));
	case AggFunction::PROD:
		return setValue(product<double>(_result, d));
	case AggFunction::MAX:
		_result = _result > d ? _result : d;
		return true;
	case AggFunction::MIN:
		_result = _result < d ? _result : d;
		return true;
	}
	Assert(false);
	return true;
}

AggGenerator* AggGenerator::clone() const {
	throw notyetimplemented("Cloning AggGenerators.");
}

AggGenerator::~AggGenerator() {
	for (auto it = _formulagenerators.cbegin(); it != _formulagenerators.cend(); it++) {
		delete (*it);
	}
	for (auto it = _termgenerators.cbegin(); it != _termgenerators.cend(); it++) {
		delete (*it);
	}
}

void AggGenerator::next() {
	if (not _reset) {
		notifyAtEnd();
	}
	_result = getEmptySetValue();
	_reset = false;
	//goOn could be simplified by using the atEnd method.  However, this conflicts with initDone...
	bool goOn = true;
	for (unsigned int i = 0; i < _formulagenerators.size() && goOn; i++) {
		goOn = next(i);
	}
	if (goOn) {
		_left->operator =(createDomElem(_result));
	} else {
		//Think about the meaning of this. We want maxdouble? or mindouble in this case?
		throw notyetimplemented("Overflows in aggregategenerators");
		Assert(false);
	}

}

void AggGenerator::put(std::ostream& stream) const {
	stream << "Generator for an aggregate (printing not yet implemented)";
}
