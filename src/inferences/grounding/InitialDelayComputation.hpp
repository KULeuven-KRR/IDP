#pragma once

#include "IncludeComponents.hpp"

class RuleGrounder;
class FormulaGrounder;
class DefinitionGrounder;
class LazyGroundingManager;
struct ContainerAtom;
class Grounder;
struct Delay;


class DelayInitializer {
private:
	int maxid;
	std::map<PFSymbol*, uint> symbol2id;
	std::map<uint, PFSymbol*> id2symbol;

	std::map<RuleGrounder*, DefinitionGrounder*> rule2def;

	std::vector<DefinitionGrounder*> defgrounders;

	std::vector<FormulaGrounder*> grounders; // id is index, for each index, grounders[i] or rulegrounders[i] is NULL, the other one is the associated grounder
	std::vector<RuleGrounder*> rulegrounders;

	std::vector<ContainerAtom> containeratoms; // id is index

	std::vector<Grounder*> toGround;

	Vocabulary* voc;
	AbstractTheory* theory;
	Term* minimterm;
	Structure* structure;
	const DomainElement *truedm, *falsedm;
	FuncInter *symbol, *groundsize;
	PredInter *candelayon, *isdefdelay, *isequivalence;
	SortTable *constraint, *noninfcost, *predform, *symbolsort;

	LazyGroundingManager* manager;

public:
	DelayInitializer(LazyGroundingManager* manager);

	~DelayInitializer();

	void addGrounder(Grounder* grounder);

	void addGrounder(FormulaGrounder* grounder, std::shared_ptr<Delay> delay);

	void findDelays();

private:
	void addGroundSize(Grounder* grounder, const DomainElement* id);

	void add(const DomainElement* cid, std::shared_ptr<Delay> delay, int defid, bool equivalence = false);  // defid == -1 is not defined
};
