#include "PropagatorFactory.hpp"

#include <iostream>
#include "common.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "propagate.hpp"
#include "GlobalData.hpp"
#include "utils/TheoryUtils.hpp"

using namespace std;

FOPropagator* createPropagator(AbstractTheory* theory, const std::map<PFSymbol*,InitBoundType> mpi) {
	auto domainfactory = new FOPropBDDDomainFactory();
	auto scheduler = new FOPropScheduler();
	FOPropagatorFactory<FOPropBDDDomainFactory, FOPropBDDDomain> propfactory(domainfactory,scheduler,true,mpi);
	auto propagator = propfactory.create(theory);
	return propagator;
}

template<class InterpretationFactory, class PropDomain>
FOPropagatorFactory<InterpretationFactory, PropDomain>::FOPropagatorFactory(InterpretationFactory* factory, FOPropScheduler* scheduler, bool as, const map<PFSymbol*,InitBoundType>& init)
	: _verbosity(GlobalData::instance()->getOptions()->getValue(IntType::PROPAGATEVERBOSITY)), _initbounds(init), _assertsentences(as) {
	auto options = GlobalData::instance()->getOptions();
	_propagator = new TypedFOPropagator<InterpretationFactory, PropDomain>(factory, scheduler, options);
	_multiplymaxsteps = options->getValue(BoolType::RELATIVEPROPAGATIONSTEPS);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::createleafconnector(PFSymbol* symbol) {
	if(_verbosity > 1) { cerr << "  Creating a leaf connector for " << *symbol << "\n";	}
	vector<Variable*> vars = VarUtils::makeNewVariables(symbol->sorts());
	vector<Term*> args = TermUtils::makeNewVarTerms(vars);
	PredForm* leafconnector = new PredForm(SIGN::POS,symbol,args,FormulaParseInfo());
	_leafconnectors[symbol] = leafconnector;
	_propagator->setLeafConnector(symbol, leafconnector);
	switch(_initbounds[symbol]) {
		case IBT_TWOVAL:
			_propagator->setDomain(leafconnector, ThreeValuedDomain<Domain>(_propagator->getFactory(),leafconnector));
			if(_verbosity > 1) { cerr << "    The leaf connector is twovalued\n";	}
			break;
		case IBT_BOTH:
		case IBT_CT:
		case IBT_CF:
			_propagator->setDomain(leafconnector, ThreeValuedDomain<Domain>(_propagator->getFactory(),leafconnector,_initbounds[symbol]));
			break;
		case IBT_NONE:
			initFalse(leafconnector);
			if(_verbosity > 1) { cerr << "    The leaf connector is completely unknown\n";	}
			break;
	}
}

template<class Factory, class Domain>
TypedFOPropagator<Factory, Domain>* FOPropagatorFactory<Factory, Domain>::create(const AbstractTheory* theory) {
	if(_verbosity > 1) { cerr << "=== initialize propagation datastructures\n";	}

	// transform theory to a suitable normal form
	AbstractTheory* newtheo = theory->clone();
	FormulaUtils::addCompletion(newtheo);
	FormulaUtils::unnestTerms(newtheo);	// FIXME: remove nesting does not change F(x)=y to F(x,y) anymore, which is probably needed here
	FormulaUtils::graphFunctions(newtheo);
	FormulaUtils::graphAggregates(newtheo);
	FormulaUtils::splitComparisonChains(newtheo);

	// Add function constraints
	for(auto it = _initbounds.cbegin(); it != _initbounds.cend(); ++it) {
		if(it->second == IBT_TWOVAL || not sametypeid<Function>(*(it->first))) {
			continue;
		}
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
		xset.insert(vars.cbegin(),vars.cend());
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
		zy1y2set.insert(zvars.cbegin(),zvars.cend());
		zy1y2set.insert(y1var);
		zy1y2set.insert(y2var);
		QuantForm* univ2 = new QuantForm(SIGN::POS,QUANT::UNIV,zy1y2set,disjunction,FormulaParseInfo());
		newtheo->add(univ2);
	}

	// Multiply maxsteps if requested
	if(_multiplymaxsteps) _propagator->setMaxSteps(_propagator->getMaxSteps() * FormulaUtils::nrSubformulas(newtheo));

	// create leafconnectors
	Vocabulary* voc = newtheo->vocabulary();
	for(auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for(auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
			createleafconnector(*jt);
		}
	}
	for(auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
		set<Function*> sf = it->second->nonbuiltins();
		for(auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
			createleafconnector(*jt);
		}
	}

	// visit sentences
	newtheo->accept(this);

	return _propagator;
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const Theory* theory) {
	for(size_t n = 0; n < theory->sentences().size(); ++n) {
		Formula* sentence = theory->sentences()[n];
		if(_assertsentences) {
			_propagator->schedule(sentence,DOWN,true,0);
			_propagator->setDomain(sentence, ThreeValuedDomain<Domain>(_propagator->getFactory(),true,false,sentence));
		}
		sentence->accept(this);
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::initFalse(const Formula* f) {
	if(_verbosity > 2) { cerr << "  Assigning the least precise bounds to " << *f << "\n";	}
	if(not _propagator->hasDomain(f)) {
		_propagator->setDomain(f, ThreeValuedDomain<Domain>(_propagator->getFactory(),false,false,f));
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const PredForm* pf) {
	initFalse(pf);
	PFSymbol* symbol = pf->symbol();
	if(symbol->builtin()) {
		auto it = _propagator->getUpward().find(pf);
		if(it != _propagator->getUpward().cend()) {
			assert(it->second!=NULL);
			_propagator->setDomain(pf, ThreeValuedDomain<Domain>(_propagator->getFactory(),pf));
			_propagator->schedule(it->second,UP,true,pf);
			_propagator->schedule(it->second,UP,false,pf);
		}
	}
	else {
		assert(_leafconnectors.find(symbol) != _leafconnectors.cend());
		PredForm* leafconnector = _leafconnectors[symbol];
		_propagator->addToLeafUpward(leafconnector, pf);
		LeafConnectData<Domain> lcd;
		lcd._connector = leafconnector;
		lcd._equalities = _propagator->getFactory()->trueDomain(leafconnector);
		for(unsigned int n = 0; n < symbol->sorts().size(); ++n) {
			assert(typeid(*(pf->subterms()[n])) == typeid(VarTerm));
			assert(typeid(*(leafconnector->subterms()[n])) == typeid(VarTerm));
			Variable* leafvar = *(pf->subterms()[n]->freeVars().cbegin());
			Variable* connectvar = *(leafconnector->subterms()[n]->freeVars().cbegin());
			if(lcd._leaftoconnector.find(leafvar) == lcd._leaftoconnector.cend()) {
				lcd._leaftoconnector[leafvar] = connectvar;
				lcd._connectortoleaf[connectvar] = leafvar;
			}
			else {
				FOPropDomain* temp = lcd._equalities;
				VarTerm* vt1 = new VarTerm(connectvar,TermParseInfo());
				VarTerm* vt2 = new VarTerm(lcd._leaftoconnector[leafvar],TermParseInfo());
				EqChainForm* ecf = new EqChainForm(SIGN::POS,true,vt1,FormulaParseInfo());
				ecf->add(CompType::EQ,vt2);
				Domain* eq = _propagator->getFactory()->formuladomain(ecf);
				ecf->recursiveDelete();
				lcd._equalities = _propagator->getFactory()->conjunction(lcd._equalities,eq);
				delete(temp); delete(eq);
			}
			if(leafvar->sort() != connectvar->sort()) {
				VarTerm* vt = new VarTerm(connectvar,TermParseInfo());
				PredForm* as = new PredForm(SIGN::POS,leafvar->sort()->pred(),vector<Term*>(1,vt),FormulaParseInfo());
				Domain* asd = _propagator->getFactory()->formuladomain(as);
				as->recursiveDelete();
				FOPropDomain* temp = lcd._equalities;
				lcd._equalities = _propagator->getFactory()->conjunction(lcd._equalities,asd);
				delete(temp); delete(asd);
			}
		}
		_propagator->setLeafConnectData(pf, lcd);
		_propagator->schedule(pf,UP,true,leafconnector);
		_propagator->schedule(pf,UP,false,leafconnector);
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const AggForm* af) {
	SetExpr* s = af->right()->set();
	for(auto it = s->subformulas().cbegin(); it != s->subformulas().cend(); ++it) {
		_propagator->setUpward(*it, af);
	}
	initFalse(af);
	traverse(af);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const EqChainForm* ) {
	assert(false);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const EquivForm* ef) {
	_propagator->setUpward(ef->left(), ef);
	_propagator->setUpward(ef->right(), ef);
	set<Variable*> leftqv = ef->freeVars();
	for(auto it = ef->left()->freeVars().cbegin(); it != ef->left()->freeVars().cend(); ++it) {
		leftqv.erase(*it);
	}
	set<Variable*> rightqv = ef->freeVars();
	for(auto it = ef->right()->freeVars().cbegin(); it != ef->right()->freeVars().cend(); ++it) {
		rightqv.erase(*it);
	}
	_propagator->setQuantVar(ef->left(), leftqv);
	_propagator->setQuantVar(ef->right(), rightqv);
	initFalse(ef);
	traverse(ef);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const BoolForm* bf) {
	for(auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		_propagator->setUpward(*it, bf);
		set<Variable*> sv = bf->freeVars();
		for(auto jt = (*it)->freeVars().cbegin(); jt != (*it)->freeVars().cend(); ++jt) {
			sv.erase(*jt);
		}
		_propagator->setQuantVar(*it, sv);
	}
	initFalse(bf);
	traverse(bf);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const QuantForm* qf) {
	_propagator->setUpward(qf->subformula(), qf);
	initFalse(qf);
	traverse(qf);
}
