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

#include "Invariants.hpp"
#include "common.hpp"
#include "theory/theory.hpp"
#include "theory/TheoryUtils.hpp"
#include "data/LTCData.hpp"
#include "LTCTheorySplitter.hpp"
#include "projectLTCStructure.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/entailment/Entails.hpp"
#include "data/SplitLTCTheory.hpp"
#include "structure/Structure.hpp"

bool ProveInvariantInference::proveInvariant(const AbstractTheory* ltcTheo, const AbstractTheory* invar, const Structure* struc) {
	auto ltc = dynamic_cast<const Theory*>(ltcTheo);
	auto inv = dynamic_cast<const Theory*>(invar);
	if(inv == NULL || ltc == NULL){
		Error::error("Can only perform invariant proving on theories");
	}
	auto p = ProveInvariantInference(ltc, inv, struc);
	return p.run();

}
ProveInvariantInference::ProveInvariantInference(const Theory* ltcTheo, const Theory* invar, const Structure* struc)
		: 	_ltcTheo(ltcTheo),
			_invariant(invar),
			_structure(struc) {

}
ProveInvariantInference::~ProveInvariantInference() {

}

bool ProveInvariantInference::run() {
	if (_ltcTheo->vocabulary() != _invariant->vocabulary()) {
		Error::LTC::invarVocIsNotTheoVoc();
	}
	if ((_structure != NULL) && _ltcTheo->vocabulary() != _structure->vocabulary()) {
		Error::LTC::strucVocIsNotTheoVoc();
	}
	auto data = LTCData::instance();
	try {
		//Try transforming the vocabulary without info on Time, Start Next
		//This succeeds if it has been transformed before, or if the system can correctly find time etctera itself.
		data->getStateVocInfo(_ltcTheo->vocabulary());
	} catch (IdpException& e) {
		Warning::warning(
				"Could not automatically split the LTC vocabulary. If you want to set Time, Start and Next function manually, please use the initialise inference first. ");
		throw(e);
	}
	auto splitTheo = data->getSplitTheory(_ltcTheo);
	auto splitInvariant = LTCTheorySplitter::SplitInvariant(_invariant);
#ifdef DEBUG
	if (splitInvariant->invartype == InvarType::SingleStateInvar) {
		Assert(splitInvariant->baseStep != NULL);
		Assert(splitInvariant->inductionStep);
	}
	else {
		Assert(splitInvariant->invartype == InvarType::BistateInvar);
		Assert(splitInvariant->bistateInvar);
	}
#endif

	if (_structure != NULL) {
		auto initstructure = LTCStructureProjector::projectStructure(_structure);
		auto transstructure = LTCStructureProjector::projectStructure(_structure, true);
		if (splitInvariant->invartype == InvarType::SingleStateInvar) {
			return checkImplied(splitTheo->initialTheory, splitInvariant->baseStep, initstructure, true)
					&& checkImplied(splitTheo->bistateTheory, splitInvariant->inductionStep, transstructure, false);
		} else {
			return checkImplied(splitTheo->bistateTheory, splitInvariant->bistateInvar, transstructure, false);
		}
	} else {
		if (splitInvariant->invartype == InvarType::SingleStateInvar) {
			return checkImplied(splitTheo->initialTheory, splitInvariant->baseStep, true)
					&& checkImplied(splitTheo->bistateTheory, splitInvariant->inductionStep, false);
		} else {
			return checkImplied(splitTheo->bistateTheory, splitInvariant->bistateInvar, false);
		}
	}
}

bool ProveInvariantInference::checkImplied(const Theory* hypothesis, Formula* implication, Structure* context, bool initial){
	auto implicationNegated = implication;
	implicationNegated->negate();

	auto completeTheo = hypothesis->clone();
	completeTheo->add(implicationNegated);

	auto backupNbModels = getOption(IntType::NBMODELS);
	setOption(IntType::NBMODELS, 1);

	context->changeVocabulary(completeTheo->vocabulary());
	FormulaUtils::pushNegations(completeTheo);
	FormulaUtils::flatten(completeTheo);
	auto models = ModelExpansion::doModelExpansion(completeTheo, context);
	delete (context);
	bool result = true;
	if (not models.unsat) {
		std::clog << "Found counterexample for invariant in the ";
		if (initial) {
			std::clog << "initial";
		} else {
			std::clog << "induction";
		}
		std::clog << " step:\n";
		auto model = *(models._models.begin());
		std::clog << toString(model);
		delete (model);
		result = false;
	}
	setOption(IntType::NBMODELS, backupNbModels);
	completeTheo->recursiveDelete();
	return result;
}

bool ProveInvariantInference::checkImplied(const Theory* hypothesis, Formula* conjecture, bool initial) {
	auto conj = new Theory("conjectures", hypothesis->vocabulary(), ParseInfo());
	conj->add(conjecture);
	auto temphypo = hypothesis->clone();
	auto result = Entails::doCheckEntailment(temphypo, conj);
	conj->recursiveDelete();
	temphypo->recursiveDelete();

	if(result == State::PROVEN){
		return true;
	} else if(result == State::UNKNOWN){
		Warning::warning("Could not prove invariant, but could not disprove it either");
		return false;
	} else {
		Assert(result == State::DISPROVEN);
		std::clog << "Prover disproved the ";
		if (initial) {
			std::clog << "initial";
		} else {
			std::clog << "induction";
		}
		std::clog << " step of invariant checking.\n";
		return false;
	}
}
