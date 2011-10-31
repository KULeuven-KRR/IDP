#include <cassert>

#include "term.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "fobdd.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/SimpleFuncGenerator.hpp"
#include "generators/TreeInstGenerator.hpp"
#include "generators/InverseInstGenerator.hpp"
#include "generators/SortInstGenerator.hpp"
#include "generators/LookupGenerator.hpp"
#include "generators/EnumLookupGenerator.hpp"
#include "generators/EmptyGenerator.hpp"
#include "generators/GeneratorFactory.hpp"
#include "generators/FalseQuantKernelGenerator.hpp"
#include "generators/TrueQuantKernelGenerator.hpp"

using namespace std;

BDDToGenerator::BDDToGenerator(FOBDDManager* manager) :
		_manager(manager) {
}

InstGenerator* BDDToGenerator::create(const FOBDD* bdd, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure, const Universe& universe) {

//cerr << "Create on bdd\n";
//_manager->put(cerr,bdd);
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "bddvars = "; for(unsigned int n = 0; n < bddvars.size(); ++n) cerr << "  " << *(bddvars[n]->variable()); cerr << endl;

// Detect double occurrences
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

	if (bdd == _manager->falsebdd()) {
		return new EmptyGenerator();
	} else if (bdd == _manager->truebdd()) {
		vector<const DomElemContainer*> outvars;
		vector<SortTable*> tables;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (!pattern[n]) {
				if (firstocc[n] == n) {
					outvars.push_back(vars[n]);
					tables.push_back(universe.tables()[n]);
				}
			}
		}
		GeneratorFactory gf;
		InstGenerator* result = gf.create(outvars, tables);
		return result;
	} else {
		GeneratorNode* gn = createnode(bdd, pattern, vars, bddvars, structure, universe);
		return new TreeInstGenerator(gn);
	}
}

GeneratorNode* BDDToGenerator::createnode(const FOBDD* bdd, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure, const Universe& universe) {

	// Detect double occurrences
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

	if (bdd == _manager->falsebdd()) {
		EmptyGenerator* eg = new EmptyGenerator();
		return new LeafGeneratorNode(eg);
	} else if (bdd == _manager->truebdd()) {
		vector<const DomElemContainer*> outvars;
		vector<SortTable*> tables;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (!pattern[n]) {
				if (firstocc[n] == n) {
					outvars.push_back(vars[n]);
					tables.push_back(universe.tables()[n]);
				}
			}
		}
		GeneratorFactory gf;
		InstGenerator* ig = gf.create(outvars, tables);
		return new LeafGeneratorNode(ig);
	} else {
		// split variables
		vector<Pattern> kernpattern;
		vector<const DomElemContainer*> kerngenvars;
		vector<const FOBDDVariable*> kernvars;
		vector<SortTable*> kerntables;
		vector<Pattern> branchpattern;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (_manager->contains(bdd->kernel(), bddvars[n])) {
				kernpattern.push_back(pattern[n]);
				kerngenvars.push_back(vars[n]);
				kernvars.push_back(bddvars[n]);
				kerntables.push_back(universe.tables()[n]);
				branchpattern.push_back(Pattern::INPUT);
			} else {
				branchpattern.push_back(pattern[n]);
			}
		}

		// recursive case
		if (bdd->falsebranch() == _manager->falsebdd()) {
			// Only generate the true branch possibilities
			InstGenerator* kernelgenerator = create(bdd->kernel(), kernpattern, kerngenvars, kernvars, structure, false, Universe(kerntables));
			GeneratorNode* truegenerator = createnode(bdd->truebranch(), branchpattern, vars, bddvars, structure, universe);
			return new OneChildGeneratorNode(kernelgenerator, truegenerator);
		}

		else if (bdd->truebranch() == _manager->falsebdd()) {
			// Only generate the false branch possibilities
			InstGenerator* kernelgenerator = create(bdd->kernel(), kernpattern, kerngenvars, kernvars, structure, true, Universe(kerntables));
			GeneratorNode* falsegenerator = createnode(bdd->falsebranch(), branchpattern, vars, bddvars, structure, universe);
			return new OneChildGeneratorNode(kernelgenerator, falsegenerator);
		} else {
			vector<Pattern> checkpattern(kernpattern.size(), Pattern::INPUT);
			InstChecker* kernelchecker = create(bdd->kernel(), checkpattern, kerngenvars, kernvars, structure, false, Universe(kerntables));
			vector<const DomElemContainer*> kgvars(0);
			vector<SortTable*> kguniv;
			for (unsigned int n = 0; n < kerngenvars.size(); ++n) {
				if (!kernpattern[n]) {
					unsigned int m = 0;
					for (; m < kgvars.size(); ++m) {
						if (kgvars[m] == kerngenvars[n])
							break;
					}
					if (m == kgvars.size()) {
						kgvars.push_back(kerngenvars[n]);
						kguniv.push_back(kerntables[n]);
					}
				}
			}
			GeneratorFactory gf;
			InstGenerator* kernelgenerator = gf.create(kgvars, kguniv); // Both branches possible, so just generate all possibilities
			GeneratorNode* truegenerator = createnode(bdd->truebranch(), branchpattern, vars, bddvars, structure, universe);
			GeneratorNode* falsegenerator = createnode(bdd->falsebranch(), branchpattern, vars, bddvars, structure, universe);
			return new TwoChildGeneratorNode(kernelchecker, kernelgenerator, falsegenerator, truegenerator);
		}
		return 0;
	}
}

// FIXME error in removenesting if this does not introduce a quantifier
// FIXME very ugly code
// FIXME a code in BDDTOGenerator that does not take a bdd and does not return something with bdds?
// TODO what should the method do exactly?
InstGenerator* BDDToGenerator::create(PredForm* atom, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<Variable*>& atomvars, AbstractStructure* structure, bool inverse, const Universe& universe) {
	if (FormulaUtils::containsFuncTerms(atom)) {
		bool allinput = true;
		for (auto it = pattern.cbegin(); it != pattern.cend(); ++it) {
			if (!(*it)) {
				allinput = false;
				break;
			}
		}
		if (allinput) {
			for (unsigned int n = 0; n < pattern.size(); ++n) {
				Term* solvedterm = solve(atom, atomvars[n]);
				if (solvedterm) {
					vector<Term*> newargs(2);
					newargs[0] = new VarTerm(atomvars[n], TermParseInfo());
					newargs[1] = solvedterm;
					PredForm* newatom = new PredForm(atom->sign(), atom->symbol(), newargs, atom->pi().clone());
					delete (atom);
					atom = newatom;
					break;
				}
			}
		} else {

			// The atom is of one of the following forms:
			//	(A)		P(t1,...,tn),
			//	(B)		F(t1,...,tn) = t,
			//  (C)		t = F(t1,...,tn),
			//  (D)		(t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m) = 0,
			//  (E)		0 = (t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m).

			// Convert all cases to case (A)
			// FIXME no hardcoded string comparisons!
			if (atom->symbol()->name() == "=/2") { // cases (B), (C), (D), and (E)
				if (typeid(*(atom->subterms()[0])) == typeid(DomainTerm)) { // Case (C) or (E)
					assert(typeid(*(atom->subterms()[1])) == typeid(FuncTerm));
					FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
					if (SortUtils::resolve(ft->sort(), VocabularyUtils::floatsort())
							&& (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) { // Case (E)
						unsigned int n = 0;
						for (unsigned int n = 0; n < pattern.size(); ++n) {
							if (!pattern[n]) {
								Term* solvedterm = solve(atom, atomvars[n]);
								if (solvedterm) {
									vector<Term*> newargs(2);
									newargs[0] = new VarTerm(atomvars[n], TermParseInfo());
									newargs[1] = solvedterm;
									PredForm* newatom = new PredForm(atom->sign(), atom->symbol(), newargs, atom->pi().clone());
									delete (atom);
									atom = newatom;
									break;
								}
							}
						}
						if (n == pattern.size()) {
							vector<Term*> vt = ft->subterms();
							vt.push_back(atom->subterms()[0]);
							PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
							delete (atom);
							delete (ft);
							atom = newatom;
						}
					} else { // Case (C)
						vector<Term*> vt = ft->subterms();
						vt.push_back(atom->subterms()[0]);
						PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
						delete (atom);
						delete (ft);
						atom = newatom;
					}
				} else if (typeid(*(atom->subterms()[1])) == typeid(DomainTerm)) { // Case (B) or (D)
					assert(typeid(*(atom->subterms()[0])) == typeid(FuncTerm));
					FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
					if (SortUtils::resolve(ft->sort(), VocabularyUtils::floatsort())
							&& (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) { // Case (D)
						unsigned int n = 0;
						for (unsigned int n = 0; n < pattern.size(); ++n) {
							if (!pattern[n]) {
								Term* solvedterm = solve(atom, atomvars[n]);
								if (solvedterm) {
									vector<Term*> newargs(2);
									newargs[0] = new VarTerm(atomvars[n], TermParseInfo());
									newargs[1] = solvedterm;
									PredForm* newatom = new PredForm(atom->sign(), atom->symbol(), newargs, atom->pi().clone());
									delete (atom);
									atom = newatom;
									break;
								}
							}
						}
						if (n == pattern.size()) {
							vector<Term*> vt = ft->subterms();
							vt.push_back(atom->subterms()[1]);
							PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
							delete (atom);
							delete (ft);
							atom = newatom;
						}
					} else { // Case (B)
						vector<Term*> vt = ft->subterms();
						vt.push_back(atom->subterms()[1]);
						PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
						delete (atom);
						delete (ft);
						atom = newatom;
					}
				} else if (typeid(*(atom->subterms()[0])) == typeid(FuncTerm)) { // Case (B)
					FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
					vector<Term*> vt = ft->subterms();
					vt.push_back(atom->subterms()[1]);
					PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
					delete (atom);
					delete (ft);
					atom = newatom;
				} else { // Case (C)
					FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
					vector<Term*> vt = ft->subterms();
					vt.push_back(atom->subterms()[0]);
					PredForm* newatom = new PredForm(atom->sign(), ft->function(), vt, atom->pi().clone());
					delete (atom);
					delete (ft);
					atom = newatom;
				}
			}
		}
	}

	if (FormulaUtils::containsFuncTerms(atom)) {
		Formula* newform = FormulaUtils::removeNesting(atom, Context::NEGATIVE);
		newform = FormulaUtils::removeEqChains(newform);
		newform = FormulaUtils::graphFunctions(newform);
		newform = FormulaUtils::flatten(newform);
		assert(sametypeid<QuantForm>(*newform));
		QuantForm* quantform = dynamic_cast<QuantForm*>(newform);
		assert(sametypeid<BoolForm>(*(quantform->subformula())));
		BoolForm* boolform = dynamic_cast<BoolForm*>(quantform->subformula());
		vector<PredForm*> conjunction;
		for (auto it = boolform->subformulas().cbegin(); it != boolform->subformulas().cend(); ++it) {
			assert(typeid(*(*it)) == typeid(PredForm));
			conjunction.push_back(dynamic_cast<PredForm*>(*it));
		}
		PredForm* origatom = conjunction.back();
		set<Variable*> still_free;
		for (unsigned int n = 0; n < pattern.size(); ++n) {
			if (not pattern[n]) {
				still_free.insert(atomvars[n]);
			}
		}
		for (auto it = quantform->quantVars().cbegin(); it != quantform->quantVars().cend(); ++it) {
			still_free.insert(*it);
		}
		set<PredForm*> atoms_to_order(conjunction.cbegin(), conjunction.cend());
		vector<PredForm*> orderedconjunction;
		while (!atoms_to_order.empty()) {
			PredForm* bestatom = 0;
			double bestcost = numeric_limits<double>::max();
			for (auto it = atoms_to_order.cbegin(); it != atoms_to_order.cend(); ++it) {
				bool currinverse = false;
				if (*it == origatom) {
					currinverse = inverse;
				}
				set<Variable*> projectedfree;
				for (auto jt = still_free.cbegin(); jt != still_free.cend(); ++jt) {
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
			if (not bestatom) {
				bestatom = *(atoms_to_order.cbegin());
			}
			orderedconjunction.push_back(bestatom);
			atoms_to_order.erase(bestatom);
			for (auto it = bestatom->freeVars().cbegin(); it != bestatom->freeVars().cend(); ++it) {
				still_free.erase(*it);
			}
		}

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
		for (auto it = orderedconjunction.cbegin(); it != orderedconjunction.cend(); ++it) {
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
			if (*it == origatom)
				generators.push_back(create(*it, kernpattern, kernvars, kernfovars, structure, inverse, Universe(kerntables)));
			else
				generators.push_back(create(*it, kernpattern, kernvars, kernfovars, structure, false, Universe(kerntables)));
		}

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
	} else {
		// Create the pattern for the atom
		vector<Pattern> atompattern;
		vector<const DomElemContainer*> datomvars;
		vector<SortTable*> atomtables;
		for (auto it = atom->subterms().cbegin(); it != atom->subterms().cend(); ++it) {
			if (typeid(*(*it)) == typeid(VarTerm)) {
				Variable* var = (dynamic_cast<VarTerm*>(*it))->var();
				unsigned int pos = 0;
				for (; pos < pattern.size(); ++pos) {
					if (atomvars[pos] == var)
						break;
				}assert(pos < pattern.size());
				atompattern.push_back(pattern[pos]);
				datomvars.push_back(vars[pos]);
				atomtables.push_back(universe.tables()[pos]);
			} else if (typeid(*(*it)) == typeid(DomainTerm)) {
				DomainTerm* domterm = dynamic_cast<DomainTerm*>(*it);
				const DomElemContainer* domelement = new const DomElemContainer();
				*domelement = domterm->value();

				Variable* var = new Variable(domterm->sort());
				PredForm* newatom = dynamic_cast<PredForm*>(FormulaUtils::substitute(atom, domterm, var));

				vector<Pattern> termpattern(pattern);
				termpattern.push_back(Pattern::INPUT);
				vector<const DomElemContainer*> termvars(vars);
				termvars.push_back(domelement);
				vector<Variable*> fotermvars(atomvars);
				fotermvars.push_back(var);
				vector<SortTable*> termuniv(universe.tables());
				termuniv.push_back(structure->inter(domterm->sort()));

				return create(newatom, termpattern, termvars, fotermvars, structure, inverse, Universe(termuniv));
			} else
				assert(false);
		}

		// Construct the generator
		PFSymbol* symbol = atom->symbol();
		const PredInter* inter = 0;
		if (typeid(*symbol) == typeid(Predicate))
			inter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else {
			assert(typeid(*symbol) == typeid(Function));
			inter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* table = 0;
		if (sametypeid<Predicate>(*(atom->symbol()))) {
			Predicate* predicate = dynamic_cast<Predicate*>(atom->symbol());
			switch (predicate->type()) {
				case ST_NONE:
					table = inverse ? inter->cf() : inter->ct();
					break;
				case ST_CT:
					table = inverse ? inter->pf() : inter->ct();
					break;
				case ST_CF:
					table = inverse ? inter->pt() : inter->cf();
					break;
				case ST_PT:
					table = inverse ? inter->cf() : inter->pt();
					break;
				case ST_PF:
					table = inverse ? inter->ct() : inter->pf();
					break;
			}
		} else {
			table = inverse ? inter->cf() : inter->ct();
		}
		return GeneratorFactory::create(table, atompattern, datomvars, Universe(atomtables));
	}
}

InstGenerator* BDDToGenerator::create(const FOBDDKernel* kernel, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<const FOBDDVariable*>& kernelvars, AbstractStructure* structure, bool inverse, const Universe& universe) {

//cerr << "Create on kernel\n";
//_manager->put(cerr,kernel);
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "kernelvars = "; for(unsigned int n = 0; n < kernelvars.size(); ++n) cerr << "  " << *(kernelvars[n]->variable()); cerr << endl;
//cerr << "inverse = " << (inverse ? "true" : "false") << endl;

	if (typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);

		if (_manager->containsFuncTerms(atom)) {
			Formula* atomform = _manager->toFormula(atom);
			assert(typeid(*atomform) == typeid(PredForm));
			PredForm* pf = dynamic_cast<PredForm*>(atomform);
			vector<Variable*> atomvars;
			for (auto it = kernelvars.cbegin(); it != kernelvars.cend(); ++it)
				atomvars.push_back((*it)->variable());
			return create(pf, pattern, vars, atomvars, structure, inverse, universe);
		}

		// Create the pattern for the atom
		vector<Pattern> atompattern;
		vector<const DomElemContainer*> atomvars;
		vector<SortTable*> atomtables;
		for (auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
			if (typeid(*(*it)) == typeid(FOBDDVariable)) {
				const FOBDDVariable* var = dynamic_cast<const FOBDDVariable*>(*it);
				unsigned int pos = 0;
				for (; pos < pattern.size(); ++pos) {
					if (kernelvars[pos] == var)
						break;
				}assert(pos < pattern.size());
				atompattern.push_back(pattern[pos]);
				atomvars.push_back(vars[pos]);
				atomtables.push_back(universe.tables()[pos]);
			} else if (typeid(*(*it)) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* domterm = dynamic_cast<const FOBDDDomainTerm*>(*it);
				const DomElemContainer* domelement = new const DomElemContainer();
				*domelement = domterm->value();

				Variable* termvar = new Variable(domterm->sort());
				const FOBDDVariable* bddtermvar = _manager->getVariable(termvar);
				const FOBDDKernel* termkernel = _manager->substitute(kernel, domterm, bddtermvar);

				vector<Pattern> termpattern(pattern);
				termpattern.push_back(Pattern::INPUT);
				vector<const DomElemContainer*> termvars(vars);
				termvars.push_back(domelement);
				vector<const FOBDDVariable*> termkernelvars(kernelvars);
				termkernelvars.push_back(bddtermvar);
				vector<SortTable*> termuniv(universe.tables());
				termuniv.push_back(structure->inter(domterm->sort()));

				return create(termkernel, termpattern, termvars, termkernelvars, structure, inverse, Universe(termuniv));
			} else
				assert(false);
		}

		// Construct the generator
		PFSymbol* symbol = atom->symbol();
		const PredInter* inter = 0;
		if (typeid(*symbol) == typeid(Predicate))
			inter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else {
			assert(typeid(*symbol) == typeid(Function));
			inter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* table = 0;
		switch (atom->type()) {
			case AKT_TWOVAL:
				table = inverse ? inter->cf() : inter->ct();
				break;
			case AKT_CF:
				table = inverse ? inter->pt() : inter->cf();
				break;
			case AKT_CT:
				table = inverse ? inter->pf() : inter->ct();
				break;
		}
		return GeneratorFactory::create(table, atompattern, atomvars, Universe(atomtables));
	} else { // Quantification kernel
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

		// Create a new variable
		Variable* quantvar = new Variable(quantkernel->sort());
		const FOBDDVariable* bddquantvar = _manager->getVariable(quantvar);
		const FOBDDDeBruijnIndex* quantindex = _manager->getDeBruijnIndex(quantkernel->sort(), 0);

		// Substitute the variable for the De Bruyn index
		const FOBDD* quantbdd = _manager->substitute(quantkernel->bdd(), quantindex, bddquantvar);

		// Create a generator for then quantified formula
		vector<Pattern> quantpattern;
		if (inverse)
			quantpattern = vector<Pattern>(pattern.size(), Pattern::INPUT);
		else
			quantpattern = pattern;
		quantpattern.push_back(Pattern::OUTPUT);
		vector<const DomElemContainer*> quantvars(vars);
		quantvars.push_back(new const DomElemContainer());
		vector<const FOBDDVariable*> bddquantvars(kernelvars);
		bddquantvars.push_back(bddquantvar);
		vector<SortTable*> quantuniv = universe.tables();
		quantuniv.push_back(structure->inter(quantkernel->sort()));
		BDDToGenerator btg(_manager);

		// Create a generator for the kernel
		InstGenerator* result = 0;
		if (inverse) {
			GeneratorFactory gf;
			vector<const DomElemContainer*> univgenvars;
			vector<SortTable*> univgentables;
			for (unsigned int n = 0; n < pattern.size(); ++n) {
				if (!pattern[n]) {
					univgenvars.push_back(vars[n]);
					univgentables.push_back(universe.tables()[n]);
				}
			}
			InstGenerator* univgenerator = gf.create(univgenvars, univgentables);
			InstChecker* bddtruechecker = btg.create(quantbdd, quantpattern, quantvars, bddquantvars, structure, Universe(quantuniv)); // TODO review checking?
			result = new FalseQuantKernelGenerator(univgenerator, bddtruechecker);
		} else {
			unsigned int firstout = 0;
			for (; firstout < pattern.size(); ++firstout) {
				if (!pattern[firstout])
					break;
			}
			if (firstout == pattern.size()) {
				InstGenerator* quantgenerator = btg.create(quantbdd, vector<Pattern>(quantvars.size(), Pattern::INPUT), quantvars, bddquantvars,
						structure, Universe(quantuniv));
				result = new TrueQuantKernelGenerator(quantgenerator);
			} else {
				InstGenerator* quantgenerator = btg.create(quantbdd, quantpattern, quantvars, bddquantvars, structure, Universe(quantuniv));
				result = new TrueQuantKernelGenerator(quantgenerator);
			}
		}

		return result;
	}
}

Term* BDDToGenerator::solve(PredForm* atom, Variable* var) {
	FOBDDFactory factory(_manager);
	const FOBDD* bdd = factory.run(atom);
	assert(!_manager->isTruebdd(bdd));
	assert(!_manager->isFalsebdd(bdd));
	const FOBDDKernel* kernel = bdd->kernel();
	const FOBDDArgument* arg = _manager->getVariable(var);
	const FOBDDArgument* solved = _manager->solve(kernel, arg);
	if (solved)
		return _manager->toTerm(solved);
	else
		return 0;
}
