/*
 * LazyGroundingManager.hpp
 *
 *  Created on: 27-jul.-2012
 *      Author: Broes
 */

#pragma once

#include "common.hpp"
#include "utils/ListUtils.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"
#include <vector>

struct ContainerAtom {
	PFSymbol* symbol;
	std::vector<SortTable*> tables; // allowed instantiations to fire for
	std::vector<const DomElemContainer*> args;
	bool watchedvalue;

	~ContainerAtom();
};

struct Delay {
	std::set<const DomElemContainer*> query;
	std::vector<ContainerAtom> condition; // Conjunction of atoms

	void put(std::ostream&) const;
};

class DelayedSentence {
public:
	bool done;
	FormulaGrounder* sentence; // No free vars
	std::shared_ptr<Delay> delay;
	std::map<PFSymbol*, std::pair<bool, Formula*> > construction;

	DelayedSentence(FormulaGrounder* sentence, std::shared_ptr<Delay> delay);

	void notifyFired(const LazyGroundingManager& manager, PFSymbol* symbol, const ElementTuple& tuple);

	void put(std::ostream& stream) const;

private:
	struct FireInformation {
		const LazyGroundingManager& manager;
		const std::vector<ContainerAtom>& conjunction;
		uint currentindex;
		const uint firedindex;
		const ElementTuple& tuplefired;
		std::set<const DomElemContainer*> fullinstantiation;
	};
	void recursiveFire(FireInformation& info);
	std::vector<std::pair<Atom, ElementTuple>> singletuple;
	void recursiveFire(const std::vector<std::pair<Atom, ElementTuple>>& tuples, FireInformation& info);
};
class DelayedRule {
private:
	RuleGrounder* _rule;
	std::set<ElementTuple> grounded; // The list of all elements tuples for which the head watch has already fired

	DefId construction;

public:
	DelayedRule(RuleGrounder* rule, DefId definition)
			: 	_rule(rule),
				construction(definition) {
	}

	void notifyHeadFired(const Lit& head, const ElementTuple& tuple, GroundTranslator* translator, std::map<DefId, GroundDefinition*>& tempdefs);

	DefId getConstruction() const {
		return construction;
	}

	Rule* getRule() const;
};

typedef std::vector<DelayedSentence*> sentlist;
typedef std::vector<DelayedRule*> rulelist;

class DelayInitializer;

class StructureExtender {
public:
	virtual ~StructureExtender() {
	}
	virtual std::vector<Definition*> extendStructure(Structure* structure) const = 0;
	virtual void put(std::ostream&) const = 0;
};

class LazyGroundingManager: public Grounder, public StructureExtender {
private:
	const bool _nbModelsEquivalent;
	Vocabulary const * const _outputvocabulary;
	StructureInfo _structures;

	std::vector<Grounder*> groundersRegisteredForDeletion;
	std::vector<SortTable*> tablesToDelete;

	sentlist sentences;
	rulelist rules;

	std::queue<Grounder*> tobeinitialized;
	std::map<FormulaGrounder*, int> fg2definitionid; // only has a mapping if the onlyif part of a completion
	std::queue<std::pair<DefinitionGrounder*, std::set<RuleGrounder*>>>rulegrounderstodelay;
	std::vector<std::pair<FormulaGrounder*, std::shared_ptr<Delay>>> formwithdelaytobeinitialized;
	std::queue<Grounder*> toGround;
	friend class DelayInitializer;

	std::map<PFSymbol*, std::map<bool, rulelist>> symbol2watchedrules;
	std::map<PFSymbol*, std::map<bool, sentlist>> symbol2watchedsentences;

	std::set<Lit> alreadygroundedlits;
	std::map<DelayedSentence*, std::set<Lit>> sent2alreadygroundedlits;
	std::map<DelayedRule*, std::set<Lit>> rule2alreadygroundedlits;
	std::map<PFSymbol*, std::map<bool, std::vector<std::pair<Atom, ElementTuple> >>> symbol2knownlits; // Has become true at least once, ...

	std::set<PFSymbol*> constructedAsDef;// Symbols which are constructed as definition. Can never be used for other watches

protected:
	virtual void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request);

public:
	LazyGroundingManager(AbstractGroundTheory* grounding, const GroundingContext& context, Vocabulary const * const outputvocabulary,
			StructureInfo structures, bool nbModelsEquivalent);
	virtual ~LazyGroundingManager();

	bool getNbModelEquivalent() const {
		return _nbModelsEquivalent;
	}
	StructureInfo getStructureInfo() const {
		return _structures;
	}
	const Vocabulary* getOutputVocabulary() const {
		return _outputvocabulary;
	}

	void notifyNewLiteral(PFSymbol* symbol, const ElementTuple& args, Lit translatedliteral);

	void add(Grounder* grounder);
	void add(FormulaGrounder* grounder, std::shared_ptr<Delay> delay);
	void add(FormulaGrounder* grounder, PredForm* atom, bool watchontrue, int definitionid);// Note: -1 == not defined

	void notifyBecameTrue(const Lit& lit, bool onlyqueue = false);

	bool canBeDelayedOn(PFSymbol* pfs, bool truewatch) const;
	bool canBeDelayedOn(Formula* head, Formula* body) const;

	std::vector<Definition*> extendStructure(Structure* structure) const;

	Grounder* getFirstSubGrounder() const;

	Structure const * getStructure() const {
		return _structures.concrstructure;
	}

	virtual void put(std::ostream&) const;

	void notifyNewVarId(Function *pFunction, const std::vector<GroundTerm>& vector, VarId& id);

private:
	bool split(Grounder* grounder);

	void resolveQueues();

	void addToManager(Grounder* grounder);

	std::map<DefId, GroundDefinition*> tempdefs;
	bool resolvingqueues;
	std::queue<std::pair<Atom, bool>> queuedforgrounding;

	void delay(DefinitionGrounder* dg);
	void delay(DefinitionGrounder* dg, const std::set<RuleGrounder*>& delayable);
	void fireAllKnown(DelayedRule* delrule, bool watchedvalue);

	void needWatch(bool watchedvalue, Lit translatedliteral);
	void checkAddedDelay(PredForm* pf, bool watchedvalue, bool deforequiv); // IMPORTANT: call GroundMore after all relevant delays have been added (necessary to accumulate rules etc)
	std::set<std::pair<const PFSymbol*, bool> > alreadyAddedFromStructures;
	void addKnownToStructures(PredForm* pf, bool watchedvalue);
	std::set<const PFSymbol*> alreadyAddedToOutputVoc;
	void addToOutputVoc(PFSymbol* symbol, bool expensiveConstruction);

	void fired(Atom atom, bool value);
	void specificFire(DelayedRule* rule, Lit lit, const ElementTuple& args);
	void specificFire(DelayedSentence* sentence, Lit lit, PFSymbol* symbol, const ElementTuple& args);

	void notifyForOutputVoc(PFSymbol* symbol, const litlist& literals);

	void delay(FormulaGrounder* grounder, std::shared_ptr<Delay> delay);

	std::vector<std::pair<Atom, ElementTuple> > emptylist;// Slight hack ;-) to allow to return const ref even for the empty list without having it as keys

	friend class DelayedSentence;
	friend class FindDelayPredForms;
	const std::vector<std::pair<Atom, ElementTuple> >& getFiredLits(PFSymbol* symbol, bool v) const {
		if (not contains(symbol2knownlits, symbol)) {
			return emptylist;
		}
		const auto& value2lits = symbol2knownlits.at(symbol);
		if (not contains(value2lits, v)) {
			return emptylist;
		}
		return value2lits.at(v);
	}
};
