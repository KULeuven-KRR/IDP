#include <cassert>

#include "term.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "theory.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/SimpleFuncGenerator.hpp"
#include "generators/TreeInstGenerator.hpp"
#include "generators/InverseInstGenerator.hpp"
#include "generators/SortInstGenerator.hpp"
#include "generators/LookupGenerator.hpp"
#include "generators/EnumLookupGenerator.hpp"
#include "generators/BasicGenerators.hpp"
#include "generators/GeneratorFactory.hpp"
#include "generators/FalseQuantKernelGenerator.hpp"
#include "generators/TrueQuantKernelGenerator.hpp"

#include "theorytransformations/Utils.hpp"

using namespace std;

Term* solve(FOBDDManager& manager, PredForm* atom, Variable* var) {
	FOBDDFactory factory(&manager);
	const FOBDD* bdd = factory.run(atom);
	assert(not manager.isTruebdd(bdd));
	assert(not manager.isFalsebdd(bdd));
	const FOBDDKernel* kernel = bdd->kernel();
	const FOBDDArgument* arg = manager.getVariable(var);
	const FOBDDArgument* rewrittenarg = manager.solve(kernel, arg);
	if (rewrittenarg != NULL) {
		return manager.toTerm(rewrittenarg);
	} else {
		return NULL;
	}
}

vector<uint> detectDoubleOccurences(const vector<const DomElemContainer*>& vars) {
	vector<uint> firstocc;
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

BDDToGenerator::BDDToGenerator(FOBDDManager* manager) :
		_manager(manager) {
}

void extractFirstOccurringOutputs(const BddGeneratorData& data, const vector<unsigned int>& firstocc, vector<const DomElemContainer*>& outvars,
		vector<SortTable*>& tables) {
	for (unsigned int n = 0; n < data.pattern.size(); ++n) {
		if (data.pattern[n] == Pattern::OUTPUT) {
			if (firstocc[n] == n) {
				outvars.push_back(data.vars[n]);
				tables.push_back(data.universe.tables()[n]);
			}
		}
	}
}

InstGenerator* BDDToGenerator::create(const BddGeneratorData& data) {
	assert(data.check());
	vector<unsigned int> firstocc = detectDoubleOccurences(data.vars);

	if (data.bdd == _manager->falsebdd()) {
		return new EmptyGenerator();
	}

	if (data.bdd == _manager->truebdd()) {
		vector<const DomElemContainer*> outvars;
		vector<SortTable*> tables;
		extractFirstOccurringOutputs(data, firstocc, outvars, tables);
		return GeneratorFactory::create(outvars, tables);
	}

	return new TreeInstGenerator(createnode(data));
}

GeneratorNode* BDDToGenerator::createnode(const BddGeneratorData& data) {
	assert(data.check());
	vector<unsigned int> firstocc = detectDoubleOccurences(data.vars);

	if (data.bdd == _manager->falsebdd()) {
		return new LeafGeneratorNode(new EmptyGenerator());
	}

	if (data.bdd == _manager->truebdd()) {
		vector<const DomElemContainer*> outvars;
		vector<SortTable*> tables;
		extractFirstOccurringOutputs(data, firstocc, outvars, tables);
		return new LeafGeneratorNode(GeneratorFactory::create(outvars, tables));
	}

	// Otherwise: recursive case

	BddGeneratorData branchdata = data;
	branchdata.pattern.clear();

	// split variables into kernel and branch and also extract first-occurring kernel output variables
	vector<Pattern> kernpattern;
	vector<const DomElemContainer*> kernvars, kernoutputvars;
	vector<const FOBDDVariable*> kernbddvars;
	vector<SortTable*> kerntables, kernoutputtables;
	for (unsigned int n = 0; n < data.pattern.size(); ++n) {
		if (_manager->contains(data.bdd->kernel(), data.bddvars[n])) {
			kernpattern.push_back(data.pattern[n]);
			kernvars.push_back(data.vars[n]);
			if (data.pattern[n] == Pattern::OUTPUT) {
				bool firstocc = true;
				for (uint m = 0; m < kernoutputvars.size(); ++m) {
					if (kernoutputvars[m] == kernvars[n]) {
						firstocc = false;
						break;
					}
				}
				if (firstocc) {
					kernoutputvars.push_back(data.vars[n]);
					kernoutputtables.push_back(data.universe.tables()[n]);
				}
			}
			kernbddvars.push_back(data.bddvars[n]);
			kerntables.push_back(data.universe.tables()[n]);
			branchdata.pattern.push_back(Pattern::INPUT);
		} else {
			branchdata.pattern.push_back(data.pattern[n]);
		}
	}

	if (data.bdd->falsebranch() == _manager->falsebdd()) {
		// Only generate the true branch possibilities
		branchdata.bdd = data.bdd->truebranch();
		auto kernelgenerator = createFromKernel(data.bdd->kernel(), kernpattern, kernvars, kernbddvars, data.structure, false, Universe(kerntables));
		auto truegenerator = createnode(branchdata);
		return new OneChildGeneratorNode(kernelgenerator, truegenerator);
	}

	if (data.bdd->truebranch() == _manager->falsebdd()) {
		// Only generate the false branch possibilities
		branchdata.bdd = data.bdd->falsebranch();
		auto kernelgenerator = createFromKernel(data.bdd->kernel(), kernpattern, kernvars, kernbddvars, data.structure, true, Universe(kerntables));
		auto falsegenerator = createnode(branchdata);
		return new OneChildGeneratorNode(kernelgenerator, falsegenerator);
	}

	// Both branches possible: create a checker and a generator for all possibilities

	vector<Pattern> checkerpattern(kernpattern.size(), Pattern::INPUT);
	auto kernelchecker = createFromKernel(data.bdd->kernel(), checkerpattern, kernvars, kernbddvars, data.structure, false, Universe(kerntables));

	auto kernelgenerator = GeneratorFactory::create(kernoutputvars, kernoutputtables);

	branchdata.bdd = data.bdd->falsebranch();
	auto truegenerator = createnode(branchdata);

	branchdata.bdd = data.bdd->truebranch();
	auto falsegenerator = createnode(branchdata);

	return new TwoChildGeneratorNode(kernelchecker, kernelgenerator, falsegenerator, truegenerator);
}

// FIXME error in removenesting if this does not introduce a quantifier
// FIXME very ugly code
// FIXME a code in BDDTOGenerator that does not take a bdd and does not return something with bdds?
// TODO what should the method do exactly?
InstGenerator* BDDToGenerator::createFromPredForm(PredForm* atom, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const vector<Variable*>& atomvars, AbstractStructure* structure, bool inverse, const Universe& universe) {
	if (FormulaUtils::containsFuncTerms(atom)) {
		bool allinput = true;
		for (auto it = pattern.cbegin(); it != pattern.cend(); ++it) {
			if (*it == Pattern::OUTPUT) {
				allinput = false;
				break;
			}
		}
		if (allinput) {
			for (unsigned int n = 0; n < pattern.size(); ++n) {
				Term* solvedterm = solve(*_manager, atom, atomvars[n]);
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
							if (pattern[n] == Pattern::OUTPUT) {
								Term* solvedterm = solve(*_manager, atom, atomvars[n]);
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
							if (pattern[n] == Pattern::OUTPUT) {
								Term* solvedterm = solve(*_manager, atom, atomvars[n]);
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
		Formula* newform = FormulaUtils::unnestTerms(atom, Context::NEGATIVE);
		newform = FormulaUtils::splitComparisonChains(newform);
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
			if (pattern[n] == Pattern::OUTPUT) {
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
				generators.push_back(createFromPredForm(*it, kernpattern, kernvars, kernfovars, structure, inverse, Universe(kerntables)));
			else
				generators.push_back(createFromPredForm(*it, kernpattern, kernvars, kernfovars, structure, false, Universe(kerntables)));
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
					if (atomvars[pos] == var) break;
				}assert(pos < pattern.size());
				atompattern.push_back(pattern[pos]);
				datomvars.push_back(vars[pos]);
				atomtables.push_back(universe.tables()[pos]);
			} else if (typeid(*(*it)) == typeid(DomainTerm)) {
				DomainTerm* domterm = dynamic_cast<DomainTerm*>(*it);
				const DomElemContainer* domelement = new const DomElemContainer();
				*domelement = domterm->value();

				Variable* var = new Variable(domterm->sort());
				PredForm* newatom = dynamic_cast<PredForm*>(FormulaUtils::substituteTerm(atom, domterm, var));

				vector<Pattern> termpattern(pattern);
				termpattern.push_back(Pattern::INPUT);
				vector<const DomElemContainer*> termvars(vars);
				termvars.push_back(domelement);
				vector<Variable*> fotermvars(atomvars);
				fotermvars.push_back(var);
				vector<SortTable*> termuniv(universe.tables());
				termuniv.push_back(structure->inter(domterm->sort()));

				return createFromPredForm(newatom, termpattern, termvars, fotermvars, structure, inverse, Universe(termuniv));
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

InstGenerator* BDDToGenerator::createFromKernel(const FOBDDKernel* kernel, const vector<Pattern>& origpattern,
		const vector<const DomElemContainer*>& origvars, const vector<const FOBDDVariable*>& origkernelvars, AbstractStructure* structure,
		bool generateFalsebranch, const Universe& origuniverse) {

	if (sametypeid<FOBDDAtomKernel>(*kernel)) {
		auto atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);

		if (_manager->containsFuncTerms(atom)) {
			auto atomform = _manager->toFormula(atom);
			assert(typeid(*atomform) == typeid(PredForm));
			// TODO why is this the case?
			auto pf = dynamic_cast<PredForm*>(atomform);
			vector<Variable*> atomvars;
			for (auto it = origkernelvars.cbegin(); it != origkernelvars.cend(); ++it) {
				atomvars.push_back((*it)->variable());
			}
			return createFromPredForm(pf, origpattern, origvars, atomvars, structure, generateFalsebranch, origuniverse);
		}

		// Replace all fobbddomainterms with an instantiated variable (necessary for the generators)
		auto newkernelvars = origkernelvars;
		auto newuniverse = origuniverse;
		auto newpattern = origpattern;
		auto newvars = origvars;
		for (auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
			if (not sametypeid<FOBDDDomainTerm>(**it)) {
				continue;
			}
			auto domterm = dynamic_cast<const FOBDDDomainTerm*>(*it);
			auto domelement = new const DomElemContainer();
			*domelement = domterm->value();

			auto termvar = new Variable(domterm->sort());
			auto bddtermvar = _manager->getVariable(termvar);
			auto termkernel = _manager->substitute(kernel, domterm, bddtermvar); // NOTE: should not introduce quantifications!
			assert(sametypeid<FOBDDAtomKernel>(*termkernel));

			newpattern.push_back(Pattern::INPUT);
			newvars.push_back(domelement);
			newkernelvars.push_back(bddtermvar);
			newuniverse.addTable(structure->inter(domterm->sort()));

			atom = dynamic_cast<const FOBDDAtomKernel*>(termkernel);
			it = atom->args().cbegin();
		}

		// Create the pattern for the atom
		vector<Pattern> atompattern;
		vector<const DomElemContainer*> atomvars;
		vector<SortTable*> atomtables;
		for (auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
			// An atomkernel without functerms can only have variables and domainterms (and the last have been removed)
			assert(sametypeid<FOBDDVariable>(**it));
			auto var = dynamic_cast<const FOBDDVariable*>(*it);
			unsigned int pos = 0;
			for (; pos < newkernelvars.size(); ++pos) {
				if (newkernelvars[pos] == var) {
					break;
				}
			}

			assert(pos < newkernelvars.size());
			// Each variable in the atomkernel should occur in kernelvars
			atompattern.push_back(newpattern[pos]);
			atomvars.push_back(newvars[pos]);
			atomtables.push_back(newuniverse.tables()[pos]);
		}

		// Construct the generator
		auto symbol = atom->symbol();
		const PredInter* inter = NULL;
		if (sametypeid<Predicate>(*symbol)) {
			inter = structure->inter(dynamic_cast<Predicate*>(symbol));
		} else {
			assert(sametypeid<Function>(*symbol));
			inter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		}
		const PredTable* table = 0;
		switch (atom->type()) {
		case AtomKernelType::AKT_TWOVALUED:
			table = generateFalsebranch ? inter->cf() : inter->ct();
			break;
		case AtomKernelType::AKT_CT:
			table = generateFalsebranch ? inter->pf() : inter->ct();
			break;
		case AtomKernelType::AKT_CF:
			table = generateFalsebranch ? inter->pt() : inter->cf();
			break;
		}
		return GeneratorFactory::create(table, atompattern, atomvars, Universe(atomtables));
	}

	// Quantification kernel
	assert(sametypeid<FOBDDQuantKernel>(*kernel));
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
	if (generateFalsebranch) { // NOTE if generating the false branch, we implement a generator for the universe and check the false branch
		quantdata.pattern = vector<Pattern>(origpattern.size(), Pattern::INPUT);
	} else {
		quantdata.pattern = origpattern;
	}
	quantdata.pattern.push_back(Pattern::OUTPUT);

	quantdata.vars = origvars;
	quantdata.vars.push_back(new const DomElemContainer());

	quantdata.bddvars = origkernelvars;
	quantdata.bddvars.push_back(bddquantvar);

	quantdata.universe = origuniverse.tables();
	quantdata.universe.addTable(structure->inter(quantkernel->sort()));

	BDDToGenerator btg(_manager);

	// Create a generator for the kernel
	if (generateFalsebranch) {
		vector<const DomElemContainer*> univgenvars;
		vector<SortTable*> univgentables;
		for (unsigned int n = 0; n < quantdata.pattern.size(); ++n) {
			if (quantdata.pattern[n] == Pattern::OUTPUT) {
				univgenvars.push_back(quantdata.vars[n]);
				univgentables.push_back(quantdata.universe.tables()[n]);
			}
		}
		auto univgenerator = GeneratorFactory::create(univgenvars, univgentables);
		auto bddtruechecker = btg.create(quantdata);
		return new FalseQuantKernelGenerator(univgenerator, bddtruechecker);
	} else {
		auto quantgenerator = btg.create(quantdata);
		return new TrueQuantKernelGenerator(quantgenerator);
	}
}
