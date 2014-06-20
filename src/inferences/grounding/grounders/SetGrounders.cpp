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

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(LitGrounder* sublitgrounder, const TermGrounder& subtermgrounder, litlist& literals, weightlist& weights, weightlist& trueweights,
		InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = sublitgrounder->translator()->trueLit();
	} else {
		auto lgr = LazyGroundingRequest({});
		l = sublitgrounder->groundAndReturnLit(lgr);
	}
	if (l == sublitgrounder->translator()->falseLit()) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();
	Assert(not groundweight.isVariable);
	const auto& d = groundweight._domelement;
	if (d == NULL) {
		return;
	}
	auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);

	if (l == sublitgrounder->translator()->trueLit()) {
		trueweights.push_back(w);
	} else {
		weights.push_back(w);
		literals.push_back(l);
	}
}

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(LitGrounder* sublitgrounder, const TermGrounder& subtermgrounder, weightlist& trueweights, litlist& conditions, termlist& cpterms,
		InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = sublitgrounder->translator()->trueLit();
	} else {
		auto lgr = LazyGroundingRequest({});
		l = sublitgrounder->groundAndReturnLit(lgr);
	}
	if (l == sublitgrounder->translator()->falseLit()) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();

	if (l == sublitgrounder->translator()->trueLit() && not groundweight.isVariable) {
		const auto& d = groundweight._domelement;
		if(d==NULL){
			return;
		}
		auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);
		trueweights.push_back(w);
		return;
	}
	conditions.push_back(l);
	cpterms.push_back(groundweight);
}

EnumSetGrounder::EnumSetGrounder(std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, const std::vector<QuantSetGrounder*>& subgrounders)
		: SetGrounder(freevarcontainers, gt), _set(NULL), _subgrounders(subgrounders) {
	std::vector<QuantSetExpr*> exprs;
	for(auto sg: subgrounders){
		addAll(_varmap, sg->getVarmapping());
		exprs.push_back(sg->getSet()->cloneKeepVars());
	}
	_set = new EnumSetExpr(exprs,{});
}

EnumSetGrounder::~EnumSetGrounder() {
	deleteList(_subgrounders);
	_set->recursiveDelete();
}

SetId EnumSetGrounder::run() {
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
	for (auto grounder : _subgrounders) {
		grounder->run(literals, weights, trueweights);
	}
	return _translator->translateSet(id, tuple, literals, weights, trueweights, { });
}

SetId EnumSetGrounder::runAndRewriteUnknowns() {
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
	for (auto grounder : _subgrounders) {
		grounder->run(trueweights, conditions, cpterms);
	}
	return _translator->translateSet(id, tuple, conditions, { }, trueweights, cpterms);
}

QuantSetGrounder::QuantSetGrounder(QuantSetExpr* expr, std::vector<const DomElemContainer*> freevarcontainers, GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, InstChecker* checker, TermGrounder* w)
		: SetGrounder(freevarcontainers, gt), _set(expr), _subgrounder(gr), _generator(ig), _checker(checker), _weightgrounder(w) {
	addAll(_varmap, gr->getVarmapping());
	addAll(_varmap, w->getVarmapping());
}

QuantSetGrounder::~QuantSetGrounder() {
	_set->recursiveDelete();
	delete _subgrounder;
	delete _generator;
	delete _checker;
	delete _weightgrounder;
}

void QuantSetGrounder::run(litlist& literals, weightlist& weights, weightlist& trueweights) {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(_subgrounder, *_weightgrounder, literals, weights, trueweights, *_checker);
	}
}

void QuantSetGrounder::run(weightlist& trueweights, litlist& conditions, termlist& cpvars) {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(_subgrounder, *_weightgrounder, trueweights, conditions, cpvars, *_checker);
	}
}

SetId QuantSetGrounder::run() {
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

SetId QuantSetGrounder::runAndRewriteUnknowns() {
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
