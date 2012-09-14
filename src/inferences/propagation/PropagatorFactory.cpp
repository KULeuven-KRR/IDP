/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "PropagatorFactory.hpp"

#include "Propagator.hpp"
#include "PropagationDomainFactory.hpp"
#include "PropagationScheduler.hpp"
#include "theory/TheoryUtils.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "GenerateBDDAccordingToBounds.hpp"

using namespace std;

typedef std::map<PFSymbol*, const FOBDD*> Bound;

GenerateBDDAccordingToBounds* generateBounds(AbstractTheory* theory, AbstractStructure*& structure, bool doSymbolicPropagation, bool applyToStructure) {
	Assert(theory != NULL);
	Assert(structure != NULL);
	auto mpi = propagateVocabulary(theory, structure);
	auto propagator = createPropagator(theory, structure, mpi);
	if (doSymbolicPropagation) {
		propagator->doPropagation();
	}
	if(applyToStructure){
		propagator->applyPropagationToStructure(structure);
	}
	auto result = propagator->symbolicstructure();
	delete (propagator);
	return result;
}

FOPropagator* createPropagator(AbstractTheory* theory, AbstractStructure*, const std::map<PFSymbol*, InitBoundType> mpi) {
//	if(getOption(BoolType::GROUNDWITHBOUNDS)){
	auto domainfactory = new FOPropBDDDomainFactory();
	auto scheduler = new FOPropScheduler();
	FOPropagatorFactory<FOPropBDDDomainFactory, FOPropBDDDomain> propfactory(domainfactory, scheduler, true, mpi);
	return propfactory.create(theory);
//	}else{
//		TODO notyetimplemented("Propagation without bdds.");
	/*auto domainfactory = new FOPropTableDomainFactory(s);
	 auto scheduler = new FOPropScheduler();
	 FOPropagatorFactory<FOPropTableDomainFactory, FOPropTableDomain> propfactory(domainfactory,scheduler,true,mpi);
	 return propfactory.create(theory);*/
//		return NULL;
//	}
}

/** Collect symbolic propagation vocabulary **/
std::map<PFSymbol*, InitBoundType> propagateVocabulary(AbstractTheory* theory, AbstractStructure* structure) {
	std::map<PFSymbol*, InitBoundType> mpi;
	Vocabulary* v = theory->vocabulary();
	for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
		auto spi = it->second->nonbuiltins();
		for (auto jt = spi.cbegin(); jt != spi.cend(); ++jt) {
			if (structure->vocabulary()->contains(*jt)) {
				PredInter* pinter = structure->inter(*jt);
				if (pinter->approxTwoValued()) {
					mpi[*jt] = IBT_TWOVAL;
				} else if (pinter->ct()->approxEmpty()) {
					if (pinter->cf()->approxEmpty()) {
						mpi[*jt] = IBT_NONE;
					} else {
						mpi[*jt] = IBT_CF;
					}
				} else if (pinter->cf()->approxEmpty()) {
					mpi[*jt] = IBT_CT;
				} else {
					mpi[*jt] = IBT_BOTH;
				}
			} else {
				mpi[*jt] = IBT_NONE;
			}
		}
	}
	for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
		auto sfi = it->second->nonbuiltins();
		for (auto jt = sfi.cbegin(); jt != sfi.cend(); ++jt) {
			if (structure->vocabulary()->contains(*jt)) {
				FuncInter* finter = structure->inter(*jt);
				if (finter->approxTwoValued()) {
					mpi[*jt] = IBT_TWOVAL;
				} else if (finter->graphInter()->ct()->approxEmpty()) {
					if (finter->graphInter()->cf()->approxEmpty()) {
						mpi[*jt] = IBT_NONE;
					} else {
						mpi[*jt] = IBT_CF;
					}
				} else if (finter->graphInter()->cf()->approxEmpty()) {
					mpi[*jt] = IBT_CT;
				} else {
					mpi[*jt] = IBT_BOTH;
				}
			} else {
				mpi[*jt] = IBT_NONE;
			}
		}
	}
	return mpi;
}

template<class InterpretationFactory, class PropDomain>
FOPropagatorFactory<InterpretationFactory, PropDomain>::FOPropagatorFactory(InterpretationFactory* factory, FOPropScheduler* scheduler, bool as,
		const map<PFSymbol*, InitBoundType>& init)
		: 	_initbounds(init),
			_assertsentences(as) {
	auto options = GlobalData::instance()->getOptions();
	_propagator = new TypedFOPropagator<InterpretationFactory, PropDomain>(factory, scheduler, options);
	_multiplymaxsteps = options->getValue(BoolType::RELATIVEPROPAGATIONSTEPS);
}

template<class InterpretationFactory, class PropDomain>
FOPropagatorFactory<InterpretationFactory, PropDomain>::~FOPropagatorFactory() {
	//deleteList(_leafconnectors); Do not delete connectors, they are passed to the propagator.
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::createleafconnector(PFSymbol* symbol) {
	if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 1) {
		clog << "  Creating a leaf connector for " << *symbol << "\n";
	}
	vector<Variable*> vars = VarUtils::makeNewVariables(symbol->sorts());
	vector<Term*> args = TermUtils::makeNewVarTerms(vars);
	PredForm* leafconnector = new PredForm(SIGN::POS, symbol, args, FormulaParseInfo());
	_leafconnectors[symbol] = leafconnector;
	_propagator->setLeafConnector(symbol, leafconnector);
	switch (_initbounds[symbol]) {
	case IBT_TWOVAL:
		_propagator->setDomain(leafconnector, ThreeValuedDomain<Domain>(_propagator->getFactory(), leafconnector));
		if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 1) {
			clog << "    The leaf connector is twovalued\n";
		}
		break;
	case IBT_BOTH:
	case IBT_CT:
	case IBT_CF:
		_propagator->setDomain(leafconnector, ThreeValuedDomain<Domain>(_propagator->getFactory(), leafconnector, _initbounds[symbol]));
		break;
	case IBT_NONE:
		initUnknown(leafconnector);
		if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 1) {
			clog << "    The leaf connector is completely unknown\n";
		}
		break;
	}
}

template<class Factory, class Domain>
TypedFOPropagator<Factory, Domain>* FOPropagatorFactory<Factory, Domain>::create(const AbstractTheory* theory) {
	if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 1) {
		clog << "=== initialize propagation datastructures\n";
	}

	// transform theory to a suitable normal form
	AbstractTheory* newtheo = theory->clone();
	FormulaUtils::addCompletion(newtheo);
	FormulaUtils::unnestTerms(newtheo, Context::POSITIVE, NULL, newtheo->vocabulary());
	FormulaUtils::unnestDomainTerms(newtheo);
	FormulaUtils::splitComparisonChains(newtheo);
	FormulaUtils::graphFuncsAndAggs(newtheo, NULL, false);

	// Add function constraints
	for (auto it = _initbounds.cbegin(); it != _initbounds.cend(); ++it) {
		if (it->second == IBT_TWOVAL || not isa<Function>(*(it->first))) {
			continue;
		}
		Function* function = dynamic_cast<Function*>(it->first);

		// Add  (! x : ? y : F(x) = y)
		//Here, we do it manually, we do not use the transformation since the propagation on the aggregates is not doing anything yet
		if (not function->partial()) {
			vector<Variable*> vars = VarUtils::makeNewVariables(function->sorts());
			vector<Term*> terms = TermUtils::makeNewVarTerms(vars);
			PredForm* atom = new PredForm(SIGN::POS, function, terms, FormulaParseInfo());
			Variable* y = vars.back();
			set<Variable*> yset = { y };
			QuantForm* exists = new QuantForm(SIGN::POS, QUANT::EXIST, yset, atom, FormulaParseInfo());
			vars.pop_back();
			set<Variable*> xset(vars.cbegin(), vars.cend());
			if (xset.size() == 0) {
				newtheo->add(exists);
			} else {
				QuantForm* univ1 = new QuantForm(SIGN::POS, QUANT::UNIV, xset, exists, FormulaParseInfo());
				newtheo->add(univ1);
			}
		}

		// Add	(! z y1 y2 : F(z) ~= y1 | F(z) ~= y2 | y1 = y2)
		vector<Variable*> zvars = VarUtils::makeNewVariables(function->insorts());
		Variable* y1var = new Variable(function->outsort());
		Variable* y2var = new Variable(function->outsort());
		vector<Variable*> zy1vars = zvars;
		zy1vars.push_back(y1var);
		vector<Variable*> zy2vars = zvars;
		zy2vars.push_back(y2var);
		vector<Variable*> y1y2vars;
		y1y2vars.push_back(y1var);
		y1y2vars.push_back(y2var);
		vector<Term*> zy1terms = TermUtils::makeNewVarTerms(zy1vars);
		vector<Term*> zy2terms = TermUtils::makeNewVarTerms(zy2vars);
		vector<Term*> y1y2terms = TermUtils::makeNewVarTerms(y1y2vars);
		vector<Formula*> atoms;
		atoms.push_back(new PredForm(SIGN::NEG, function, zy1terms, FormulaParseInfo()));
		atoms.push_back(new PredForm(SIGN::NEG, function, zy2terms, FormulaParseInfo()));
		atoms.push_back(new PredForm(SIGN::POS, get(STDPRED::EQ, function->outsort()), y1y2terms, FormulaParseInfo()));
		BoolForm* disjunction = new BoolForm(SIGN::POS, false, atoms, FormulaParseInfo());
		set<Variable*> zy1y2set;
		zy1y2set.insert(zvars.cbegin(), zvars.cend());
		zy1y2set.insert(y1var);
		zy1y2set.insert(y2var);
		QuantForm* univ2 = new QuantForm(SIGN::POS, QUANT::UNIV, zy1y2set, disjunction, FormulaParseInfo());
		newtheo->add(univ2);
	}
	//From now on, newtheo is the responsibility of _propagator
	_propagator->setTheory(newtheo);
	// Multiply maxsteps if requested
	if (_multiplymaxsteps) {
		_propagator->setMaxSteps(_propagator->getMaxSteps() * FormulaUtils::nrSubformulas(newtheo));
	}

	// create leafconnectors
	Vocabulary* voc = newtheo->vocabulary();
	for (auto it = voc->firstPred(); it != voc->lastPred(); ++it) {
		set<Predicate*> sp = it->second->nonbuiltins();
		for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
			createleafconnector(*jt);
		}
	}
	for (auto it = voc->firstFunc(); it != voc->lastFunc(); ++it) {
		set<Function*> sf = it->second->nonbuiltins();
		for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
			createleafconnector(*jt);
		}
	}

	// visit sentences
	newtheo->accept(this);
	return _propagator;
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const Theory* theory) {
	for (size_t n = 0; n < theory->sentences().size(); ++n) {
		Formula* sentence = theory->sentences()[n];
		if (_assertsentences) {
			_propagator->schedule(sentence, DOWN, true, 0);
			//What is the meaning of the true and false?  They determine to choose which domain?
			_propagator->setDomain(sentence, ThreeValuedDomain<Domain>(_propagator->getFactory(), true, false, sentence));
		}
		sentence->accept(this);
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::initUnknown(const Formula* f) {
	if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 2) {
		clog << "  Assigning the least precise bounds to " << *f << "\n";
	}
	if (not _propagator->hasDomain(f)) {
		_propagator->setDomain(f, ThreeValuedDomain<Domain>(_propagator->getFactory(), false, false, f));
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::initTwoVal(const Formula* f) {
	if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 2) {
		clog << "  Assigning a two-valued domain to " << *f << "\n";
	}
	if (not _propagator->hasDomain(f)) {
		_propagator->setDomain(f, ThreeValuedDomain<Domain>(_propagator->getFactory(), f));
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const PredForm* pf) {
	initUnknown(pf);
	PFSymbol* symbol = pf->symbol();
	if (symbol->builtin()) {
		auto it = _propagator->getUpward().find(pf);
		if (it != _propagator->getUpward().cend()) {
			Assert(it->second!=NULL);
			auto formulaDomain = ThreeValuedDomain<Domain>(_propagator->getFactory(), pf);
			_propagator->setDomain(pf, formulaDomain);
			if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 2) {
				clog << "  " << toString(pf) << " is builtin. Updated its domain to" << "\n" << toString(formulaDomain)<<nt();
			}
			_propagator->schedule(it->second, UP, true, pf);
			_propagator->schedule(it->second, UP, false, pf);
		}
	} else {
#ifndef NDEBUG
		if (_leafconnectors.find(symbol) == _leafconnectors.cend()) {
			clog << toString(symbol);
		}
		Assert(_leafconnectors.find(symbol) != _leafconnectors.cend());
#endif
		PredForm* leafconnector = _leafconnectors[symbol];
		_propagator->addToLeafUpward(leafconnector, pf);
		LeafConnectData<Domain> lcd;
		lcd._connector = leafconnector;
		lcd._equalities = _propagator->getFactory()->trueDomain(leafconnector);
		for (unsigned int n = 0; n < symbol->sorts().size(); ++n) {
			Assert(typeid(*(pf->subterms()[n])) == typeid(VarTerm));
			Assert(typeid(*(leafconnector->subterms()[n])) == typeid(VarTerm));
			Variable* leafvar = *(pf->subterms()[n]->freeVars().cbegin());
			Variable* connectvar = *(leafconnector->subterms()[n]->freeVars().cbegin());
			if (lcd._leaftoconnector.find(leafvar) == lcd._leaftoconnector.cend()) {
				lcd._leaftoconnector[leafvar] = connectvar;
				lcd._connectortoleaf[connectvar] = leafvar;
			} else {
				FOPropDomain* temp = lcd._equalities;
				VarTerm* vt1 = new VarTerm(connectvar, TermParseInfo());
				VarTerm* vt2 = new VarTerm(lcd._leaftoconnector[leafvar], TermParseInfo());
				EqChainForm* ecf = new EqChainForm(SIGN::POS, true, vt1, FormulaParseInfo());
				ecf->add(CompType::EQ, vt2);
				Domain* eq = _propagator->getFactory()->formuladomain(ecf);
				ecf->recursiveDelete();
				lcd._equalities = _propagator->getFactory()->conjunction(lcd._equalities, eq);
				delete (temp);
				delete (eq);
			}
			if (leafvar->sort() != connectvar->sort()) {
				VarTerm* vt = new VarTerm(connectvar, TermParseInfo());
				PredForm* as = new PredForm(SIGN::POS, leafvar->sort()->pred(), vector<Term*>(1, vt), FormulaParseInfo());
				Domain* asd = _propagator->getFactory()->formuladomain(as);
				FOPropDomain* temp = lcd._equalities;
				lcd._equalities = _propagator->getFactory()->conjunction(lcd._equalities, asd);
				as->recursiveDelete();
				delete (temp);
				delete (asd);
			}
		}
		_propagator->setLeafConnectData(pf, lcd);
		_propagator->schedule(pf, UP, true, leafconnector);
		_propagator->schedule(pf, UP, false, leafconnector);
	}
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const AggForm* af) {
	auto set = af->getAggTerm()->set();
	for (auto i = set->getSets().cbegin(); i < set->getSets().cend(); ++i) {
		_propagator->setUpward((*i)->getCondition(), af);
	}
	initUnknown(af);
	traverse(af);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const EqChainForm*) {
	throw notyetimplemented("Creating a propagator for comparison chains");
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const EquivForm* ef) {
	_propagator->setUpward(ef->left(), ef);
	_propagator->setUpward(ef->right(), ef);
	set<Variable*> leftqv = ef->freeVars();
	for (auto it = ef->left()->freeVars().cbegin(); it != ef->left()->freeVars().cend(); ++it) {
		leftqv.erase(*it);
	}
	set<Variable*> rightqv = ef->freeVars();
	for (auto it = ef->right()->freeVars().cbegin(); it != ef->right()->freeVars().cend(); ++it) {
		rightqv.erase(*it);
	}
	_propagator->setQuantVar(ef->left(), leftqv); //SetQuantVar?  Looks more like setNonFreeVars to me! Or better: freevars of the superformula that do not appear in the subf
	_propagator->setQuantVar(ef->right(), rightqv); //SetQuantVar?  Looks more like setNonFreeVars to me!
	initUnknown(ef);
	traverse(ef);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const BoolForm* bf) {
	if(bf->subformulas().size() == 0){
		initTwoVal(bf);
		return;
	}
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		_propagator->setUpward(*it, bf);
		set<Variable*> sv = bf->freeVars();
		for (auto jt = (*it)->freeVars().cbegin(); jt != (*it)->freeVars().cend(); ++jt) {
			sv.erase(*jt);
		}
		_propagator->setQuantVar(*it, sv); //SetQuantVar?  Looks more like setNonFreeVars to me!
	}
	initUnknown(bf);
	traverse(bf);
}

template<class Factory, class Domain>
void FOPropagatorFactory<Factory, Domain>::visit(const QuantForm* qf) {
	_propagator->setUpward(qf->subformula(), qf);
	initUnknown(qf);
	traverse(qf);
}
