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

#include "SetGrounders.hpp"

#include "IncludeComponents.hpp"
#include "common.hpp"
#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/BasicCheckersAndGenerators.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

SetGrounder::SetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt)
		: id(gt->createNewQuantSetId()), _freevarcontainers(freevarcontainers), _translator(gt) {
}
EnumSetGrounder::EnumSetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, const std::vector<QuantSetGrounder*>& subgrounders)
		: SetGrounder(freevarcontainers, gt), _subgrounders(subgrounders) {
}

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder, litlist& literals, weightlist& weights, weightlist& trueweights,
		InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = _true;
	} else {
		l = sublitgrounder.groundAndReturnLit();
	}
	if (l == _false) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();
	Assert(not groundweight.isVariable);
	const auto& d = groundweight._domelement;
	if (d == NULL) {
		throw notyetimplemented("invalid term (partial function?) in set expression");
	}
	Assert(d != NULL);
	auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);

	if (l == _true) {
		trueweights.push_back(w);
	} else {
		weights.push_back(w);
		literals.push_back(l);
	}
}

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder, weightlist& trueweights, litlist& conditions, termlist& cpterms,
		InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = _true;
	} else {
		l = sublitgrounder.groundAndReturnLit();
	}
	if (l == _false) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();

	if (l == _true && not groundweight.isVariable) {
		const auto& d = groundweight._domelement;
		if(d==NULL){
			throw notyetimplemented("Invalid term in set expression");
		}
		auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);
		trueweights.push_back(w);
		return;
	}
	conditions.push_back(l);
	cpterms.push_back(groundweight);
}

EnumSetGrounder::~EnumSetGrounder() {
	deleteList(_subgrounders);
}

SetId EnumSetGrounder::run() const {
	ElementTuple tuple;
	for(auto container:_freevarcontainers){
		tuple.push_back(container->get());
	}
	auto possset = _translator->getPossibleSet(id, tuple);
	if(_translator->isSet(possset)){
		return possset;
	}
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	for (auto i = _subgrounders.cbegin(); i < _subgrounders.cend(); ++i) {
		(*i)->run(literals, weights, trueweights);
	}
	return _translator->translateSet(id, tuple, literals, weights, trueweights, { });
}

SetId EnumSetGrounder::runAndRewriteUnknowns() const {
	ElementTuple tuple;
	for(auto container:_freevarcontainers){
		tuple.push_back(container->get());
	}
	auto possset = _translator->getPossibleSet(id, tuple);
	if(_translator->isSet(possset)){
		return possset;
	}
	weightlist trueweights;
	termlist cpterms;
	litlist conditions;
	for (auto i = _subgrounders.cbegin(); i < _subgrounders.cend(); ++i) {
		(*i)->run(trueweights, conditions, cpterms);
	}
	return _translator->translateSet(id, tuple, conditions, { }, trueweights, cpterms);
}

QuantSetGrounder::QuantSetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, InstChecker* checker, TermGrounder* w)
		: SetGrounder(freevarcontainers, gt), _subgrounder(gr), _generator(ig), _checker(checker), _weightgrounder(w) {
}

QuantSetGrounder::~QuantSetGrounder() {
	delete _subgrounder;
	delete _generator;
	delete _checker;
	delete _weightgrounder;
}

void QuantSetGrounder::run(litlist& literals, weightlist& weights, weightlist& trueweights) const {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, *_checker);
	}
}

void QuantSetGrounder::run(weightlist& trueweights, litlist& conditions, termlist& cpvars) const {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, trueweights, conditions, cpvars, *_checker);
	}
}

SetId QuantSetGrounder::run() const {
	ElementTuple tuple;
	for(auto container:_freevarcontainers){
		tuple.push_back(container->get());
	}
	auto possset = _translator->getPossibleSet(id, tuple);
	if(_translator->isSet(possset)){
		return possset;
	}
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	run(literals, weights, trueweights);
	return _translator->translateSet(id, tuple, literals, weights, trueweights, { });
}

SetId QuantSetGrounder::runAndRewriteUnknowns() const {
	ElementTuple tuple;
	for(auto container:_freevarcontainers){
		tuple.push_back(container->get());
	}
	auto possset = _translator->getPossibleSet(id, tuple);
	if(_translator->isSet(possset)){
		return possset;
	}
	weightlist trueweights;
	termlist cpterms;
	litlist conditions;
	run(trueweights, conditions, cpterms);
	return _translator->translateSet(id, tuple, conditions, { }, trueweights, cpterms);
}
