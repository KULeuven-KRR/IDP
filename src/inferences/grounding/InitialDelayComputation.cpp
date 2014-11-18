#include "InitialDelayComputation.hpp"
#include "LazyGroundingManager.hpp"
#include "theory/TheoryUtils.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "utils/LogAction.hpp"
#include "InitialDelayComputation.hpp"

extern void parsefile(const std::string&);

using namespace std;

DelayInitializer::DelayInitializer(LazyGroundingManager* manager)
		: 	maxid(1),
			manager(manager) {
	if (not getGlobal()->instance()->alreadyParsed("delay_optimization")) {
		auto warnings = getOption(SHOWWARNINGS);
		auto autocomplete = getOption(AUTOCOMPLETE);
		setOption(BoolType::SHOWWARNINGS, false);
		setOption(BoolType::AUTOCOMPLETE, true);
		parsefile("delay_optimization");
		setOption(SHOWWARNINGS, warnings);
		setOption(AUTOCOMPLETE, autocomplete);
	}

	auto ns = getGlobal()->getGlobalNamespace();
	stringstream ss;
	ss << "delay_temp_voc" << getGlobal()->getNewID();
	voc = new Vocabulary(ss.str());
	voc->add(ns->vocabulary("Delay_Voc_Input"));
	theory = ns->theory("Delay_Theory")->clone();
	minimterm = ns->term("Delay_Minimization_Term")->clone();
	structure = dynamic_cast<Structure*>(ns->structure("Delay_Basic_Data"))->clone();
	theory->vocabulary(voc);
	minimterm->vocabulary(voc);
	structure->changeVocabulary(voc);

	constraint = structure->inter(voc->sort("Constraint"));
	noninfcost = structure->inter(voc->sort("noninfcost"));
	predform = structure->inter(voc->sort("PredForm"));
	symbolsort = structure->inter(voc->sort("Symbol"));

	symbol = structure->inter(voc->func("symbol/1"));
	candelayon = structure->inter(voc->pred("canDelayOn/3"));
	groundsize = structure->inter(voc->func("groundSize/1"));
	isdefdelay = structure->inter(voc->pred("isDefinitionDelay/3"));
	isequivalence = structure->inter(voc->pred("isEquivalence/1"));
	truedm = createDomElem("True");
	falsedm = createDomElem("False");
}

DelayInitializer::~DelayInitializer() {
	theory->recursiveDelete();
	minimterm->recursiveDelete();
	delete (structure);
	delete (voc);
}

void DelayInitializer::addGrounder(Grounder* grounder) {
	auto fg = dynamic_cast<FormulaGrounder*>(grounder);
	if (fg != NULL && fg->hasFormula()) {
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

void DelayInitializer::addGrounder(FormulaGrounder* grounder, shared_ptr<Delay> delay) {
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

void DelayInitializer::findDelays() {
	if (getOption(VERBOSE_GROUNDING) > 0) {
		logActionAndTime("Finding delays");
		clog << "Solving optimization problem to find delays.\n";
	}

//#warning delay_optimization currently only works if useIFCompletion is FALSE!!! (has extra constraint)
	auto savedoptions = getGlobal()->getOptions();
	auto newoptions = new Options(false);
	getGlobal()->setOptions(newoptions);
	setOption(POSTPROCESS_DEFS,false);
	setOption(VERBOSE_GROUNDING, 0);
	setOption(VERBOSE_CREATE_GROUNDERS, 0);
	setOption(VERBOSE_GEN_AND_CHECK, 0);
	setOption(VERBOSE_SOLVING, 0);
	setOption(VERBOSE_GROUNDING_STATISTICS, 0);
	setOption(VERBOSE_PROPAGATING, 0);
	setOption(GROUNDWITHBOUNDS, true);
	setOption(LIFTEDUNITPROPAGATION, true);
	setOption(LONGESTBRANCH, 8);
	setOption(NRPROPSTEPS, 4);
	setOption(CPSUPPORT, true);
	setOption(TSEITINDELAY, false);
	setOption(SATISFIABILITYDELAY, false);
	setOption(NBMODELS, 1);
	setOption(AUTOCOMPLETE, true);
//		setOption(TIMEOUT, 10);

	structure->inter(voc->func("sizeThreshold/0"))->graphInter()->makeTrueExactly( { createDomElem(getOption(LAZYSIZETHRESHOLD)) });
	makeUnknownsFalse(symbol->graphInter());
	makeUnknownsFalse(candelayon);
	makeUnknownsFalse(groundsize->graphInter());
	makeUnknownsFalse(isdefdelay);
	makeUnknownsFalse(isequivalence);
	auto newvoc = new Vocabulary("internelmore");
	newvoc->add(getGlobal()->getGlobalNamespace()->vocabulary("Delay_Voc"));
	theory->vocabulary(newvoc);
	minimterm->vocabulary(newvoc);
	structure->changeVocabulary(newvoc);
	delete (voc);
	voc = newvoc;
	structure->clean();

	if (getOption(VERBOSE_GROUNDING) > 1) {
		clog << "Structure used:\n" << toString(structure) << "\n";
	}

	map<FormulaGrounder*, shared_ptr<Delay> > grounder2delay;
	for (auto g : grounders) {
		if (g != NULL) { // eithers grounders or rulegrounders is null for an index
			grounder2delay.insert( { g, shared_ptr<Delay>() });
		}
	}
	map<DefinitionGrounder*, std::set<RuleGrounder*> > defg2delayedrules;
	for (auto def : defgrounders) {
		defg2delayedrules.insert( { def, { } });
	}

	auto solutions = ModelExpansion::doMinimization(theory, structure, minimterm, NULL, NULL)._models;

	if (getOption(VERBOSE_GROUNDING) > 0) {
		clog << "Minimal solution: " << print(solutions[0]) << "\n";
	}

	auto delayInter = solutions[0]->inter(solutions[0]->vocabulary()->pred("delay/3"));

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

	deleteList(solutions);

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
	for (auto def : defgrounders) {
		if (getOption(VERBOSE_GROUNDING) > 0) {
			clog << "Did not delay " << toString(def) << "\n";
		}
	}
	if (getOption(VERBOSE_GROUNDING) > 0) {
		logActionAndTime("Finding delays done");
	}
}

void DelayInitializer::addGroundSize(Grounder* grounder, const DomainElement* id) {
	// TODO use estimations to compute size, as below?
	//auto bdd = manager->_structures.symstructure->evaluate(grounder->getFormula(), TruthType::POSS_FALSE, manager->_structures.concrstructure);
	//EstimateEnumerationCost e;
	//e.run(new PredTable(new BDDInternalPredTable(bdd, manager->_structures.symstructure->obtainManager(), {}, manager->_structures.concrstructure), Universe{}), {});

	auto gsize = grounder->getMaxGroundSize();
	int size = 0;
	if (gsize._type != TableSizeType::TST_EXACT) {
		size = log(getMaxElem<int>()) / log(2);
	} else {
		auto dsize = log(toDouble(gsize)) / log(2);
		size = (int) min(dsize, (double) getMaxElem<int>());
	}
	size = size <= 0 ? 1 : size; // Handle log of 0
	constraint->add(id);
	noninfcost->add(createDomElem(size));
	groundsize->add( { id, createDomElem(size) });
}

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

void DelayInitializer::add(const DomainElement* cid, shared_ptr<Delay> delay, int defid, bool equivalence) {
	constraint->add(cid);
	for (auto conjunct : delay->condition) {
		containeratoms.push_back(conjunct);
		auto pfid = createDomElem((int) containeratoms.size() - 1);
		predform->add(pfid);
		auto symbolid = createDomElem(getSymbolID(conjunct.symbol, maxid, symbol2id, id2symbol));
		symbolsort->add(symbolid);
		symbol->add( { pfid, symbolid });
		candelayon->ct()->add( { cid, conjunct.watchedvalue ? truedm : falsedm, pfid });
		if (defid != -1) {
			auto defiddom = createDomElem(defid);
			structure->inter(voc->sort("DefID"))->add(defiddom);
			isdefdelay->ct()->add( { pfid, cid, defiddom });
		}
	}
	if (equivalence) {
		isequivalence->ct()->add( { cid });
	}
}
