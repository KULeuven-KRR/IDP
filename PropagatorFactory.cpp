#include "PropagatorFactory.hpp"

using namespace std;

template<class InterpretationFactory, class PropDomain>
FOPropagatorFactory<InterpretationFactory>::FOPropagatorFactory(InterpretationFactory* factory, FOPropScheduler* scheduler, bool as, const map<PFSymbol*,InitBoundType>& init, Options* opts)
	: _verbosity(opts->getValue(IntType::PROPAGATEVERBOSITY)), _initbounds(init), _assertsentences(as) {
	_propagator = new FOPropagator<InterpretationFactory, PropDomain>(factory, scheduler, opts);
	_multiplymaxsteps = opts->getValue(BoolType::RELATIVEPROPAGATIONSTEPS);
}

void FOPropagatorFactory::createleafconnector(PFSymbol* symbol) {
	if(_verbosity > 1) { cerr << "  Creating a leaf connector for " << *symbol << "\n";	}
	vector<Variable*> vars = VarUtils::makeNewVariables(symbol->sorts());
	vector<Term*> args = TermUtils::makeNewVarTerms(vars);
	PredForm* leafconnector = new PredForm(SIGN::POS,symbol,args,FormulaParseInfo());
	_leafconnectors[symbol] = leafconnector;
	_propagator->_leafconnectors[symbol] = leafconnector;
	switch(_initbounds[symbol]) {
		case IBT_TWOVAL:
			_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector);
			if(_verbosity > 1) { cerr << "    The leaf connector is twovalued\n";	}
			break;
		case IBT_BOTH:
		case IBT_CT:
		case IBT_CF:
			_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector,_initbounds[symbol]);
			break;
		case IBT_NONE:
			initFalse(leafconnector);
			if(_verbosity > 1) { cerr << "    The leaf connector is completely unknown\n";	}
			break;
	}
}

FOPropagator* FOPropagatorFactory::create(const AbstractTheory* theory) {
	if(_verbosity > 1) { cerr << "=== initialize propagation datastructures\n";	}

	// transform theory to a suitable normal form
	AbstractTheory* newtheo = theory->clone();
	TheoryUtils::completion(newtheo);
	TheoryUtils::removeNesting(newtheo);	// FIXME: remove nesting does not change F(x)=y to F(x,y) anymore, which is probably needed here
	TheoryUtils::graphFunctions(newtheo);
	TheoryUtils::graphAggregates(newtheo);
	TheoryUtils::removeEqChains(newtheo);

	// Add function constraints
	for(auto it = _initbounds.begin(); it != _initbounds.end(); ++it) {
		if(it->second != IBT_TWOVAL && typeid(*(it->first)) == typeid(Function)) {
			Function* function = dynamic_cast<Function*>(it->first);

			// Add  (! x : ? y : F(x) = y)
			vector<Variable*> vars = VarUtils::makeNewVariables(function->sorts());
			vector<Term*> terms = TermUtils::makeNewVarTerms(vars);
			PredForm* atom = new PredForm(SIGN::POS,function,terms,FormulaParseInfo());
			Variable* y = vars.back();
			set<Variable*> yset;
			yset.insert(y);
			QuantForm* exists = new QuantForm(SIGN::POS,QUANT::EXIST,yset,atom,FormulaParseInfo());
			vars.pop_back();
			set<Variable*> xset;
			xset.insert(vars.begin(),vars.end());
			QuantForm* univ1 = new QuantForm(SIGN::POS,QUANT::UNIV,xset,exists,FormulaParseInfo());
			newtheo->add(univ1);

			// Add	(! z y1 y2 : F(z) ~= y1 | F(z) ~= y2 | y1 = y2)
			vector<Variable*> zvars = VarUtils::makeNewVariables(function->insorts());
			Variable* y1var = new Variable(function->outsort());
			Variable* y2var = new Variable(function->outsort());
			vector<Variable*> zy1vars = zvars; zy1vars.push_back(y1var);
			vector<Variable*> zy2vars = zvars; zy2vars.push_back(y2var);
			vector<Variable*> y1y2vars; y1y2vars.push_back(y1var); y1y2vars.push_back(y2var);
			vector<Term*> zy1terms = TermUtils::makeNewVarTerms(zy1vars);
			vector<Term*> zy2terms = TermUtils::makeNewVarTerms(zy2vars);
			vector<Term*> y1y2terms = TermUtils::makeNewVarTerms(y1y2vars);
			vector<Formula*> atoms;
			atoms.push_back(new PredForm(SIGN::NEG,function,zy1terms,FormulaParseInfo()));
			atoms.push_back(new PredForm(SIGN::NEG,function,zy2terms,FormulaParseInfo()));
			atoms.push_back(new PredForm(SIGN::POS,VocabularyUtils::equal(function->outsort()),y1y2terms,FormulaParseInfo()));
			BoolForm* disjunction = new BoolForm(SIGN::POS,false,atoms,FormulaParseInfo());
			set<Variable*> zy1y2set;
			zy1y2set.insert(zvars.begin(),zvars.end());
			zy1y2set.insert(y1var);
			zy1y2set.insert(y2var);
			QuantForm* univ2 = new QuantForm(SIGN::POS,QUANT::UNIV,zy1y2set,disjunction,FormulaParseInfo());
			newtheo->add(univ2);
		}
	}

	// Multiply maxsteps if requested
	if(_multiplymaxsteps) _propagator->_maxsteps = _propagator->_maxsteps * TheoryUtils::nrSubformulas(newtheo);

	// create leafconnectors
	Vocabulary* voc = newtheo->vocabulary();
	for(map<string,Predicate*>::const_iterator it = voc->firstPred(); it != voc->lastPred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for(set<Predicate*>::const_iterator jt = sp.begin(); jt != sp.end(); ++jt) {
			createleafconnector(*jt);
		}
	}
	for(map<string,Function*>::const_iterator it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
		set<Function*> sf = it->second->nonbuiltins();
		for(set<Function*>::const_iterator jt = sf.begin(); jt != sf.end(); ++jt) {
			createleafconnector(*jt);
		}
	}

	// visit sentences
	newtheo->accept(this);

	return _propagator;
}

void FOPropagatorFactory::visit(const Theory* theory) {
	for(size_t n = 0; n < theory->sentences().size(); ++n) {
		Formula* sentence = theory->sentences()[n];
		if(_assertsentences) {
			_propagator->schedule(sentence,DOWN,true,0);
			_propagator->_domains[sentence] = ThreeValuedDomain(_propagator->_factory,true,false,sentence);
		}
		sentence->accept(this);
	}
}

void FOPropagatorFactory::initFalse(const Formula* f) {
	if(_verbosity > 2) { cerr << "  Assigning the least precise bounds to " << *f << "\n";	}
	if(_propagator->_domains.find(f) == _propagator->_domains.end()) {
		_propagator->_domains[f] = ThreeValuedDomain(_propagator->_factory,false,false,f);
	}
}

void FOPropagatorFactory::visit(const PredForm* pf) {
	initFalse(pf);
	PFSymbol* symbol = pf->symbol();
	if(symbol->builtin()) {
		map<const Formula*, const Formula*>::const_iterator it = _propagator->_upward.find(pf);
		if(it != _propagator->_upward.end()) {
			_propagator->_domains[pf] = ThreeValuedDomain(_propagator->_factory,pf);
			_propagator->schedule(it->second,UP,true,pf);
			_propagator->schedule(it->second,UP,false,pf);
		}
	}
	else {
		assert(_leafconnectors.find(symbol) != _leafconnectors.end());
		PredForm* leafconnector = _leafconnectors[symbol];
		_propagator->_leafupward[leafconnector].insert(pf);
		LeafConnectData lcd;
		lcd._connector = leafconnector;
		lcd._equalities = _propagator->_factory->trueDomain(leafconnector);
		for(unsigned int n = 0; n < symbol->sorts().size(); ++n) {
			assert(typeid(*(pf->subterms()[n])) == typeid(VarTerm));
			assert(typeid(*(leafconnector->subterms()[n])) == typeid(VarTerm));
			Variable* leafvar = *(pf->subterms()[n]->freeVars().begin());
			Variable* connectvar = *(leafconnector->subterms()[n]->freeVars().begin());
			if(lcd._leaftoconnector.find(leafvar) == lcd._leaftoconnector.end()) {
				lcd._leaftoconnector[leafvar] = connectvar;
				lcd._connectortoleaf[connectvar] = leafvar;
			}
			else {
				FOPropDomain* temp = lcd._equalities;
				VarTerm* vt1 = new VarTerm(connectvar,TermParseInfo());
				VarTerm* vt2 = new VarTerm(lcd._leaftoconnector[leafvar],TermParseInfo());
				EqChainForm* ecf = new EqChainForm(SIGN::POS,true,vt1,FormulaParseInfo());
				ecf->add(CompType::EQ,vt2);
				FOPropDomain* eq = _propagator->_factory->formuladomain(ecf);
				ecf->recursiveDelete();
				lcd._equalities = _propagator->_factory->conjunction(lcd._equalities,eq);
				delete(temp); delete(eq);
			}
			if(leafvar->sort() != connectvar->sort()) {
				VarTerm* vt = new VarTerm(connectvar,TermParseInfo());
				PredForm* as = new PredForm(SIGN::POS,leafvar->sort()->pred(),vector<Term*>(1,vt),FormulaParseInfo());
				FOPropDomain* asd = _propagator->_factory->formuladomain(as);
				as->recursiveDelete();
				FOPropDomain* temp = lcd._equalities;
				lcd._equalities = _propagator->_factory->conjunction(lcd._equalities,asd);
				delete(temp); delete(asd);
			}
		}
		_propagator->_leafconnectdata[pf] = lcd;
		_propagator->schedule(pf,UP,true,leafconnector);
		_propagator->schedule(pf,UP,false,leafconnector);
	}
}

void FOPropagatorFactory::visit(const AggForm* af) {
	SetExpr* s = af->right()->set();
	for(vector<Formula*>::const_iterator it = s->subformulas().begin(); it != s->subformulas().end(); ++it) {
		_propagator->_upward[*it] = af;
	}
	initFalse(af);
	traverse(af);
}

void FOPropagatorFactory::visit(const EqChainForm* ) {
	assert(false);
}

void FOPropagatorFactory::visit(const EquivForm* ef) {
	_propagator->_upward[ef->left()] = ef;
	_propagator->_upward[ef->right()] = ef;
	set<Variable*> leftqv = ef->freeVars();
	for(set<Variable*>::const_iterator it = ef->left()->freeVars().begin(); it != ef->left()->freeVars().end(); ++it) {
		leftqv.erase(*it);
	}
	set<Variable*> rightqv = ef->freeVars();
	for(set<Variable*>::const_iterator it = ef->right()->freeVars().begin(); it != ef->right()->freeVars().end(); ++it) {
		rightqv.erase(*it);
	}
	_propagator->_quantvars[ef->left()] = leftqv;
	_propagator->_quantvars[ef->right()] = rightqv;
	initFalse(ef);
	traverse(ef);
}

void FOPropagatorFactory::visit(const BoolForm* bf) {
	for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
		_propagator->_upward[*it] = bf;
		set<Variable*> sv = bf->freeVars();
		for(set<Variable*>::const_iterator jt = (*it)->freeVars().begin(); jt != (*it)->freeVars().end(); ++jt) {
			sv.erase(*jt);
		}
		_propagator->_quantvars[*it] = sv;
	}
	initFalse(bf);
	traverse(bf);
}

void FOPropagatorFactory::visit(const QuantForm* qf) {
	_propagator->_upward[qf->subformula()] = qf;
	initFalse(qf);
	traverse(qf);
}
