/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddAggTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddAggKernel.hpp"
#include "BDDBasedGeneratorFactory.hpp"
#include "InstGenerator.hpp"
#include "SimpleFuncGenerator.hpp"
#include "TreeInstGenerator.hpp"
#include "UnionGenerator.hpp"
#include "SortInstGenerator.hpp"
#include "LookupGenerator.hpp"
#include "EnumLookupGenerator.hpp"
#include "SortLookupGenerator.hpp"
#include "BasicGenerators.hpp"
#include "AggregateGenerator.hpp"
#include "ComparisonGenerator.hpp"
#include "GeneratorFactory.hpp"
#include "FalseQuantKernelGenerator.hpp"
#include "TrueQuantKernelGenerator.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

/*
 * Tries to rewrite the given predform with var in the right hand side. Or -var in the righthandside (depending on bool invert)
 * If it is not possible to rewrite it this way, this returns NULL
 */
Term* solve(FOBDDManager& manager, PredForm* atom, Variable* var, bool invert) {
	FOBDDFactory factory(&manager);
	auto bdd = factory.turnIntoBdd(atom);
	Assert(not manager.isTruebdd(bdd));
	Assert(not manager.isFalsebdd(bdd));
	auto kernel = bdd->kernel();
	const FOBDDTerm* arg = manager.getVariable(var);
	if (invert) {
		if (not SortUtils::isSubsort(arg->sort(), VocabularyUtils::floatsort())) {
			//We only do arithmetic on float and subsorts
			return NULL;
		}
		const DomainElement* minus_one = createDomElem(-1);
		const FOBDDTerm* minus_one_term = manager.getDomainTerm(VocabularyUtils::intsort(), minus_one);
		Function* times = Vocabulary::std()->func("*/2");
		times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(VocabularyUtils::intsort(), arg->sort())), 0);
		vector<const FOBDDTerm*> timesterms(2);
		timesterms[0] = minus_one_term;
		timesterms[1] = arg;
		arg = manager.getFuncTerm(times, timesterms);
	}
	auto rewrittenarg = manager.solve(kernel, arg);
	if (rewrittenarg != NULL) {
		return manager.toTerm(rewrittenarg);
	} else {
		return NULL;
	}
}

/*
 * Returns a vector with at position i the first occurence of vars[i] in vars
 * For example (a,b,b,a) would return (0,1,1,0)
 */
vector<unsigned int> getFirstOccurences(const vector<const DomElemContainer*>& vars) {
	vector<unsigned int> firstocc;
	for (unsigned int n = 0; n < vars.size(); ++n) {
		firstocc.push_back(n);
		for (unsigned int m = 0; m < n; ++m) {
			if (vars[n] == vars[m]) {
				firstocc[n] = m;
				break;
			}
		}
	}
	return firstocc;
}

/*
 * Extracts first occurences.  For example (a,b,b,a) with (IN,OUT,OUT,IN) would be changed to (b).
 * Also changes tables to be compatible with this.
 * This method assumes outvars and tables to be empty!
 */
void extractFirstOccurringOutputs(const BddGeneratorData& data, const vector<unsigned int>& firstocc, vector<const DomElemContainer*>& outvars,
		vector<SortTable*>& tables) {
	Assert(outvars.empty());
	Assert(tables.empty());
	for (unsigned int n = 0; n < data.pattern.size(); ++n) {
		if (data.pattern[n] == Pattern::OUTPUT) {
			if (firstocc[n] == n) {
				outvars.push_back(data.vars[n]);
				tables.push_back(data.universe.tables()[n]);
			}
		}
	}
}

BDDToGenerator::BDDToGenerator(FOBDDManager* manager)
		: _manager(manager) {
}

InstGenerator* BDDToGenerator::create(const BddGeneratorData& data) {
	Assert(data.check());
	if (data.bdd == _manager->falsebdd()) {
		return new EmptyGenerator();
	}

	if (data.bdd == _manager->truebdd()) {
		vector<unsigned int> firstocc = getFirstOccurences(data.vars);
		vector<const DomElemContainer*> outvars;
		vector<SortTable*> tables;
		extractFirstOccurringOutputs(data, firstocc, outvars, tables);
		return GeneratorFactory::create(outvars, tables);
	}

	return new TreeInstGenerator(createnode(data));
}

GeneratorNode* BDDToGenerator::createnode(const BddGeneratorData& data) {
	Assert(data.check());

	//The base-cases are already covered in BDDToGenerator::create
	if (data.bdd == _manager->falsebdd() || data.bdd == _manager->truebdd()) {
		return new LeafGeneratorNode(create(data));
	}

	// Otherwise: recursive case

	// Copy data to branchdata.  This data will be used for one of the branches.
	// It is practically the same as data except for the pattern (since the kernel might set some variables)
	BddGeneratorData branchdata = data;
	branchdata.pattern.clear();

	// split variables into kernel and branch and also extract first-occurring kernel output variables
	vector<Pattern> kernpattern;
	vector<const DomElemContainer*> kernvars, kernoutputvars;
	vector<const FOBDDVariable*> kernbddvars;
	vector<SortTable*> kerntables, kernoutputtables;
	for (unsigned int n = 0; n < data.pattern.size(); ++n) {
		if (not _manager->contains(data.bdd->kernel(), data.bddvars[n])) {
			//Variable does not appear in kernel
			branchdata.pattern.push_back(data.pattern[n]);
			continue;
		}

		//Variable appears in kernel
		kernpattern.push_back(data.pattern[n]);
		kernvars.push_back(data.vars[n]);
		kernbddvars.push_back(data.bddvars[n]);
		kerntables.push_back(data.universe.tables()[n]);

		//Extract first occuring output
		if (data.pattern[n] == Pattern::OUTPUT) {
			bool firstocc = true;
			for (unsigned int m = 0; m < kernoutputvars.size(); ++m) {
				if (kernoutputvars[m] == kernvars.back()) {
					firstocc = false;
					break;
				}
			}
			if (firstocc) {
				kernoutputvars.push_back(data.vars[n]);
				kernoutputtables.push_back(data.universe.tables()[n]);
			}
		}

		//If a variable appears in the kernel, it is input for the branches
		branchdata.pattern.push_back(Pattern::INPUT);
	}

	if (data.bdd->falsebranch() == _manager->falsebdd()) {
		// Only generate the true branch possibilities
		branchdata.bdd = data.bdd->truebranch();
		auto kernelgenerator = createFromKernel(data.bdd->kernel(), kernpattern, kernvars, kernbddvars, data.structure, BRANCH::TRUEBRANCH,
				Universe(kerntables));
		auto truegenerator = createnode(branchdata);
		return new OneChildGeneratorNode(kernelgenerator, truegenerator);
	}

	if (data.bdd->truebranch() == _manager->falsebdd()) {
		// Only generate the false branch possibilities
		branchdata.bdd = data.bdd->falsebranch();
		auto kernelgenerator = createFromKernel(data.bdd->kernel(), kernpattern, kernvars, kernbddvars, data.structure, BRANCH::FALSEBRANCH,
				Universe(kerntables));
		auto falsegenerator = createnode(branchdata);
		return new OneChildGeneratorNode(kernelgenerator, falsegenerator);
	}

	if (data.bdd->truebranch() == _manager->truebdd()) {
		//Avoid creating a twochildgeneratornode (too expensive since it has a univgenerator)
		branchdata.bdd = data.bdd->falsebranch();
		branchdata.pattern = data.pattern;
		auto kernelgenerator = createFromKernel(data.bdd->kernel(), kernpattern, kernvars, kernbddvars, data.structure, BRANCH::TRUEBRANCH,
				Universe(kerntables));
		auto falsegenerator = createnode(branchdata);
		std::vector<InstGenerator*> vec(2);
		vec[0] = kernelgenerator;
		vec[1] = new TreeInstGenerator(falsegenerator);
		return new LeafGeneratorNode(new UnionGenerator(vec));
	}

	// Both branches possible: create a checker and a generator for all possibilities
	vector<Pattern> checkerpattern(kernpattern.size(), Pattern::INPUT);
	auto kernelchecker = createFromKernel(data.bdd->kernel(), checkerpattern, kernvars, kernbddvars, data.structure, BRANCH::TRUEBRANCH, Universe(kerntables));

	//Generator for the universe of kerneloutput
	auto kernelgenerator = GeneratorFactory::create(kernoutputvars, kernoutputtables);

	branchdata.bdd = data.bdd->falsebranch();
	auto falsegenerator = createnode(branchdata);

	branchdata.bdd = data.bdd->truebranch();
	auto truegenerator = createnode(branchdata);

	return new TwoChildGeneratorNode(kernelchecker, kernelgenerator, falsegenerator, truegenerator);
}

/*
 * Given matchingPattern (input or output), try to rewrite atom as ... op x
 * with x a variable with the right pattern
 * If it is not possible, the original atom is returned
 */
PredForm* solveAndReplace(PredForm* atom, const vector<Pattern>& pattern, const vector<Variable*>& atomvars, FOBDDManager* manager, Pattern matchingPattern) {
	for (unsigned int n = 0; n < pattern.size(); ++n) {
		if (pattern[n] == matchingPattern) {
			auto solvedterm = solve(*manager, atom, atomvars[n], false);
			if (solvedterm != NULL) {
				auto varterm = new VarTerm(atomvars[n], TermParseInfo());
				PredForm* newatom = new PredForm(atom->sign(), atom->symbol(), { solvedterm, varterm }, atom->pi());
				delete (atom);
				return newatom;
			}
			auto invertedSolvedTerm = solve(*manager, atom, atomvars[n], true);
			if (invertedSolvedTerm != NULL) {
				auto varterm = new VarTerm(atomvars[n], TermParseInfo());
				PFSymbol* newsymbol;
				if (atom->symbol()->name() == ">/2") {
					newsymbol = VocabularyUtils::lessThan(atom->symbol()->sort(0));
				} else {
					// "=/2" should already have been handled by "solvedterm"
					Assert(atom->symbol()->name() == "</2");
					newsymbol = VocabularyUtils::greaterThan(atom->symbol()->sort(0));
				}
				if (invertedSolvedTerm->type() == TermType::TT_DOM) {
					auto solvedEl = domElemUmin(dynamic_cast<DomainTerm*>(invertedSolvedTerm)->value());
					solvedterm = new DomainTerm(SortUtils::resolve(VocabularyUtils::intsort(), invertedSolvedTerm->sort()), solvedEl, TermParseInfo());
				} else {
					auto minus_one = createDomElem(-1);
					auto minusOneTerm = new DomainTerm(VocabularyUtils::intsort(), minus_one, TermParseInfo());
					Function* times = Vocabulary::std()->func("*/2");
					times = times->disambiguate(vector<Sort*>(3, SortUtils::resolve(VocabularyUtils::intsort(), invertedSolvedTerm->sort())), 0);
					vector<Term*> timesterms(2);
					timesterms[0] = minusOneTerm;
					timesterms[1] = invertedSolvedTerm;
					solvedterm = new FuncTerm(times, timesterms, TermParseInfo());
				}

				PredForm* newatom = new PredForm(atom->sign(), newsymbol, { solvedterm, varterm }, atom->pi());
				delete (atom);
				return newatom;
			}
		}
	}
	return atom;
}

/**
 * NOTE: deletes the functerm and the atom
 */
PredForm* graphOneFunction(PredForm* atom, FuncTerm* ft, Term* rangeTerm) {
	vector<Term*> vt = ft->subterms();
	vt.push_back(rangeTerm);
	auto newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi());
	delete (atom);
	delete (ft);
	return newatom;
}

/**
 * NOTE:Graphs an atom.  Precondition: atom is an equality symbol and one of its arguments is a functerm
 */
PredForm* graphOneFunction(PredForm* atom) {
	Assert(atom->symbol()->name() == "=/2");
	FuncTerm* ft;
	Term* rangeTerm;
	if (sametypeid<FuncTerm>(*(atom->subterms()[1]))) {
		ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
		rangeTerm = atom->subterms()[0];
		return graphOneFunction(atom, ft, rangeTerm);
	} else {
		Assert(sametypeid<FuncTerm>(*(atom->subterms()[0])));
		ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
		rangeTerm = atom->subterms()[1];
	}
	return graphOneFunction(atom, ft, rangeTerm);
}

/**
 * This method serves for rewriting an atom a+b+c = 0 to
 * +(a,b,c) with c output. Or to y=x, with x output and y functionfree
 * In general, it tries to rewrite F(x,y) = z to ... = v with v some output variable.
 * If not such rewriting is possible, this method simply graphs the function.
 */
PredForm* rewriteSum(PredForm* atom, FuncTerm* lhs, Term* rhs, const vector<Pattern>& pattern, const vector<Variable*>& atomvars, FOBDDManager* manager) {
	Assert((atom->subterms()[0] == lhs&& atom->subterms()[1] == rhs)||(atom->subterms()[1] == lhs&& atom->subterms()[0] == rhs));
	auto newatom = solveAndReplace(atom, pattern, atomvars, manager, Pattern::OUTPUT);
	if (atom == newatom) {
		return graphOneFunction(newatom, lhs, rhs);
	}
	if (FormulaUtils::containsFuncTermsOutsideOfSets(newatom)) {
		//We are in the case the sum has been rewritten to t = x, with x still containing functions
		return graphOneFunction(newatom);
	}
	//We are in the case the sum has been rewritten to y = x, with x output and y no functionterm
	return newatom;
}

/**
 * Takes a specific atom (one of the four forms beneath) and rewrite it such that:
 * Either, the result is of the form a=x with x an output variable and a function-free (hence, a domainterm or a varterm)
 * Or, the result satisfies the following conditions
 * 1) The result is a predform without equality at the root
 * 2) if possible, sums are rewritten to be graphed as +(x,y,z) with z an output variable.
 * The input should be an atom that
 * a) contains functerms
 * b) has equality at its root
 */
//	(A)		F(t1,...,tn) = t,
//  (B)		t = F(t1,...,tn),
//  (C)		(t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m) = 0,
//  (D)		0 = (t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m).
PredForm *BDDToGenerator::smartGraphFunction(PredForm *atom, const vector<Pattern> & pattern, const vector<Variable*> & atomvars) {
	Assert(atom->symbol()->name() == "=/2");
	Assert(FormulaUtils::containsFuncTermsOutsideOfSets(atom));

	if (sametypeid<DomainTerm>(*(atom->subterms()[0]))) { // Case (B) or (D)
		Assert(sametypeid<FuncTerm>(*(atom->subterms()[1])));
		auto ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
		if (SortUtils::resolve(ft->sort(), VocabularyUtils::floatsort()) && (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) { // Case (D)
			atom = rewriteSum(atom, ft, atom->subterms()[0], pattern, atomvars, _manager);
		} else { // Case B
			atom = graphOneFunction(atom, ft, atom->subterms()[0]);
		}
	} else if (sametypeid<DomainTerm>(*(atom->subterms()[1]))) { // Case (A) or (C)
		Assert(sametypeid<FuncTerm>(*(atom->subterms()[0])));
		auto ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
		if (SortUtils::resolve(ft->sort(), VocabularyUtils::floatsort()) && (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) { // Case (C)
			atom = rewriteSum(atom, ft, atom->subterms()[1], pattern, atomvars, _manager);
		} else { // Case (B)
			atom = graphOneFunction(atom, ft, atom->subterms()[1]);
		}
	} else if (sametypeid<FuncTerm>(*(atom->subterms()[0]))) { // Case (A)
		auto ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
		atom = graphOneFunction(atom, ft, atom->subterms()[1]);
	} else { // Case (B)
		Assert(sametypeid<FuncTerm>(*(atom->subterms()[1])));
		auto ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
		atom = graphOneFunction(atom, ft, atom->subterms()[0]);
	}
	return atom;
}

bool checkInput(uint size1, uint size2, uint size3, const Universe& universe) {
	for (auto it = universe.tables().cbegin(); it != universe.tables().cend(); ++it) {
		if (*it == NULL) {
			return false;
		}
	}
	return size1 == size2 && size2 == size3 && size3 == universe.tables().size();
}

bool checkInput(const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars, const vector<const FOBDDVariable*>& bddvars,
		const Universe& universe) {
	return checkInput(pattern.size(), vars.size(), bddvars.size(), universe);
}
bool checkInput(const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars, const vector<Variable*>& atomvars, const Universe& universe) {
	return checkInput(pattern.size(), vars.size(), atomvars.size(), universe);

}
InstGenerator* BDDToGenerator::createFromSimplePredForm(PredForm* atom, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<Variable*>& atomvars, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe& universe) {
	Assert(checkInput(pattern, vars, atomvars, universe));
	//Check the precondition
	Assert(not FormulaUtils::containsFuncTerms(atom) && not FormulaUtils::containsAggTerms(atom));
	// Create the pattern for an atom with only Var and DomainTerms.
	vector<Pattern> atompattern;
	vector<const DomElemContainer*> datomvars;
	vector<SortTable*> atomtables;
	//Now we create atompattern, ...
	//These vectors should be in the order that the elements occur in atom.
	//If multiple occurrences, they should also occur multiple times in atompattern
	for (auto it = atom->subterms().cbegin(); it != atom->subterms().cend(); ++it) {
		Assert(sametypeid<VarTerm>(**it) || sametypeid<DomainTerm>(**it));
		if (sametypeid<VarTerm>(**it)) {
			auto var = (dynamic_cast<VarTerm*>(*it))->var();
			// For each var, find its position, pattern and table
			unsigned int pos = 0;
			for (; pos < pattern.size(); ++pos) {
				if (atomvars[pos] == var) {
					break;
				}
			}
			atompattern.push_back(pattern[pos]);
			datomvars.push_back(vars[pos]);
			atomtables.push_back(universe.tables()[pos]);
		} else { // Domain term
			// For each domain term, create a new variable and substitute the domainterm with the new varterm
			auto domterm = dynamic_cast<DomainTerm*>(*it);
			auto domelement = new const DomElemContainer();
			*domelement = domterm->value();
			auto var = new Variable(domterm->sort());
			auto newatom = dynamic_cast<PredForm*>(FormulaUtils::substituteTerm(atom, domterm, var));
#ifndef NDEBUG // Check that the var has really been added in place of the domainterm
			bool found = false;
			for (auto it = newatom->subterms().cbegin(); it != newatom->subterms().cend(); ++it) {
				if (sametypeid<VarTerm>(**it) && dynamic_cast<VarTerm*>(*it)->var() == var) {
					found = true;
				}Assert((*it)!=domterm);
			}Assert(found);
#endif
			vector<Pattern> termpattern(pattern);
			termpattern.push_back(Pattern::INPUT);
			vector<const DomElemContainer*> termvars(vars);
			termvars.push_back(domelement);
			vector<Variable*> fotermvars(atomvars);
			fotermvars.push_back(var);
			vector<SortTable*> termuniv(universe.tables());
			termuniv.push_back(structure->inter(domterm->sort()));

			// The domain term has been replaced, hence recursive case.
			// We cannot continue looping since other subterms have also already been replaced.
			return createFromSimplePredForm(newatom, termpattern, termvars, fotermvars, structure, branchToGenerate, Universe(termuniv));
		}
	}
	return GeneratorFactory::create(atom, structure, branchToGenerate == BRANCH::FALSEBRANCH, atompattern, datomvars, Universe(atomtables));
}

vector<Formula*> orderSubformulas(set<Formula*> atoms_to_order, Formula *& origatom, BRANCH & branchToGenerate, set<Variable*> free_vars,
		const AbstractStructure *& structure) {
	vector<Formula*> orderedconjunction;
	while (!atoms_to_order.empty()) {
		Formula *bestatom = 0;
		double bestcost = getMaxElem<double>();
		for (auto it = atoms_to_order.cbegin(); it != atoms_to_order.cend(); ++it) {
			bool currinverse = false;
			if (*it == origatom) {
				currinverse = (branchToGenerate == BRANCH::FALSEBRANCH);
			}
			set<Variable*> projectedfree;
			//projectedfree are the free variables that occur free in THIS atom
			for (auto jt = free_vars.cbegin(); jt != free_vars.cend(); ++jt) {
				if ((*it)->freeVars().find(*jt) != (*it)->freeVars().cend()) {
					projectedfree.insert(*jt);
				}
			}

			double currcost = FormulaUtils::estimatedCostAll(*it, projectedfree, currinverse, structure);
			if (currcost < bestcost) {
				bestcost = currcost;
				bestatom = *it;
			}
		}

		if (!bestatom) {
			bestatom = *(atoms_to_order.cbegin());
		}
		orderedconjunction.push_back(bestatom);
		atoms_to_order.erase(bestatom);
		for (auto it = bestatom->freeVars().cbegin(); it != bestatom->freeVars().cend(); ++it) {
			free_vars.erase(*it);
		}
	}Assert(free_vars.empty());
	return orderedconjunction;
}

vector<InstGenerator*> BDDToGenerator::turnConjunctionIntoGenerators(const vector<Pattern> & pattern, const vector<const DomElemContainer*> & vars,
		const vector<Variable*> & atomvars, const Universe & universe, QuantForm *quantform, const AbstractStructure *structure, vector<Formula*> conjunction,
		Formula *origatom, BRANCH branchToGenerate) {
	vector<InstGenerator*> generators;
	vector<Pattern> branchpattern = pattern;
	vector<const DomElemContainer*> branchvars = vars;
	vector<Variable*> branchfovars = atomvars;
	vector<SortTable*> branchuniverse = universe.tables();
	for (auto it = quantform->quantVars().cbegin(); it != quantform->quantVars().cend(); ++it) {
		branchpattern.push_back(Pattern::OUTPUT);
		branchvars.push_back(new const DomElemContainer());
		branchfovars.push_back(*it);
		branchuniverse.push_back(structure->inter((*it)->sort()));
	}
	for (auto it = conjunction.cbegin(); it != conjunction.cend(); ++it) {
		vector<Pattern> kernpattern;
		vector<const DomElemContainer*> kernvars;
		vector<Variable*> kernfovars;
		vector<SortTable*> kerntables;
		vector<Pattern> newbranchpattern;
		for (unsigned int n = 0; n < branchpattern.size(); ++n) {
			if ((*it)->freeVars().find(branchfovars[n]) == (*it)->freeVars().cend()) {
				newbranchpattern.push_back(branchpattern[n]);
			} else {
				kernpattern.push_back(branchpattern[n]);
				kernvars.push_back(branchvars[n]);
				kernfovars.push_back(branchfovars[n]);
				kerntables.push_back(branchuniverse[n]);
				newbranchpattern.push_back(Pattern::INPUT);
			}
		}

		branchpattern = newbranchpattern;
		if (*it == origatom) {
			//This distinction serves for: the original atom might need to be inverted, the newly created not.
			generators.push_back(createFromFormula(*it, kernpattern, kernvars, kernfovars, structure, branchToGenerate, Universe(kerntables)));
		} else {
			generators.push_back(createFromFormula(*it, kernpattern, kernvars, kernfovars, structure, BRANCH::TRUEBRANCH, Universe(kerntables)));
		}
	}

	return generators;
}

// FIXME error in removenesting if this does not introduce a quantifier
// FIXME no hardcoded string comparisons! (see "=/2")
// FIXME very ugly code
// TODO move to other class: not related to bdds. ---> Actually it is, since we heavily rely on the normal form the bdd has transformed our formula to!
/**
 * Given a predform, a pattern, structure and variables, create a generator which generates all true (false) values (false if inverse).
 * TODO can only call on specific predforms
 */
InstGenerator* BDDToGenerator::createFromPredForm(PredForm* atom, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<Variable*>& atomvars, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe& universe) {
	Assert(checkInput(pattern, vars, atomvars, universe));
	if (getOption(IntType::GROUNDVERBOSITY) > 3) {
		clog << "BDDGeneratorFactory visiting: " << toString(atom) << "\n";
	}
	if (atom->symbol()->name() == "=/2") {
		if (FormulaUtils::containsFuncTermsOutsideOfSets(atom)) {
			atom = smartGraphFunction(atom, pattern, atomvars);
		}
	} else if (atom->symbol()->name() == "</2" || atom->symbol()->name() == ">/2") {
		atom = solveAndReplace(atom, pattern, atomvars, _manager, Pattern::OUTPUT);
	}
	// NOW, atom is of one of the forms:
	// 1* a = b where a and b are no functerms (hence, aggterm, domainterm or varterm)
	// 2* P(x,y,z,...) with P no equality
	// -> In this case: two possibilities:
	// A** either, atom still contains functerms or aggterms, hence we need to unnest,...
	// B** or, atom only contains varterms and domainterms (this is the simplest case)

	//CASE 1
	if (atom->symbol()->name() == "=/2") {
		if (FormulaUtils::containsAggTerms(atom)) {
			auto newform = FormulaUtils::graphFuncsAndAggs(atom);
			Assert(sametypeid<AggForm>(*newform));
			auto transform = dynamic_cast<AggForm*>(newform);
			return createFromAggForm(transform, pattern, vars, atomvars, structure, branchToGenerate, universe);
		}

		Assert(not FormulaUtils::containsFuncTerms(atom));
		return createFromSimplePredForm(atom, pattern, vars, atomvars, structure, branchToGenerate, universe);
	}

	//CASE 2B
	if (not FormulaUtils::containsFuncTerms(atom) && not FormulaUtils::containsAggTerms(atom)) {
		return createFromSimplePredForm(atom, pattern, vars, atomvars, structure, branchToGenerate, universe);
	}
	//CASE 2A
	//We unnest non-recursive since we don't want to pull functerms outside of aggregates.

	auto newform = FormulaUtils::unnestFuncsAndAggsNonRecursive(atom, NULL, Context::NEGATIVE);
	newform = FormulaUtils::splitComparisonChains(newform);
	newform = FormulaUtils::graphFuncsAndAggs(newform);
	FormulaUtils::flatten(newform);
	if (not sametypeid<QuantForm>(*newform)) {
		throw notyetimplemented("Creating a bdd in which unnesting does not introduce quantifiers.");
	}
	QuantForm *quantform = dynamic_cast<QuantForm*>(newform);
	Assert(sametypeid<BoolForm>(*(quantform->subformula())));
	BoolForm *boolform = dynamic_cast<BoolForm*>(quantform->subformula());
	Assert(boolform->isConjWithSign());
	for (auto it = boolform->subformulas().cbegin(); it != boolform->subformulas().cend(); ++it) {
		Assert(sametypeid<PredForm>(**it)||sametypeid<AggForm>(**it));
	}
	Formula *origatom = boolform->subformulas().back();
	//All variables that are still free: the original free variables + the quantvars
	set<Variable*> all_free_vars;
	for (unsigned int n = 0; n < pattern.size(); ++n) {
		if (pattern[n] == Pattern::OUTPUT) {
			all_free_vars.insert(atomvars[n]);
		}
	}
	for (auto it = quantform->quantVars().cbegin(); it != quantform->quantVars().cend(); ++it) {
		all_free_vars.insert(*it);
	}
	set<Formula*> atoms_to_order(boolform->subformulas().cbegin(), boolform->subformulas().cend());
	vector<Formula*> orderedconjunction = orderSubformulas(atoms_to_order, origatom, branchToGenerate, all_free_vars, structure);

	vector<InstGenerator*> generators = turnConjunctionIntoGenerators(pattern, vars, atomvars, universe, quantform, structure, orderedconjunction, origatom,
			branchToGenerate);

	if (generators.size() == 1)
		return generators[0];
	else {
		GeneratorNode* node = 0;
		for (auto it = generators.rbegin(); it != generators.rend(); ++it) {
			if (node)
				node = new OneChildGeneratorNode(*it, node);
			else
				node = new LeafGeneratorNode(*it);
		}
		return new TreeInstGenerator(node);
	}

}

InstGenerator* BDDToGenerator::createFromKernel(const FOBDDKernel* kernel, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<const FOBDDVariable*>& fobddvars, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe& universe) {
	Assert(checkInput(pattern, vars, fobddvars, universe));
	if (sametypeid<FOBDDAggKernel>(*kernel)) {
		return createFromAggKernel(dynamic_cast<const FOBDDAggKernel*>(kernel), pattern, vars, fobddvars, structure, branchToGenerate, universe);
	}
	if (sametypeid<FOBDDAtomKernel>(*kernel)) {
		auto formula = _manager->toFormula(kernel);
		Assert(sametypeid<PredForm>(*formula) || sametypeid<AggForm>(*formula));
		vector<Variable*> atomvars;
		for (auto it = fobddvars.cbegin(); it != fobddvars.cend(); ++it) {
			atomvars.push_back((*it)->variable());
		}
		auto gen = createFromFormula(formula, pattern, vars, atomvars, structure, branchToGenerate, universe);
		if (getOption(IntType::GROUNDVERBOSITY) > 3) {
			clog << "Created kernel generator: " << toString(gen) << "\n";
		}
		return gen;
	}

	// Quantification kernel
	Assert(sametypeid<FOBDDQuantKernel>(*kernel));
	auto quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

	BddGeneratorData quantdata;
	quantdata.structure = structure;

	// Create a new variable
	Variable* quantvar = new Variable(quantkernel->sort());
	const FOBDDVariable* bddquantvar = _manager->getVariable(quantvar);
	const FOBDDDeBruijnIndex* quantindex = _manager->getDeBruijnIndex(quantkernel->sort(), 0);

	// Substitute the variable for the De Bruyn index
	quantdata.bdd = _manager->substitute(quantkernel->bdd(), quantindex, bddquantvar);

	// Create a generator for the quantified formula
	if (branchToGenerate == BRANCH::FALSEBRANCH) {
		//To create a falsebranchgenerator, we need a checker of the subformula
		quantdata.pattern = vector<Pattern>(pattern.size(), Pattern::INPUT);
	} else {
		//To get all positive answers, we generate tuples satisfying the subformula (same input pattern as always)
		quantdata.pattern = pattern;
	}

	//The quantvar is output, both in the checker for falsebranch as the generator for truebranch
	quantdata.pattern.push_back(Pattern::OUTPUT);

	quantdata.vars = vars;
	quantdata.vars.push_back(new const DomElemContainer());

	quantdata.bddvars = fobddvars;
	quantdata.bddvars.push_back(bddquantvar);

	quantdata.universe = universe.tables();
	quantdata.universe.addTable(structure->inter(quantkernel->sort()));

	BDDToGenerator btg(_manager);

	// Create a generator for the kernel
	if (branchToGenerate == BRANCH::FALSEBRANCH) {
		vector<const DomElemContainer*> univgenvars;
		vector<SortTable*> univgentables;
		for (unsigned int n = 0; n < quantdata.pattern.size() - 1; ++n) {
			if (pattern[n] == Pattern::OUTPUT) {
				univgenvars.push_back(quantdata.vars[n]);
				univgentables.push_back(quantdata.universe.tables()[n]);
			}
		}
		auto univgenerator = GeneratorFactory::create(univgenvars, univgentables);

		auto bddtruechecker = btg.create(quantdata);
		return new FalseQuantKernelGenerator(univgenerator, bddtruechecker);
	} else {
		vector<const DomElemContainer*> outvars;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (pattern[n] == Pattern::OUTPUT) {
				outvars.push_back(vars[n]);
			}
		}
		auto bddtruegenerator = btg.create(quantdata);
		return new TrueQuantKernelGenerator(bddtruegenerator, outvars);
	}
}

InstGenerator* BDDToGenerator::createFromAggForm(AggForm* af, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
		const std::vector<Variable*>& fovars, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe& universe) {
	//Now, make all variables input and turn the aggterm into bdd.
	Assert(checkInput(pattern, vars, fovars, universe));
	BddGeneratorData data;
	FOBDDFactory factory(_manager);
	data.bdd = factory.turnIntoBdd(af);
	data.bdd = branchToGenerate == BRANCH::FALSEBRANCH ? _manager->negation(data.bdd) : data.bdd;
	std::vector<const FOBDDVariable*> bddvars(fovars.size());
	int i = 0;
	for (auto var = fovars.cbegin(); var != fovars.cend(); ++var, i++) {
		bddvars[i] = _manager->getVariable(*var);
	}
	data.vars = vars;
	data.bddvars = bddvars;
	data.pattern = pattern;
	data.structure = structure;
	data.universe = universe;
	return create(data);
}

InstGenerator* BDDToGenerator::createFromFormula(Formula* f, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
		const std::vector<Variable*>& fovars, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe& universe) {
	Assert(checkInput(pattern, vars, fovars, universe));
	if (sametypeid<PredForm>(*f)) {
		auto newf = dynamic_cast<PredForm*>(f);
		return createFromPredForm(newf, pattern, vars, fovars, structure, branchToGenerate, universe);
	}Assert(sametypeid<AggForm>(*f));
	auto newf = dynamic_cast<AggForm*>(f);
	return createFromAggForm(newf, pattern, vars, fovars, structure, branchToGenerate, universe);
}

InstGenerator* BDDToGenerator::createFromAggKernel(const FOBDDAggKernel* ak, const std::vector<Pattern>& pattern,
		const std::vector<const DomElemContainer*>& vars, const std::vector<const FOBDDVariable*>& fobddvars, const AbstractStructure* structure,
		BRANCH branchToGenerate, const Universe& universe) {
	//First, we create a generator for all the free variables (that are output), since the AggregateGenerator assumes everything input
	//EXCEPTION: if left of the aggform is one of those variables
	Assert(checkInput(pattern, vars, fobddvars, universe));
	auto comp = (branchToGenerate == BRANCH::FALSEBRANCH ? negateComp(ak->comp()) : ak->comp());
	std::vector<const DomElemContainer*> freevars;
	std::vector<SortTable*> freetables;
	std::vector<Pattern> newpattern(vars.size(), Pattern::INPUT);
	const DomElemContainer* left;
	Pattern leftpattern;
	for (unsigned int n = 0; n < vars.size(); n++) {
		if (not sametypeid<FOBDDVariable>(*(ak->left())) || ak->left() != fobddvars[n]) {
			if (pattern[n] == Pattern::OUTPUT) {
				freevars.push_back(vars[n]);
				freetables.push_back(universe.tables()[n]);
			}
		} else {
			//If the left term of the aggkernel appears in the righthand, we cannot make it output of the agggenerator.
			//In this case, we also add it to the freegenerator. Otherwise, we let the agggenerator decide it's value.
			if (_manager->contains(ak->right(), fobddvars[n])) {
				if (pattern[n] == Pattern::OUTPUT) {
					freevars.push_back(vars[n]);
					freetables.push_back(universe.tables()[n]);
				}
			} else {
				newpattern[n] = pattern[n];
			}
			leftpattern = pattern[n];
			left = vars[n];
		}
	}

	Assert(left != NULL);
	auto freegenerator = GeneratorFactory::create(freevars, freetables); //TODO: simplify if no free vars!

	//Now, we start creating the formulagenerators and termgenerators for the agggenerator
	auto set = ak->right()->setexpr();
	std::vector<InstGenerator*> formulagenerators(set->size());
	std::vector<InstGenerator*> termgenerators(set->size());
	std::vector<const DomElemContainer*> terms(set->size());

	BddGeneratorData data;
	//TODO: optimize for every formula separately: remove vars that dont appear in subformula or subterm.
	//Will this help?
	auto subformvars = vars;
	auto subformbddvars = fobddvars;
	//For subformulas, everything is input
	std::vector<Pattern> subformpattern(subformvars.size(), Pattern::INPUT);
	auto subformtables = universe.tables();
	std::map<const FOBDDDeBruijnIndex*, const FOBDDVariable*> deBruynMapping;
	int i = 0;
	for (auto it = set->quantvarsorts().crbegin(); it != set->quantvarsorts().crend(); it++, i++) {
		auto newquantvar = new DomElemContainer();
		auto newfobddvar = _manager->getVariable((new Variable(*it)));
		subformvars.push_back(newquantvar);
		subformpattern.push_back(Pattern::OUTPUT);
		subformbddvars.push_back(newfobddvar);
		deBruynMapping[_manager->getDeBruijnIndex(*it, i)] = newfobddvar;
		subformtables.push_back(structure->inter(*it));
	}

	data.vars = subformvars;
	data.bddvars = subformbddvars;
	data.pattern = subformpattern;
	data.structure = structure;
	data.universe = Universe(subformtables);

	for (int j = 0; j < set->size(); j++) {
		data.bdd = _manager->substitute(set->subformula(j), deBruynMapping);
		formulagenerators[j] = create(data);
		const DomElemContainer* newvar = new DomElemContainer();
		terms[j] = newvar;
		auto sort = set->subterm(j)->sort();
		auto newfobddvar = _manager->getVariable(new Variable(sort));
		auto equalpred = VocabularyUtils::equal(sort); //TODO: depends on comparison!!!
		auto equalkernel = _manager->getAtomKernel(equalpred, AtomKernelType::AKT_TWOVALUED,
				{ newfobddvar, _manager->substitute(set->subterm(j), deBruynMapping) });
		auto termvars = subformvars;
		termvars.push_back(newvar);
		auto termfobddvars = subformbddvars;
		termfobddvars.push_back(newfobddvar);
		//For the term: everything is input, except for the newly created variable. It is calculated in function of all other variables.
		std::vector<Pattern> termpattern(subformpattern.size(), Pattern::INPUT);
		termpattern.push_back(Pattern::OUTPUT);
		auto termtables = subformtables;
		termtables.push_back(structure->inter(sort));
		auto newuniverse = new Universe(termtables);
		termgenerators[j] = createFromKernel(equalkernel, termpattern, termvars, termfobddvars, structure, BRANCH::TRUEBRANCH, *newuniverse);
	}
	auto rightvalue = new DomElemContainer();
	//AggGenerator calculates the value of the set and stores it in "rightvalue".
	auto aggGenerator = new AggGenerator(rightvalue, ak->right()->aggfunction(), formulagenerators, termgenerators, terms);

	//Finally, we construct the  comparisongenerator
	//We want left to be comp than the value of the set (i.e.~rightvalue)
	auto compgenerator = new ComparisonGenerator(structure->inter(ak->left()->sort()), structure->inter(ak->right()->sort()), left, rightvalue,
			(leftpattern == Pattern::INPUT ? Input::BOTH : Input::RIGHT), comp);

	return new TreeInstGenerator(new OneChildGeneratorNode(freegenerator, new OneChildGeneratorNode(aggGenerator, new LeafGeneratorNode(compgenerator))));

}
