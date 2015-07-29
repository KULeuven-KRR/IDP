#include "LazyGroundingManager.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/querying/Query.hpp"
#include "theory/Query.hpp"
#include "structure/StructureComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "utils/LogAction.hpp"
#include "errorhandling/UnsatException.hpp"
#include "InitialDelayComputation.hpp"

// TODO combine with delaying on functions and cpsupport, can also remove some checks in the code then

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

Rule* DelayedRule::getRule() const {
	return _rule->getRule();
}

ContainerAtom::~ContainerAtom(){
	// deleteList(tables); // FIXME deletion (not guaranteed to be unique)
}

DelayedSentence::DelayedSentence(FormulaGrounder* sentence, std::shared_ptr<Delay> delay)
		: 	done(false),
			sentence(sentence),
			delay(delay),
			singletuple(std::vector<std::pair<Atom, ElementTuple>>{ { -1, {} } }){
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
	translator()->addMonitor(this); // NOTE: request notifications of literal additions
	notifyForOutputVoc(NULL, { }); // NOTE: ugly hack to notify the solver there will be an output vocabulary (otherwise everything is default outputvoc)
	setMaxGroundSize(tablesize(TST_EXACT, 1));
}

LazyGroundingManager::~LazyGroundingManager() {
	deleteList<Grounder>(groundersRegisteredForDeletion);
	deleteList(tablesToDelete);
}

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
	Assert(d.get()!=NULL);
	formwithdelaytobeinitialized.push_back( { grounder, d });
	setMaxGroundSize(getMaxGroundSize() + grounder->getMaxGroundSize());
}

// NOTE: construction for this is the definition!!!
void LazyGroundingManager::add(FormulaGrounder* grounder, PredForm* atom, bool watchontrue, int definitionid) {
	setMaxGroundSize(getMaxGroundSize() + grounder->getMaxGroundSize());

	if (not getOption(SATISFIABILITYDELAY)) {
		toGround.push(grounder);
		return;
	}
	if (getOption(CPSUPPORT) && atom->symbol()->isFunction()) {
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
				auto sorttable = new SortTable(getStructure()->inter(var->sort())->internTable());;
				tablesToDelete.push_back(sorttable);
				elem.tables[elem.args.size() - 1] = sorttable;
			}
		}
	}
	elem.watchedvalue = watchontrue;
	d->condition.push_back(elem);
	formwithdelaytobeinitialized.push_back( { grounder, d });
	fg2definitionid[grounder] = definitionid;
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
		bool canwatch = (not getOption(CPSUPPORT) || not s->isFunction());
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
	auto symbol = delrule->getRule()->head()->symbol();
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
				getGrounding()->addEmptyClause();
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

	auto fg = dynamic_cast<FormulaGrounder*>(grounder);
	if (getOption(SATISFIABILITYDELAY) && fg != NULL && fg->hasFormula()) {

		auto delayforms = FormulaUtils::findDelay(fg->getFormula(), fg->getVarmapping(), this);
		if (delayforms.get() != NULL) {
			delay(fg, delayforms);
			return;
		}
	}

	if (verbosity() > 2 && fg != NULL && fg->hasFormula() && getOption(SATISFIABILITYDELAY)) {
		clog << "Did not find delay for formula " << toString(fg->getFormula()) << "\n";
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

		map<Variable*, const DomainElement*> var2dom;
		for(auto freevar: formula->freeVars()){
			var2dom[freevar] = grounder->getVarmapping().at(freevar)->get();
		}

		recurse = FormulaUtils::substituteVarWithDom(recurse, var2dom);

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

			auto newformula = new QuantForm(SIGN::POS, QUANT::UNIV, notdelayed, recurse, formula->pi());
			if(not delayed.empty()){
				newformula = new QuantForm(SIGN::POS, QUANT::UNIV, delayed, newformula, formula->pi());
			}

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
		auto pf = new PredForm(SIGN::POS, conjunct.symbol, terms, { });
		checkAddedDelay(pf, conjunct.watchedvalue, false);
		pf->recursiveDelete();

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
	auto formula = new BoolForm(SIGN::POS, true, eqs, { });
	Query q("", vars, formula, { });
	auto table = Querying::doSolveQuery(&q, getStructure(), _structures.symstructure);
	for (auto i = table->begin(); not i.isAtEnd(); ++i) {
		auto atom = translator()->translateNonReduced(pf->symbol(), *i);
		notifyBecameTrue(watchedvalue ? atom : -atom, true);
	}
	//formula->recursiveDelete();
}

void LazyGroundingManager::addToOutputVoc(PFSymbol* symbol, bool expensiveConstruction) {
	if (contains(alreadyAddedToOutputVoc, symbol)) {
		return;
	}

	if (not getOption(EXPANDIMMEDIATELY)) {
		if (not expensiveConstruction && not _nbModelsEquivalent) {
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
		auto lit = translator()->translateReduced(symbol, *i, false); // NOTE: also creates all those literals, so also notifyNewLiteral gets called!
		if (lit != _true && lit != _false) {
			lits.push_back(lit);
		}
	}
	notifyForOutputVoc(symbol, lits);
	formula->recursiveDelete();
}
void LazyGroundingManager::checkAddedDelay(PredForm* pf, bool watchedvalue, bool expensiveConstruction) {
	if (getOption(CPSUPPORT) && pf->symbol()->isFunction()) {
		throw notyetimplemented("Invalid code path: lazy grounding with support for function symbols in the grounding.");
	}

	addKnownToStructures(pf, watchedvalue);
	addToOutputVoc(pf->symbol(), expensiveConstruction);
	for(auto tuple2lit: translator()->getIntroducedLiteralsFor(pf->symbol())){
		needWatch(watchedvalue, tuple2lit.second);
	}
}

void LazyGroundingManager::needWatch(bool watchedvalue, Lit translatedliteral) {
	getGrounding()->notifyLazyWatch(translatedliteral, watchedvalue ? TruthValue::True : TruthValue::False, this);
}

//============================
//		NOTIFICATIONS
//============================

// Note: none of the literals should be true/false
void LazyGroundingManager::notifyForOutputVoc(PFSymbol* symbol, const litlist& literals) {
	// NOTE: this code is used for two things
	// 1. whenever an output literal is added to the solver: notify the solver that this literal is output
	// 2. whenever some output literal is delayed on, also notify the solver. This is important since otherwise the model invalidating clauses will be invalid when lazygrounding
	// Hence, notifying the solver happens whenever 1. there is an outputvoc or 2. we are lazy grounding (SATDELAY)
	// The first return in this procedure reflects this behavior.
	if (not getOption(SATISFIABILITYDELAY) && _outputvocabulary == NULL) {
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
		clog << "Queued firing of " << translator()->printLit(lit) << "\n";
	}
	if (onlyqueue) {
		return;
	}
	if (not resolvingqueues) {
		resolveQueues();
	}
}

void LazyGroundingManager::notifyNewLiteral(PFSymbol* symbol, const ElementTuple&, Lit translatedliteral) {

	Assert(translatedliteral!=_true && translatedliteral!=_false);
	if (verbosity() > 2) {
		clog << "Notified of new literal " << translator()->printLit(translatedliteral) << "\n";
	}

	notifyForOutputVoc(symbol, { translatedliteral });

	if (not useLazyGrounding()) {
		return;
	}
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
//			 FIRING
//============================

// TODO remove watch if fully grounded
void LazyGroundingManager::fired(Atom atom, bool value) {
	if (verbosity() > 2) {
		clog << "Fired " << translator()->printLit(atom) << " on " << value << "\n";
	}

	if (not translator()->isInputAtom(atom)) {
		return;
	}
	const auto& symbol = translator()->getSymbol(atom);
	const auto& args = translator()->getArgs(atom);

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

void LazyGroundingManager::specificFire(DelayedRule* rule, Lit lit, const ElementTuple& args) {
	if (contains(rule2alreadygroundedlits[rule], lit)) {
		if (verbosity() > 5) {
			clog << "Already grounded for " << toString(rule->getRule()) << "\n";
		}
		return;
	}
	rule2alreadygroundedlits[rule].insert(lit);
	rule->notifyHeadFired(lit, args, translator(), tempdefs);
}

void LazyGroundingManager::specificFire(DelayedSentence* sentence, Lit lit, PFSymbol* symbol, const ElementTuple& args) {
	if (contains(sent2alreadygroundedlits[sentence], lit)) {
		if (verbosity() > 5) {
			clog << "Already grounded for " << toString((sentence)->sentence) << "\n";
		}
		return;
	}
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
		clog << "Head literal " << translator->printLit(head) << " of rule " << toString(_rule->getRule()) << " fired \n";
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
		FireInformation fireinfo( { manager, delay->condition, 0, index, tuple, { } });
		recursiveFire(fireinfo);
	}
}

// To be reduced to the query at hand // TODO prevent regrounding same query?
void DelayedSentence::recursiveFire(FireInformation& info) {
	if (info.currentindex >= info.conjunction.size()) {
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


bool LazyGroundingManager::canBeDelayedOn(PFSymbol* symbol, bool truewatch) const {
	if (symbol->isFunction() && getOption(CPSUPPORT)) {
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

Grounder* LazyGroundingManager::getFirstSubGrounder() const {
	if(not toGround.empty()){
		return toGround.front();
	}
	if(not tobeinitialized.empty()){
		return tobeinitialized.front();
	}
	return NULL;
}

// FIXME can be used to get a complete extension of the structure
std::vector<Definition*> LazyGroundingManager::extendStructure(Structure* structure) const {
// TODO handle functions, sorts and definitions
//#warning equivalences handled incorrectly
	std::vector<Definition*> postprocessdefs;
	for (auto pred2inter : structure->getPredInters()) {
		auto pred = pred2inter.first;
		auto symbolit = symbol2watchedrules.find(pred);
		if (symbolit != symbol2watchedrules.cend()) {
			auto def = new Definition();
			for(auto r: symbolit->second.at(true)){
				def->add(r->getRule());
			}
			for(auto r: symbolit->second.at(false)){
				def->add(r->getRule());
			}
			postprocessdefs.push_back(def);
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
	return postprocessdefs;
}

void DelayedSentence::put(std::ostream& stream) const {
	stream << "Delayed " << toString(sentence->getFormula()) << " on " << toString(delay) << "\n";
}

void LazyGroundingManager::notifyNewVarId(Function *symbol, const std::vector<GroundTerm>& vector, VarId& id) {
	if (not getOption(SATISFIABILITYDELAY) && _outputvocabulary == NULL) {
		return;
	}
	if (_outputvocabulary != NULL && symbol != NULL && not _outputvocabulary->contains(symbol)) {
		return;
	}
	auto solver = dynamic_cast<SolverTheory*>(getGrounding());
	if (solver == NULL) {
		return;
	}
	solver->requestTwoValued(id);
}
