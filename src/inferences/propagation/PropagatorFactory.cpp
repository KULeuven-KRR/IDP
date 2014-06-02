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

FOPropagator* createPropagator(const AbstractTheory* theory, const Structure* structure, const std::map<PFSymbol*, InitBoundType> mpi) {
	auto domainfactory = new FOPropBDDDomainFactory();
	auto scheduler = new FOPropScheduler();
	FOPropagatorFactory<FOPropBDDDomainFactory, FOPropBDDDomain> propfactory(domainfactory, scheduler, true, mpi);
	return propfactory.create(theory, structure);
}

std::shared_ptr<GenerateBDDAccordingToBounds> generateNonLiftedBounds(AbstractTheory* theory, Structure const * const structure) {
	Assert(theory != NULL);
	Assert(structure != NULL);
	auto mpi = propagateVocabulary(theory, structure);
	auto propagator = createPropagator(theory, structure, mpi);
	auto result = propagator->symbolicstructure(theory->vocabulary());
	delete (propagator);
	return result;
}

shared_ptr<GenerateBDDAccordingToBounds> generateBounds(AbstractTheory* theory, Structure* structure, bool doSymbolicPropagation, bool LUP,
		Vocabulary* outputvoc) {
	Assert(theory != NULL);
	Assert(structure != NULL);
	auto mpi = propagateVocabulary(theory, structure);
	auto propagator = createPropagator(theory, structure, mpi);
	if (doSymbolicPropagation) { // Strange, this should be called LUP
		propagator->doPropagation();
	if (LUP) {
			if (getOption(IntType::VERBOSE_GROUNDING) >= 1) {
				clog <<"Applying propagation to structure\n";
			}
			// FIXME not applying propagation after approximation is still bugged
//			if(getOption(SATISFIABILITYDELAY)){
//				propagator->applyPropagationToStructure(structure, new Vocabulary("Temp"));
//			}else{
				propagator->applyPropagationToStructure(structure, outputvoc!=NULL?*outputvoc:*structure->vocabulary());
//			}
		}
	}

	// We ONLY want to replace atoms by their BDDs IF
	//  * We did not yet propagate ALL information
	//  * BUT, we are sure that we propagated ENOUGH information to the structure to be sure that the outputvoc is correct.
	Vocabulary* symbolsThatShouldNotBeReplacedByBDDs = NULL;
	if(LUP){
//		if(not getOption(SATISFIABILITYDELAY)){
			if(outputvoc==NULL){
				symbolsThatShouldNotBeReplacedByBDDs = theory->vocabulary();
			}else{
				symbolsThatShouldNotBeReplacedByBDDs = new Vocabulary("Temp");
			}
//		}
	}else{
		symbolsThatShouldNotBeReplacedByBDDs = theory->vocabulary();
	}
	auto result = propagator->symbolicstructure(symbolsThatShouldNotBeReplacedByBDDs);
	delete (propagator);
	return result;
}

void propagateSymbol(Structure const * const structure, PFSymbol* symbol, std::map<PFSymbol*, InitBoundType>& mpi) {
	if (structure->vocabulary()->contains(symbol)) {
		PredInter* pinter = structure->inter(symbol);
		if (pinter->approxTwoValued()) {
			mpi[symbol] = IBT_TWOVAL;
		} else if (pinter->ct()->approxEmpty()) {
			if (pinter->cf()->approxEmpty()) {
				mpi[symbol] = IBT_NONE;
			} else {
				mpi[symbol] = IBT_CF;
			}
		} else if (pinter->cf()->approxEmpty()) {
			mpi[symbol] = IBT_CT;
		} else {
			mpi[symbol] = IBT_BOTH;
		}
	} else {
		mpi[symbol] = IBT_NONE;
	}
}

/** Collect symbolic propagation vocabulary **/
std::map<PFSymbol*, InitBoundType> propagateVocabulary(AbstractTheory* theory, Structure const * const structure) {
	std::map<PFSymbol*, InitBoundType> mpi;
	Vocabulary* v = theory->vocabulary();
	for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
		auto spi = it->second->nonbuiltins();
		for (auto jt = spi.cbegin(); jt != spi.cend(); ++jt) {
			propagateSymbol(structure, *jt, mpi);
		}
	}
	for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
		auto sfi = it->second->nonbuiltins();
		for (auto jt = sfi.cbegin(); jt != sfi.cend(); ++jt) {
			propagateSymbol(structure, *jt, mpi);
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
TypedFOPropagator<Factory, Domain>* FOPropagatorFactory<Factory, Domain>::create(const AbstractTheory* theory, const Structure* structure) {
	if (getOption(IntType::VERBOSE_CREATE_PROPAGATORS) > 1) {
		clog << "=== initialize propagation datastructures\n";
	}
	if (structure->vocabulary() != theory->vocabulary()) {
		throw IdpException("Approximation requires that the theory and structure range over the same vocabulary.");
	}

	// transform theory to a suitable normal form
	AbstractTheory* newtheo = theory->clone();
	FormulaUtils::replaceDefinitionsWithCompletion(newtheo, structure);
	FormulaUtils::unnestTerms(newtheo, structure, newtheo->vocabulary());
	FormulaUtils::splitComparisonChains(newtheo);
	FormulaUtils::graphFuncsAndAggs(newtheo, NULL, {}, true, false);
	newtheo = FormulaUtils::pushQuantifiersAndNegations(newtheo);
	FormulaUtils::flatten(newtheo);
	/* Since we will create "leafconnectors" for all (non-builtin) predicates, it is important that we unnest
	 * all terms to achieve that predforms are always of the form P(\bar x)
	 * All terms should have been unnested before, we only need to unnest domainterms now.
	 *
	 * Since we don't create leafconnectors for built-in predicates, we only unnest from non builtins */
	FormulaUtils::unnestDomainTermsFromNonBuiltins(newtheo);

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
			varset yset = { y };
			QuantForm* exists = new QuantForm(SIGN::POS, QUANT::EXIST, yset, atom, FormulaParseInfo());
			vars.pop_back();
			varset xset(vars.cbegin(), vars.cend());
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
		varset zy1y2set;
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
				clog << "  " << print(pf) << " is builtin. Updated its domain to" << "\n" << print(formulaDomain)<<nt();
			}
			_propagator->schedule(it->second, UP, true, pf);
			_propagator->schedule(it->second, UP, false, pf);
		}
	} else {
		auto lc = _leafconnectors.find(symbol);
		if (lc == _leafconnectors.cend()) {
			stringstream ss;
			ss << "Internal error in approximation: symbol " << toString(symbol)
					<< "occurs in theory unexpectedly. Please report this bug to krr@cs.kuleuven.be";
			throw IdpException(ss.str());
		}
		PredForm* leafconnector = lc->second;
		_propagator->addToLeafUpward(leafconnector, pf);
		LeafConnectData<Domain> lcd;
		lcd._connector = leafconnector;
		lcd._equalities = _propagator->getFactory()->trueDomain(leafconnector);
		for (unsigned int n = 0; n < symbol->sorts().size(); ++n) {
			Assert(typeid(*(pf->subterms()[n])) == typeid(VarTerm));
			Assert(typeid(*(leafconnector->subterms()[n])) == typeid(VarTerm));
			Variable* leafvar = *(pf->subterms()[n]->freeVars().cbegin());
			Variable* connectvar = *(leafconnector->subterms()[n]->freeVars().cbegin());
			lcd._connectortoleaf[connectvar] = leafvar;
			if (lcd._leaftoconnector.find(leafvar) == lcd._leaftoconnector.cend()) {
				lcd._leaftoconnector[leafvar] = connectvar;
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
	auto leftqv = ef->freeVars();
	for (auto it = ef->left()->freeVars().cbegin(); it != ef->left()->freeVars().cend(); ++it) {
		leftqv.erase(*it);
	}
	auto rightqv = ef->freeVars();
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
	if (bf->subformulas().size() == 0) {
		initTwoVal(bf);
		return;
	}
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		_propagator->setUpward(*it, bf);
		auto sv = bf->freeVars();
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
