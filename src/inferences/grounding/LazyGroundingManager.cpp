/*
 * LazyGroundingManager.cpp
 *
 *  Created on: 27-jul.-2012
 *      Author: Broes
 */

#include "LazyGroundingManager.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/querying/Query.hpp"
#include "theory/Query.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "utils/LogAction.hpp"
#include "errorhandling/UnsatException.hpp"

extern void parsefile(const std::string&);

#warning remove twovaluedness constraint again except for definition and equivalence heads (and in future also not for those)
#warning bug in equivalence.idp, where the initial delay query for P does NOT result in P(1,350), but the approximation DID find it out!

using namespace std;

template<class List>
bool isNonEmptyFor(const List& list, bool value) {
	auto it = list.find(value);
	return it != list.cend() && not it->second.empty();
}

template<class List, class ListElem>
bool hasNonEmpty(const List& list, const ListElem& elem) {
	auto it = list.find(elem);
	return it != list.cend() && it->second.size() > 0;
}

template<class List, class ListIt>
bool exists(const List& list, const ListIt& iterator) {
	return list.cend() != iterator;
}

void Delay::put(std::ostream& stream) const {
	bool begin = true;
	for (auto conjunct : condition) {
		if (not begin) {
			stream << " and ";
		}
		begin = false;
		stream << (conjunct.watchedvalue ? "" : "~") << toString(conjunct.symbol);
	}
}

const Rule& DelayedRule::getRule() const {
	return _rule->getRule();
}

DelayedSentence::DelayedSentence(FormulaGrounder* sentence, std::shared_ptr<Delay> delay)
		: 	done(false),
			sentence(sentence),
			delay(delay),
			 singletuple({ { -1, {} } }){
	for (auto conjunct : delay->condition) {
		// TODO Create construction definition
	}
}

LazyGroundingManager::LazyGroundingManager(AbstractGroundTheory* grounding, const GroundingContext& context, Vocabulary const * const outputvocabulary,
		StructureInfo structures, bool nbModelsEquivalent)
		: 	Grounder(grounding, context),
			_nbModelsEquivalent(nbModelsEquivalent),
			_outputvocabulary(outputvocabulary),
			_structures(structures),
			resolvingqueues(false) {
	getTranslator()->addMonitor(this); // NOTE: request notifications of literal additions
	notifyForOutputVoc(NULL, { });
	setMaxGroundSize(tablesize(TST_EXACT, 1));
}

LazyGroundingManager::~LazyGroundingManager() {
	deleteList<Grounder>(groundersRegisteredForDeletion);
}

// TODO handle functions / equalities added to the translator

bool LazyGroundingManager::split(Grounder* grounder) {
	if (isa<TheoryGrounder>(*grounder)) { // Split up sentences as much as possible.
		auto tg = dynamic_cast<TheoryGrounder*>(grounder);
		for (auto g : tg->getSubGrounders()) {
			add(g);
		}
		return true;
	}
	if (isa<BoolGrounder>(*grounder)) { // Split up sentences as much as possible.
		auto bg = dynamic_cast<BoolGrounder*>(grounder);
		if (bg->conjunctiveWithSign()) {
			for (auto g : bg->getSubGrounders()) {
				add(g);
			}
			return true;
		}
	}
	return false;
}

void LazyGroundingManager::add(Grounder* grounder) {
	auto donesplit = split(grounder);
	if (donesplit) {
		return;
	}
	tobeinitialized.push(grounder);
	setMaxGroundSize(getMaxGroundSize() + grounder->getMaxGroundSize());
}

void LazyGroundingManager::add(FormulaGrounder* grounder, shared_ptr<Delay> d) {
	formwithdelaytobeinitialized.push_back( { grounder, d });
	setMaxGroundSize(getMaxGroundSize() + grounder->getMaxGroundSize());
}

void LazyGroundingManager::add(FormulaGrounder* grounder, PredForm* atom, bool watchontrue, int definitionid) { // TODO construction for this is the definition!!!
	setMaxGroundSize(getMaxGroundSize() + grounder->getMaxGroundSize());

	if (not getOption(SATISFIABILITYDELAY)) {
		toGround.push(grounder);
		return;
	}
	if (getOption(CPSUPPORT) && atom->symbol()->isFunction()) { // TODO combine cp with lazy grounding
		toGround.push(grounder);
		return;
	}

	auto d = shared_ptr<Delay>(new Delay());
	ContainerAtom elem;
	elem.symbol = atom->symbol();
	if (isa<QuantGrounder>(*grounder)) {
		const auto& containers = dynamic_cast<QuantGrounder*>(grounder)->getGeneratedContainers();
		for (auto arg : atom->args()) {
			elem.args.push_back(NULL);
			elem.tables.push_back(NULL);
			Assert(arg->type()==TermType::VAR);
			auto var = dynamic_cast<VarTerm*>(arg)->var();
			auto mapping = grounder->getVarmapping().at(var);
			if (contains(containers, mapping)) {
				elem.args[elem.args.size() - 1] = mapping;
				elem.tables[elem.args.size() - 1] = new SortTable(getStructure()->inter(var->sort())->internTable());
			}
		}
	}
	elem.watchedvalue = watchontrue;
	d->condition.push_back(elem);
	formwithdelaytobeinitialized.push_back( { grounder, d });
	fg2definitionid[grounder] = definitionid;
}

struct DelGrounder {
	Grounder* grounder;
	uint size;
	Delay possibledelays;
};

int getSymbolID(PFSymbol* symbol, int& maxid, std::map<PFSymbol*, uint>& symbol2id, std::map<uint, PFSymbol*>& id2symbol) {
	int id = 0;
	auto it = symbol2id.find(symbol);
	if (it == symbol2id.cend()) {
		maxid++;
		id = maxid;
		symbol2id[symbol] = id;
		id2symbol[id] = symbol;
	} else {
		id = it->second;
	}
	return id;
}

PredForm* findEquivPredDelay(Formula* formula) {
	auto tempformula = formula;
	while (isa<QuantForm>(*tempformula)) {
		auto qf = dynamic_cast<QuantForm*>(tempformula);
		if (not qf->isUnivWithSign()) {
			break;
		}
		tempformula = qf->subformula();
	}
	auto equiv = dynamic_cast<EquivForm*>(tempformula);
	if (equiv == NULL) {
		return NULL;
	}
	auto delaypossible = dynamic_cast<PredForm*>(equiv->left());
	if (delaypossible != NULL && not FormulaUtils::containsSymbol(delaypossible->symbol(), equiv->right())) { // TODO even better, check only for negative loops
		return delaypossible;
	}
	delaypossible = dynamic_cast<PredForm*>(equiv->right());
	if (delaypossible != NULL && not FormulaUtils::containsSymbol(delaypossible->symbol(), equiv->left())) { // TODO even better, check only for negative loops
		return delaypossible;
	}
	return NULL;
}

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
	PredInter *symbol, *candelayon, *groundsize, *isdefdelay, *isequivalence;

	LazyGroundingManager* manager;

	Function *symbolFunc, *groundsizeFunc;

public:
	DelayInitializer(LazyGroundingManager* manager)
			: 	manager(manager),
				maxid(1) {
		if (not getGlobal()->instance()->alreadyParsed("delay_optimization")) {
			auto warnings = getOption(BoolType::SHOWWARNINGS);
			setOption(BoolType::SHOWWARNINGS, false);
			parsefile("delay_optimization");
			setOption(BoolType::SHOWWARNINGS, warnings);
		}

		auto ns = getGlobal()->getGlobalNamespace();
		voc = ns->vocabulary("Delay_Voc");
		theory = ns->theory("Delay_Theory");
		minimterm = ns->term("Delay_Minimization_Term");
		structure = dynamic_cast<Structure*>(ns->structure("Delay_Basic_Data"))->clone();
		Assert(theory!=NULL && minimterm!=NULL && structure!=NULL);

		symbolFunc = voc->func("symbol/1");
		Assert(symbolFunc!=NULL);
		auto candelayonPred = voc->pred("canDelayOn/3");
		Assert(candelayonPred!=NULL);
		groundsizeFunc = voc->func("groundSize/1");
		Assert(groundsizeFunc!=NULL);
		auto isdefdelayPred = voc->pred("isDefinitionDelay/3");
		Assert(isdefdelayPred!=NULL);
		auto isequivalencePred = voc->pred("isEquivalence/1");
		Assert(isequivalencePred!=NULL);
		symbol = structure->inter(symbolFunc)->graphInter();
		symbol = symbol->clone(symbol->universe());
		candelayon = structure->inter(candelayonPred);
		groundsize = structure->inter(groundsizeFunc)->graphInter();
		groundsize = groundsize->clone(groundsize->universe());
		isdefdelay = structure->inter(isdefdelayPred);
		isequivalence = structure->inter(isequivalencePred);
		truedm = createDomElem(new string("True"));
		falsedm = createDomElem(new string("False"));
	}

	void addGrounder(Grounder* grounder) {
		cerr <<"Adding " <<print(grounder) <<"\n";
		if (grounder->hasFormula() && isa<FormulaGrounder>(*grounder)) {
			auto fg = dynamic_cast<FormulaGrounder*>(grounder);
			auto delay = FormulaUtils::findDelay(fg->getFormula(), fg->getVarmapping(), manager);
			if (delay.get() == NULL) {
				// TODO equivalence delaying currently disabled, should first fix that no loops can go over them, how they are constructed (and outputvoc!) etc
				/*auto equivpreddelay = findEquivPredDelay(fg->getFormula());
				 if(equivpreddelay!=NULL){
				 auto delay = make_shared<Delay>();
				 ContainerAtom containeratom;
				 containeratom.symbol = equivpreddelay->symbol();
				 containeratom.watchedvalue = true;
				 for(auto arg:equivpreddelay->args()){ // FIXME assuming these are the only quantified variabeles before the equiv!
				 auto varterm = dynamic_cast<VarTerm*>(arg);
				 Assert(varterm!=NULL);
				 auto container = fg->getVarmapping().at(varterm->var());
				 delay->query.insert(container);
				 containeratom.args.push_back(container);
				 containeratom.tables.push_back(manager->getStructure()->inter(varterm->var()->sort()));
				 }
				 delay->condition.push_back(containeratom);
				 containeratom.watchedvalue=false;
				 delay->condition.push_back(containeratom);
				 grounders.push_back(fg);
				 rulegrounders.push_back(NULL);
				 auto id = createDomElem((int) grounders.size() - 1);
				 addGroundSize(fg, id);
				 add(id, delay, -1,true);
				 }else{*/
				toGround.push_back(grounder);
				//}
			} else {
				grounders.push_back(fg);
				rulegrounders.push_back(NULL);
				auto id = createDomElem((int) grounders.size() - 1);
				addGroundSize(fg, id);
				add(id, delay, -1);
			}
		} else if (isa<DefinitionGrounder>(*grounder)) {
			auto defg = dynamic_cast<DefinitionGrounder*>(grounder);
			defgrounders.push_back(defg);
			for (auto rg : defg->getSubGrounders()) {
				rulegrounders.push_back(rg);
				rule2def[rg] = defg;
				grounders.push_back(NULL);
				auto id = createDomElem((int) grounders.size() - 1);
				addGroundSize(grounder, id);
				auto delay = make_shared<Delay>();
				ContainerAtom atom;
				atom.symbol = rg->getHead()->symbol();
				atom.watchedvalue = true;
				delay->condition.push_back(atom);
				add(id, delay, defg->getDefinitionID().id);
			}
		} else {
			toGround.push_back(grounder);
		}
	}

	void addGrounder(FormulaGrounder* grounder, shared_ptr<Delay> delay) {
		Assert(delay.get()!=NULL);
		grounders.push_back(grounder);
		rulegrounders.push_back(NULL);
		auto id = createDomElem((int) grounders.size() - 1);
		addGroundSize(grounder, id);
		auto defid = -1;
		if (contains(manager->fg2definitionid, grounder)) {
			defid = manager->fg2definitionid.at(grounder);
		}
		add(id, delay, defid);
	}

	void findDelays() {
		if (getOption(VERBOSE_GROUNDING) > 0) {
			logActionAndTime("Finding delays");
			clog << "Solving optimization problem to find delays.\n";
		}
		structure->inter(groundsizeFunc)->graphInter(groundsize);
		structure->inter(symbolFunc)->graphInter(symbol);
		structure->checkAndAutocomplete();
		makeUnknownsFalse(symbol);
		makeUnknownsFalse(candelayon);
		makeUnknownsFalse(groundsize);
		makeUnknownsFalse(isdefdelay);
		makeUnknownsFalse(isequivalence);
		structure->clean();

		std::vector<AbstractStructure*> solutions;
		if (getOption(VERBOSE_GROUNDING) > 1) {
			clog << "Structure used:\n" << toString(structure) << "\n";
		}

#warning delay_optimization currently only works if useIFCompletion is FALSE!!! (has extra constraint)
		auto savedoptions = getGlobal()->getOptions();
		auto newoptions = new Options(false);
		getGlobal()->setOptions(newoptions);
		setOption(VERBOSE_GROUNDING, 1);
		setOption(VERBOSE_CREATE_GROUNDERS, 0);
		setOption(VERBOSE_SOLVING, 1);
		setOption(VERBOSE_GROUNDSTATS, 0);
		setOption(VERBOSE_PROPAGATING, 0);
		setOption(GROUNDWITHBOUNDS, true);
		setOption(LIFTEDUNITPROPAGATION, true);
		setOption(LONGESTBRANCH, 8);
		setOption(NRPROPSTEPS, 4);
		setOption(CPSUPPORT, true);
		setOption(TSEITINDELAY, false);
		setOption(SATISFIABILITYDELAY, false);
		setOption(NBMODELS, 1);
//		setOption(TIMEOUT, 10);

		map<FormulaGrounder*, shared_ptr<Delay> > grounder2delay;
		for (auto g : grounders) {
			if (g != NULL) { // eithers grounders or rulegrounders is null for an index
				grounder2delay.insert( { g, NULL });
			}
		}
		map<DefinitionGrounder*, std::set<RuleGrounder*> > defg2delayedrules;
		for (auto def : defgrounders) {
			defg2delayedrules.insert( { def, { } });
		}

		try {
			solutions = ModelExpansion::doMinimization(theory, structure, minimterm, NULL, NULL);
			if (solutions.size() == 0) {
				clog << "Problematic structure: " << print(structure) << "\n";
			}
			Assert(solutions.size()==1);

			if (getOption(VERBOSE_GROUNDING) > 0) {
				clog << "Minimal solution: " << print(solutions[0]) << "\n";
			}

			auto delayInter = solutions[0]->inter(voc->pred("delay/3"));

			std::set<uint> idsseen;
			for (auto tupleit = delayInter->ct()->begin(); not tupleit.isAtEnd(); ++tupleit) {
				auto constraintid = (*tupleit)[0]->value()._int;
				idsseen.insert(constraintid);
				auto atomid = (*tupleit)[2]->value()._int;
				if (grounders[constraintid] != NULL) {
					if (grounder2delay[grounders[constraintid]].get() == NULL) {
						grounder2delay[grounders[constraintid]] = make_shared<Delay>();
					}
					grounder2delay[grounders[constraintid]]->condition.push_back(containeratoms[atomid]);
				} else { // was a rule
					auto rg = rulegrounders[constraintid];
					defg2delayedrules[rule2def[rg]].insert(rg);
				}
			}
		} catch (TimeoutException& excp) {
			// TODO retrieve best model (currently not possible)
		}

		getGlobal()->setOptions(savedoptions);

		for (auto g2del : grounder2delay) {
			if (g2del.second.get() == NULL) {
				toGround.push_back(g2del.first);
				continue;
			}
			auto delay = g2del.second;
			auto& condition = delay->condition;
			// TODO handle equivgrounder as HACK by checking that we delayed on P and ~P in same delay (and split in two delays)
			// => in fact, the usage of delay is hacked, combining disjunctive and conjunctive meaning in separate components
			if (condition.size() == 2 && condition[0].symbol == condition[1].symbol && condition[0].watchedvalue != condition[1].watchedvalue) {
				auto g = g2del.first;
				auto del2 = make_shared<Delay>();
				del2->query = delay->query;
				del2->condition.push_back(condition[1]);
				condition.pop_back();
				if (getOption(VERBOSE_GROUNDING) > 0) {
					clog << "Delaying formula " << toString(g) << " by " << toString(delay) << "\n";
				}
				manager->formwithdelaytobeinitialized.push_back( { g, delay });
				if (getOption(VERBOSE_GROUNDING) > 0) {
					clog << "Delaying formula " << toString(g) << " by " << toString(del2) << "\n";
				}
				manager->formwithdelaytobeinitialized.push_back( { g, del2 });
				continue;
			}
			if (getOption(VERBOSE_GROUNDING) > 0) {
				clog << "Delaying formula " << toString(g2del.first) << " by " << toString(g2del.second) << "\n";
			}
			manager->formwithdelaytobeinitialized.push_back(g2del);
		}

		for (auto df2rules : defg2delayedrules) {
			if (getOption(VERBOSE_GROUNDING) > 0) {
				for (auto rule : df2rules.second) {
					clog << "Delaying rule " << toString(rule) << "\n";
				}
			}
			manager->rulegrounderstodelay.push(df2rules);
		}

		for (auto g : toGround) {
			if (getOption(VERBOSE_GROUNDING) > 0) {
				clog << "Did not delay " << toString(g) << "\n";
			}
			manager->toGround.push(g);
		}
		if (getOption(VERBOSE_GROUNDING) > 0) {
			logActionAndTime("Finding delays done");
		}
	}

private:
	void addGroundSize(Grounder* grounder, const DomainElement* id) { // FIXME should be checked in c++ instead of in IDP, simplifying the optimization problem
		auto gsize = grounder->getMaxGroundSize();
		int size = 0;
		if (gsize._type != TableSizeType::TST_EXACT) {
			size = log(getMaxElem<int>()) / log(2);
		} else {
			auto dsize = log(toDouble(gsize)) / log(2);
			size = (int) min(dsize, (double) getMaxElem<int>());
		}
		size = size < 0 ? 0 : size; // Handle log of 0
		groundsize->makeTrue( { id, createDomElem(size) }, true);
	}

	void add(const DomainElement* cid, shared_ptr<Delay> delay, int defid, bool equivalence = false) { // -1 is not defined
		for (auto conjunct : delay->condition) {
			containeratoms.push_back(conjunct);
			auto pfid = createDomElem((int) containeratoms.size() - 1);
			auto symbolid = getSymbolID(conjunct.symbol, maxid, symbol2id, id2symbol);
			symbol->makeTrue( { pfid, createDomElem(symbolid) }, true);
			candelayon->makeTrue( { cid, conjunct.watchedvalue ? truedm : falsedm, pfid }, true);
			if (defid != -1) {
				isdefdelay->makeTrue( { pfid, cid, createDomElem(defid) }, true);
			}
		}
		if (equivalence) {
			isequivalence->makeTrue( { cid }, true);
		}
	}
};

void LazyGroundingManager::internalRun(ConjOrDisj& formula, LazyGroundingRequest&) {
	formula.setType(Conn::CONJ);

	if (getOption(SATISFIABILITYDELAY)) {
		// FINDING DELAY
		DelayInitializer d(this);
		while (not tobeinitialized.empty()) {
			auto grounder = tobeinitialized.front();
			tobeinitialized.pop();
			d.addGrounder(grounder);
		}
		for (uint i = 0; i < formwithdelaytobeinitialized.size(); ++i) { // Note: can increase during grounding, so do not use special loops!
			d.addGrounder(formwithdelaytobeinitialized[i].first, formwithdelaytobeinitialized[i].second);
		}
		formwithdelaytobeinitialized.clear();
		d.findDelays();
		// END FINDING DELAYS
	}

	Grounder::_groundedatoms = 0;
	Grounder::_fullgroundsize = tablesize(TableSizeType::TST_EXACT, 0);
	auto temptobeinit = tobeinitialized;
	while (not temptobeinit.empty()) {
		auto grounder = temptobeinit.front();
		temptobeinit.pop();
		Grounder::addToFullGroundingSize(grounder->getMaxGroundSize());
	}
	for (auto fg : formwithdelaytobeinitialized) {
		Grounder::addToFullGroundingSize(fg.first->getMaxGroundSize());
	}
	auto temptoground = toGround;
	while (not temptoground.empty()) {
		auto grounder = temptoground.front();
		temptoground.pop();
		Grounder::addToFullGroundingSize(grounder->getMaxGroundSize());
	}
	auto temprules = rulegrounderstodelay;
	while (not temprules.empty()) {
		auto grounder = temprules.front();
		temprules.pop();
		for (auto rules : grounder.second) {
			Grounder::addToFullGroundingSize(rules->getMaxGroundSize());
		}
	}

	resolveQueues();
}

void LazyGroundingManager::delay(DefinitionGrounder* dg) {
	std::set<RuleGrounder*> delayable;
	for (auto rg : dg->getSubGrounders()) {
		auto s = rg->getHead()->symbol();
		bool canwatch = (not getOption(CPSUPPORT) || not s->isFunction()); // TODO combine cpsupport with lazy grounding
		for (auto truthandrule : symbol2watchedrules[s]) {
			for (auto rule : truthandrule.second) {
				if (rule->getConstruction() != dg->getDefinitionID()) {
					canwatch = false;
					break;
				}
			}
			if (not canwatch) {
				break;
			}
		}
		if (canwatch) {
			delayable.insert(rg);
		}
	}
	delay(dg, delayable);
}

// Add watches for all that have already been introduced into the grounding
void LazyGroundingManager::fireAllKnown(DelayedRule* delrule, bool watchedvalue) {
	Assert(resolvingqueues);
	auto symbol = delrule->getRule().head()->symbol();
	const auto& knownlits = symbol2knownlits[symbol][watchedvalue];
	for (const auto& atom2tuple : knownlits) {
		specificFire(delrule, watchedvalue ? atom2tuple.first : -atom2tuple.first, atom2tuple.second);
	}
}

void LazyGroundingManager::delay(DefinitionGrounder* dg, const std::set<RuleGrounder*>& delayable) {
	vector<RuleGrounder*> unwatchable;
	for (auto rg : dg->getSubGrounders()) {
		if (contains(delayable, rg)) {
			if (verbosity() > 2) {
				clog << "Delaying rule " << toString(rg) << " on its head.\n";
			}
			auto delrule = new DelayedRule(rg, dg->getDefinitionID());
			symbol2watchedrules[rg->getHead()->symbol()][true].push_back(delrule);
			fireAllKnown(delrule, true);
			//clog <<"Notifying output voc\n";
			checkAddedDelay(rg->getHead(), true, true);
			if (not useUFSAndOnlyIfSem()) {
				symbol2watchedrules[rg->getHead()->symbol()][false].push_back(delrule);
				fireAllKnown(delrule, false);
				checkAddedDelay(rg->getHead(), false, true);
			}
		} else {
			unwatchable.push_back(rg);
		}
	}
	auto newdg = new DefinitionGrounder(getGrounding(), unwatchable, dg->getContext());
	groundersRegisteredForDeletion.push_back(newdg);
	toGround.push(newdg);
}

void LazyGroundingManager::resolveQueues() {
	Assert(not resolvingqueues);
	resolvingqueues = true;
	bool changes = true;
	while (changes) {
		changes = false;
		while (not tobeinitialized.empty()) {
			changes = true;
			auto g = tobeinitialized.front();
			tobeinitialized.pop();
			addToManager(g);
		}
		while (not rulegrounderstodelay.empty()) { // TODO duplicate!
			changes = true;
			auto dg2delayable = rulegrounderstodelay.front();
			rulegrounderstodelay.pop();
			delay(dg2delayable.first, dg2delayable.second);
		}
		for (uint i = 0; i < formwithdelaytobeinitialized.size(); ++i) { // Note: can increase during grounding, so do not use special loops!
			changes = true;
			delay(formwithdelaytobeinitialized[i].first, formwithdelaytobeinitialized[i].second);
		}
		formwithdelaytobeinitialized.clear();
		while (not toGround.empty()) {
			changes = true;
			auto grounder = toGround.front();
			toGround.pop();
			if (verbosity() > 2) {
				clog << "Grounding not-delayed " << toString(grounder) << ".\n";
			}
			if(grounder->toplevelRun()){
				throw UnsatException();
			}
		}

		while (not queuedforgrounding.empty()) {
			changes = true;
			auto litAndValue = queuedforgrounding.front();
			queuedforgrounding.pop();
			fired(litAndValue.first, litAndValue.second);
		}
		for (auto def : tempdefs) {
			getGrounding()->add(*def.second);
			delete (def.second);
		}
		tempdefs.clear();
	}
	resolvingqueues = false;
}

void LazyGroundingManager::addToManager(Grounder* grounder) {
	Assert(grounder->getContext()._conjunctivePathFromRoot);

// NOTE: code keeps falling through until one case matches (and should RETURN then). If no smart case matches, the grounder is scheduled for grounding asap.

	bool donesplit = split(grounder);
	if (donesplit) {
		return;
	}

	if (getOption(SATISFIABILITYDELAY) && isa<DefinitionGrounder>(*grounder)) {
		auto dg = dynamic_cast<DefinitionGrounder*>(grounder);
		delay(dg);
		return;
	}

	if (getOption(SATISFIABILITYDELAY) && isa<FormulaGrounder>(*grounder) && grounder->hasFormula()) {
		auto fg = dynamic_cast<FormulaGrounder*>(grounder);

		auto delayforms = FormulaUtils::findDelay(fg->getFormula(), fg->getVarmapping(), this);
		if (delayforms.get() != NULL) {
			delay(fg, delayforms);
			return;
		}
	}

	if (verbosity() > 2 && grounder->hasFormula() && getOption(SATISFIABILITYDELAY)) {
		clog << "Did not find delay for formula " << toString(grounder->getFormula()) << "\n";
	}

	groundersRegisteredForDeletion.push_back(grounder);
	toGround.push(grounder);
}

//============================
//		DELAY ADDITION
//============================

void LazyGroundingManager::delay(FormulaGrounder* grounder, shared_ptr<Delay> delay) {
	groundersRegisteredForDeletion.push_back(grounder);

	if (delay.get() == NULL) {
		toGround.push(grounder);
		return;
	}

	auto formula = grounder->getFormula();
	if (isa<QuantForm>(*formula)) {
		std::set<const DomElemContainer*> delayedcontainers;
		for (auto conjunct : delay->condition) {
			for (auto arg : conjunct.args) {
				delayedcontainers.insert(arg);
			}
		}
		varset rootquantvars;
		auto recurse = formula; // NOTE: In the end, it is the subformula to use!
		while (isa<QuantForm>(*recurse)) {
			auto qf = dynamic_cast<QuantForm*>(recurse);
			if (not qf->isUnivWithSign()) {
				break;
			}
			addAll(rootquantvars, qf->quantVars());
			recurse = qf->subformula();
		}
		varset delayed, notdelayed;
		for (auto var : rootquantvars) {
			auto container = grounder->getVarmapping().find(var)->second;
			if (contains(delayedcontainers, container)) {
				delayed.insert(var);
			} else {
				notdelayed.insert(var);
			}
		}

		if (not notdelayed.empty()) {
			// map all delayed containers to their variables and then to their new containers
			std::map<const DomElemContainer*, Variable*> origcontainer2var;
			for (auto var2cont : grounder->getVarmapping()) {
				origcontainer2var[var2cont.second] = var2cont.first;
			}

			auto newformula = new QuantForm(SIGN::POS, QUANT::UNIV, delayed, new QuantForm(SIGN::POS, QUANT::UNIV, notdelayed, recurse, formula->pi()),
					formula->pi());
			grounder = GrounderFactory::createSentenceGrounder(this, newformula);

			for (auto& conjunct : delay->condition) {
				for (uint i = 0; i < conjunct.args.size(); ++i) {
					auto var = origcontainer2var[conjunct.args[i]];
					conjunct.args[i] = grounder->getVarmapping().at(var);
				}
			}
		}
	}

	auto delsent = new DelayedSentence(grounder, delay); // TODO rest is duplicate code with other add method
	for (auto conjunct : delsent->delay->condition) {
		symbol2watchedsentences[conjunct.symbol][conjunct.watchedvalue].push_back(delsent);
		std::vector<Term*> terms;
		for (auto arg : conjunct.args) {
			Variable* var = NULL;
			for (auto testvar : grounder->getVarmapping()) {
				if (testvar.second == arg) {
					var = testvar.first;
					break;
				}
			}
			if (var == NULL) {
				throw IdpException("Invalid code path.");
			}
			terms.push_back(new VarTerm(var, { }));
		}
		PredForm pf(SIGN::POS, conjunct.symbol, terms, { });
		checkAddedDelay(&pf, conjunct.watchedvalue, false);

		Assert(resolvingqueues);
		// Add watches for all that have already been introduced into the grounding
		for (auto conjunct : delsent->delay->condition) {
			const auto& knownlits = symbol2knownlits[conjunct.symbol][conjunct.watchedvalue];
			for (const auto& atom2tuple : knownlits) {
				specificFire(delsent, conjunct.watchedvalue ? atom2tuple.first : -atom2tuple.first, conjunct.symbol, atom2tuple.second);
			}
		}
	}
}

void LazyGroundingManager::addKnownToStructures(PredForm* pf, bool watchedvalue) {
	if (contains(alreadyAddedFromStructures, std::pair<const PFSymbol*, bool>{ pf->symbol(), watchedvalue })) {
		return;
	}
	alreadyAddedFromStructures.insert( { pf->symbol(), watchedvalue });
	std::vector<Variable*> vars;
	std::vector<Pattern> pattern;
	std::vector<Formula*> eqs;
	auto newpf = pf->clone();
	if (not watchedvalue) {
		newpf->negate();
	}
	eqs.push_back(newpf);
	for (auto arg : pf->args()) {
		if (arg->type() == TermType::VAR) {
			vars.push_back(dynamic_cast<VarTerm*>(arg)->var());
		} else {
			Assert(arg->type()==TermType::DOM);
			auto sort = arg->sort();
			auto newvar = new Variable(sort);
			vars.push_back(newvar);
			eqs.push_back(new EqChainForm(SIGN::POS, true, { new VarTerm(newvar, { }), arg }, { CompType::EQ }, { }));
		}
		pattern.push_back(Pattern::INPUT);
	}
	auto formula = BoolForm(SIGN::POS, true, eqs, { });
	Query q("", vars, &formula, { });
	auto table = Querying::doSolveQuery(&q, getStructure(), _structures.symstructure);
	for (auto i = table->begin(); not i.isAtEnd(); ++i) {
		auto atom = getTranslator()->translateNonReduced(pf->symbol(), *i);
		notifyBecameTrue(watchedvalue ? atom : -atom, true);
	}
}

void LazyGroundingManager::addToOutputVoc(PFSymbol* symbol, bool expensiveConstruction) {
	if (contains(alreadyAddedToOutputVoc, symbol)) {
		return;
	}

	if (not getOption(EXPANDIMMEDIATELY)) {
		if (not expensiveConstruction) {
			return;
		}
		if (_outputvocabulary != NULL && not _outputvocabulary->contains(symbol)) {
			return;
		}
	}

	alreadyAddedToOutputVoc.insert(symbol);

	// go over the symbol table, find all unknown literals and notify the solver that it should decide them.
	if (symbol->builtin() || symbol->overloaded() || getStructure()->inter(symbol)->approxTwoValued()) {
		return;
	}

	litlist lits;
	std::vector<Term*> varterms, varterms2;
	std::vector<Variable*> vars;
	for(auto s:symbol->sorts()){
		auto newvar = new Variable(s);
		vars.push_back(newvar);
		varterms.push_back(new VarTerm(newvar, TermParseInfo()));
		varterms2.push_back(new VarTerm(newvar, TermParseInfo()));
	}
	// FIXME expensive when they are in fact each others inverse
	auto posstrue = new PredForm(SIGN::POS, symbol->derivedSymbol(SymbolType::ST_PT), varterms, FormulaParseInfo());
	auto possfalse = new PredForm(SIGN::POS, symbol->derivedSymbol(SymbolType::ST_PF), varterms2, FormulaParseInfo());
	auto formula = new BoolForm(SIGN::POS, true, posstrue, possfalse, FormulaParseInfo());
	Query q("", vars, formula, { });
	auto table = Querying::doSolveQuery(&q, getStructure(), _structures.symstructure);
	for (auto i = table->begin(); not i.isAtEnd(); ++i) {
		auto lit = getTranslator()->translateReduced(symbol, *i, false); // NOTE: also creates all those literals, so also notifyNewLiteral gets called!
		if (lit != _true && lit != _false) {
			lits.push_back(lit);
		}
	}
	notifyForOutputVoc(symbol, lits);
}
void LazyGroundingManager::checkAddedDelay(PredForm* pf, bool watchedvalue, bool expensiveConstruction) {
	if (getOption(CPSUPPORT) && pf->symbol()->isFunction()) { // TODO combine cpsupport with lazy grounding
		throw notyetimplemented("Invalid code path: lazy grounding with support for function symbols in the grounding.");
	}

//	cerr <<"Adding\n";
	addKnownToStructures(pf, watchedvalue);
//	cerr <<"Outputting\n";
	addToOutputVoc(pf->symbol(), expensiveConstruction);
//	cerr <<"Introduced\n";
	for(auto tuple2lit: getTranslator()->getIntroducedLiteralsFor(pf->symbol())){
		needWatch(watchedvalue, tuple2lit.second); // FIXME not if already known?
	}
//	cerr <<"Done\n";
}

void LazyGroundingManager::needWatch(bool watchedvalue, Lit translatedliteral) {
	getGrounding()->notifyLazyWatch(translatedliteral, watchedvalue ? TruthValue::True : TruthValue::False, this);
}

//============================
//		NOTIFICATIONS
//============================

// Note: none of the literals should be true/false
void LazyGroundingManager::notifyForOutputVoc(PFSymbol* symbol, const litlist& literals) {
	if (not getOption(SATISFIABILITYDELAY)) {
		return;
	}
	if (_outputvocabulary != NULL && symbol != NULL && not _outputvocabulary->contains(symbol)) {
		return;
	}
	auto solver = dynamic_cast<SolverTheory*>(getGrounding());
	if (solver == NULL) {
		return;
	}
	solver->requestTwoValued(literals);
}

void LazyGroundingManager::notifyBecameTrue(const Lit& lit, bool onlyqueue) {
	queuedforgrounding.push( { abs(lit), lit < 0 ? false : true });
	if (verbosity() > 2) {
		clog << "Queued firing of " << getTranslator()->printL(lit) << "\n";
	}
	if (onlyqueue) {
		return;
	}
	if (not resolvingqueues) {
		resolveQueues();
	}
}

void LazyGroundingManager::notifyNewLiteral(PFSymbol* symbol, const ElementTuple&, Lit translatedliteral) {
	if (not useLazyGrounding()) {
		return;
	}
	Assert(translatedliteral!=_true && translatedliteral!=_false);
	if (verbosity() > 2) {
		clog << "Notified of new literal " << getTranslator()->printL(translatedliteral) << "\n";
	}

	notifyForOutputVoc(symbol, { translatedliteral });

	auto needwatch = false;
	bool needtrue = false, needfalse = false;

	auto watchit = symbol2watchedrules.find(symbol);
	if (watchit != symbol2watchedrules.cend()) {
		needwatch = true;
		if (hasNonEmpty(watchit->second, true)) {
			needtrue = true;
		}
		if (hasNonEmpty(watchit->second, false)) {
			needfalse = true;
		}
	}
	auto watchit2 = symbol2watchedsentences.find(symbol);
	if (watchit2 != symbol2watchedsentences.cend()) {
		needwatch = true;
		if (hasNonEmpty(watchit2->second, true)) {
			needtrue = true;
		}
		if (hasNonEmpty(watchit2->second, false)) {
			needfalse = true;
		}
	}

	if (needwatch) {
		if (needtrue) {
			needWatch(true, translatedliteral);
		}
		if (needfalse) {
			needWatch(false, translatedliteral);
		}
	}
	if (verbosity() > 2) {
		clog << "\n";
	}
}

//============================
//	  ENDED NOTIFICATIONS
//============================

//============================
//			 FIRING
//============================

// TODO remove watch if fully grounded
void LazyGroundingManager::fired(Atom atom, bool value) {
	if (verbosity() > 2) {
		clog << "Fired " << getTranslator()->printL(atom) << " on " << value << "\n";
	}

	if (not getTranslator()->isInputAtom(atom)) {
		return;
	}
	const auto& symbol = getTranslator()->getSymbol(atom);
	const auto& args = getTranslator()->getArgs(atom);

// FIXME need some assertion or check that any true or false fired literal will also already have fired on decidable

	auto lit = value ? atom : -atom;
	if (not contains(alreadygroundedlits, lit)) {
		symbol2knownlits[symbol][value].push_back( { atom, args }); // TODO table per argument. use indexing induced by the delays at hand
		alreadygroundedlits.insert(lit);
	}

	auto symbolit = symbol2watchedrules.find(symbol);
	if (symbolit != symbol2watchedrules.cend()) {
		for (auto rule : symbolit->second[value]) {
			specificFire(rule, lit, args);
		}
	}
	auto symbolit2 = symbol2watchedsentences.find(symbol);
	if (symbolit2 != symbol2watchedsentences.cend()) {
		auto& list = symbolit2->second[value];
		for (auto i = list.begin(); i < list.end();) {
			specificFire(*i, lit, symbol, args);
			if ((*i)->done) {
				i = list.erase(i);
			} else {
				++i;
			}
		}
	}
}

int groundingcount = 0;
void LazyGroundingManager::specificFire(DelayedRule* rule, Lit lit, const ElementTuple& args) {
	if (contains(rule2alreadygroundedlits[rule], lit)) {
		if (verbosity() > 5) {
			clog << "Already grounded for " << toString(rule->getRule()) << "\n";
		}
		return;
	}
//	clog <<"Grounding " <<groundingcount++ <<"\n";
	rule2alreadygroundedlits[rule].insert(lit);
	rule->notifyHeadFired(lit, args, getTranslator(), tempdefs);
}

void LazyGroundingManager::specificFire(DelayedSentence* sentence, Lit lit, PFSymbol* symbol, const ElementTuple& args) {
	if (contains(sent2alreadygroundedlits[sentence], lit)) {
		if (verbosity() > 5) {
			clog << "Already grounded for " << toString((sentence)->sentence) << "\n";
		}
		return;
	}
//	clog <<"Grounding " <<groundingcount++ <<"\n";
	sent2alreadygroundedlits[(sentence)].insert(lit);
	if (not (sentence)->done) {
		(sentence)->notifyFired(*this, symbol, args);
	}
}

void DelayedRule::notifyHeadFired(const Lit& head, const ElementTuple& tuple, GroundTranslator* translator, map<DefId, GroundDefinition*>& tempdefs) {
	Assert(tuple.size()==_rule->getHeadVarContainers().size());
	if (grounded.find(tuple) != grounded.cend()) { // Already grounded for this tuple
		return;
	}
	if (getOption(VERBOSE_GROUNDING) > 3) {
		clog << "Head literal " << translator->printL(head) << " of rule " << toString(_rule->getRule()) << " fired \n";
	}
	for (uint i = 0; i < tuple.size(); ++i) {
		*_rule->getHeadVarContainers()[i] = tuple[i];
	}
	grounded.insert(tuple);

	auto it = tempdefs.find(construction);
	if (it == tempdefs.cend()) {
		tempdefs[construction] = new GroundDefinition(construction, translator);
	}

	tempdefs[construction]->addPCRule(head, { }, false, false); // NOTE: important: add default false rule!
	_rule->groundForSetHeadInstance(*tempdefs[construction], head, tuple);
}

void DelayedSentence::notifyFired(const LazyGroundingManager& manager, PFSymbol* symbol, const ElementTuple& tuple) {
	if (getOption(VERBOSE_GROUNDING) > 3) {
		clog << "Literal " << toString(symbol) << toString(tuple) << " in sentence " << toString(sentence->getFormula()) << " fired \n";
	}
	vector<uint> matchedindices;
	bool allinst = true;
	for (uint i = 0; i < delay->condition.size(); ++i) {
		auto& delelem = delay->condition[i];
		if (delelem.symbol == symbol) {
			matchedindices.push_back(i);
		}
		if (manager.getFiredLits(delelem.symbol, delelem.watchedvalue).empty()) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << "Empty for " << toString(delelem.symbol) << " on value " << delelem.watchedvalue << "\n";
			}
			allinst = false;
			break;
		}
	}
	if (not allinst) { // Some of the tuples have not been instantiated yet, so do not have anything to ground yet.
		return;
	}

	for (auto index : matchedindices) {
/*		if (getOption(VERBOSE_GROUNDING) > 3) {
			uint count = 0;
			for (auto conjunct : delay->condition) {
				clog << "Firing for:\n" << "\t" << conjunct.symbol->name() << "[";
				if (count == index) {
					clog << toString(symbol) << toString(tuple) << ", ";
				} else {
					const auto& lits = manager.getFiredLits(conjunct.symbol, conjunct.watchedvalue);
					for (auto lit : lits) {
						clog << manager.getTranslator()->printL(lit.first) << ", ";
					}
				}
				clog << "]\n";
				count++;
			}
		}*/
		FireInformation fireinfo( { manager, delay->condition, 0, index, tuple, { } });
		recursiveFire(fireinfo);
	}
}

// To be reduced to the query at hand // TODO prevent regrounding same query?
void DelayedSentence::recursiveFire(FireInformation& info) {
	if (info.currentindex >= info.conjunction.size()) {
		//clog <<"\tMatched\n";
		auto lgr = LazyGroundingRequest(info.fullinstantiation);
		sentence->toplevelRun(lgr);
		if (lgr.groundersdone) {
			done = true;
		}
		return;
	}

	if (info.firedindex == info.currentindex) {
		singletuple[0].second = info.tuplefired;
		recursiveFire(singletuple, info);
	} else {
		recursiveFire(info.manager.getFiredLits(info.conjunction[info.currentindex].symbol, info.conjunction[info.currentindex].watchedvalue), info);
	}
}

void DelayedSentence::recursiveFire(const vector<pair<Atom, ElementTuple>>& tuples, FireInformation& info) {
	if (done) {
		return;
	}
	auto savedinst = info.fullinstantiation;
	auto changed = false;
	for (auto atomandtuple : tuples) {
		if(changed){
			info.fullinstantiation = savedinst;
		}
		changed = false;
		bool valid = true;
		for (uint i = 0; i < atomandtuple.second.size(); ++i) {
			const auto& container = info.conjunction[info.currentindex].args[i];
			if (contains(info.fullinstantiation, container)) {
				if (container->get() != atomandtuple.second[i]) { // Non-matching instantiation
					valid = false;
					if (getOption(VERBOSE_GROUNDING) > 5) {
						clog << "Instantiation did not match the contents\n";
					}
					break;
				}
			} else {
				*container = atomandtuple.second[i];
				changed = true;
				info.fullinstantiation.insert(container);
			}
			if (not info.conjunction[info.currentindex].tables[i]->contains(atomandtuple.second[i])) { // Instantiation does not match sorttable
				// FIXME also add to head firing!
				if (getOption(VERBOSE_GROUNDING) > 5) {
					clog << "Did not match the instantiation!\n";
				}
				valid = false;
				break;
			}
		}
		if (not valid) {
			continue;
		}
		info.currentindex++;
		recursiveFire(info);
		if (done) {
			return;
		}
		info.currentindex--;
	}
	info.fullinstantiation = savedinst;
}

//============================
//		ENDED FIRING
//============================

bool LazyGroundingManager::canBeDelayedOn(PFSymbol* symbol, bool truewatch) const {
	if (symbol->isFunction() && getOption(CPSUPPORT)) { // TODO combine cpsupport with lazy grounding
		return false;
	}
	auto symbolit = symbol2watchedrules.find(symbol);
	if (symbolit != symbol2watchedrules.cend()) {
		if (isNonEmptyFor(symbolit->second, not truewatch)) {
			return false;
		}
	}

	auto symbolit2 = symbol2watchedsentences.find(symbol);
	if (symbolit2 != symbol2watchedsentences.cend()) {
		if (isNonEmptyFor(symbolit2->second, not truewatch)) {
			return false;
		}
	}
// Note: also check the pending delays!
	for (auto f2del : formwithdelaytobeinitialized) {
		for (auto conjunct : f2del.second->condition) {
			if (conjunct.symbol == symbol && conjunct.watchedvalue != truewatch) {
				return false;
			}
		}
	}
	return true;
}

bool LazyGroundingManager::canBeDelayedOn(Formula* head, Formula* body) const {
	if (not isa<PredForm>(*head)) {
		return false;
	}
	auto pf = dynamic_cast<PredForm*>(head);
	return not FormulaUtils::containsSymbol(pf->symbol(), body); // TODO optimize: also true if it does not occur in a negative context.
}

void LazyGroundingManager::put(ostream&) const {
// TODO print delay information
}

// FIXME can be used to get a complete extension of the structure
void LazyGroundingManager::extendStructure(AbstractStructure* structure) const {
// TODO handle functions
// TODO future: handle sorts?
// TODO handle definitions
#warning equivalences handled incorrectly
	for (auto pred2inter : structure->getPredInters()) {
		auto pred = pred2inter.first;
		auto symbolit = symbol2watchedrules.find(pred);
		if (symbolit != symbol2watchedrules.cend()) {
			if (verbosity() > 0) {
				clog << "Symbol " << toString(pred) << " is watched as head in a definition, so the definition should be evaluated to extend it.\n";
			}
			continue;
		}
		auto symbolit2 = symbol2watchedsentences.find(pred);
		if (symbolit2 == symbol2watchedsentences.cend()) {
			continue;
		}
		const auto& bool2list = (*symbolit2).second;
		auto hastruedelay = bool2list.find(true) != bool2list.cend() and bool2list.at(true).size() > 0;
		auto hasfalsedelay = bool2list.find(false) != bool2list.cend() and bool2list.at(false).size() > 0;
		if (hastruedelay and hasfalsedelay) {
			if (verbosity() > 0) {
				clog << "Symbol " << toString(pred) << " is watched as head in an equivalence, so the equivalence should be evaluated to extend it.\n";
			}
			continue;
		}
		Assert(hastruedelay or hasfalsedelay);
		if (hastruedelay) {
			auto ct = pred2inter.second->ct();
			pred2inter.second->cf(new PredTable(InverseInternalPredTable::getInverseTable(ct->internTable()), ct->universe()));
		} else {
			auto cf = pred2inter.second->cf();
			pred2inter.second->ct(new PredTable(InverseInternalPredTable::getInverseTable(cf->internTable()), cf->universe()));
		}
	}
}

void DelayedSentence::put(std::ostream& stream) const {
	stream << "Delayed " << toString(sentence->getFormula()) << " on " << toString(delay) << "\n";
}
