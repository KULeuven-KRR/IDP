#include <ctime>
#include "ModelExpansion.hpp"

#include "monitors/tracemonitor.hpp"
#include "commands/propagate.hpp"
#include "symmetry.hpp"

#include "utils/TheoryUtils.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "OptionsStack.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

#include "external/TerminationManagement.hpp"

using namespace std;

class SolverTermination: public TerminateMonitor{
public:
	void notifyTerminateRequested(){
		requestTermination();
	}
};

std::vector<AbstractStructure*> ModelExpansion::expand(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* monitor) const {
	auto opts = GlobalData::instance()->getOptions();
	// TODO Option::pushOptions(options);

	// Calculate known definitions
	// FIXME currently skipping if working lazily!
	if (not opts->getValue(BoolType::GROUNDLAZILY) && sametypeid<Theory>(*theory)) {
		bool satisfiable = calculateKnownDefinitions(dynamic_cast<Theory*>(theory), structure);
		if (not satisfiable) {
			return std::vector<AbstractStructure*> { };
		}
	}

	// Create solver and grounder
	SATSolver* solver = createsolver();
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		clog <<"Approximation\n";
	}
	auto symstructure = generateNaiveApproxBounds(theory, structure);
	// TODO bugged! auto symstructure = generateApproxBounds(theory, structure);
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		clog <<"Grounding\n";
	}
	GrounderFactory grounderfactory(structure, symstructure);
	Grounder* grounder = grounderfactory.create(theory, solver);
	grounder->toplevelRun();
	AbstractGroundTheory* grounding = grounder->grounding();

	// Execute symmetry breaking
	if (opts->getValue(IntType::SYMMETRY) != 0) {
		if(getOption(IntType::GROUNDVERBOSITY)>1){
			clog <<"Symmetry breaking\n";
		}
		clock_t start = clock();
		auto ivsets = findIVSets(theory, structure);
		float time = (float) (clock() - start) / CLOCKS_PER_SEC;
		if (opts->getValue(IntType::SYMMETRY) == 1) {
			addSymBreakingPredicates(grounding, ivsets);
		} else if (opts->getValue(IntType::SYMMETRY) == 2) {
			for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
				std::vector<std::map<int, int> > breakingSymmetries = (*ivsets_it)->getBreakingSymmetries(grounding);
				for (auto bs_it = breakingSymmetries.cbegin(); bs_it != breakingSymmetries.cend(); ++bs_it) {
					MinisatID::Symmetry symmetry;
					for (auto s_it = bs_it->begin(); s_it != bs_it->end(); ++s_it) {
						MinisatID::Atom a1 = MinisatID::Atom(s_it->first);
						MinisatID::Atom a2 = MinisatID::Atom(s_it->second);
						std::pair<MinisatID::Atom, MinisatID::Atom> entry = std::pair<MinisatID::Atom, MinisatID::Atom>(a1, a2);
						symmetry.symmetry.insert(entry);
					}
					solver->add(symmetry);
				}
			}
		} else {
			std::cerr << "Unknown symmetry option...\n";
		}
	}

	// Run solver
	MinisatID::Solution* abstractsolutions = initsolution();
	if (opts->getValue(BoolType::TRACE)) {
		monitor->setTranslator(grounding->translator());
		monitor->setSolver(solver);
	}
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		clog <<"Solving\n";
	}
	getGlobal()->addTerminationMonitor(new SolverTermination());
	solver->solve(abstractsolutions);
	if(getGlobal()->terminateRequested()){
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	//FIXME propagator code broken structure = propagator->currstructure(structure);
	std::vector<AbstractStructure*> solutions;
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		clog <<"Generate 2-valued models\n";
	}
	for (auto model = abstractsolutions->getModels().cbegin(); model != abstractsolutions->getModels().cend(); ++model) {
		AbstractStructure* newsolution = structure->clone();
		addLiterals(*model, grounding->translator(), newsolution);
		addTerms(*model, grounding->termtranslator(), newsolution);
		newsolution->clean();
		solutions.push_back(newsolution);
		Assert(newsolution->isConsistent());
	}

	grounding->recursiveDelete();
	delete (solver);
	delete (abstractsolutions);

	return solutions;
}

bool ModelExpansion::calculateDefinition(Definition* definition, AbstractStructure* structure) const {
	// Create solver and grounder
	SATSolver* solver = createsolver();
	Theory theory("", structure->vocabulary(), ParseInfo());
	theory.add(definition);

	auto symstructure = generateNaiveApproxBounds(&theory, structure);
	GrounderFactory grounderfactory(structure, symstructure);
	Grounder* grounder = grounderfactory.create(&theory, solver);

	grounder->toplevelRun();
	AbstractGroundTheory* grounding = dynamic_cast<GroundTheory<SolverPolicy>*>(grounder->grounding());

	// Run solver
	MinisatID::Solution* abstractsolutions = initsolution();
	solver->solve(abstractsolutions);
	if(getGlobal()->terminateRequested()){
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	if (abstractsolutions->getModels().empty()) {
		return false;
	} else {
		Assert(abstractsolutions->getModels().size() == 1);
		auto model = *(abstractsolutions->getModels().cbegin());
		addLiterals(model, grounding->translator(), structure);
		addTerms(model, grounding->termtranslator(), structure);
		structure->clean();
	}

	// Cleanup
	grounding->recursiveDelete();
	delete (solver);
	delete (abstractsolutions);

	return true;
}

bool ModelExpansion::calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const {
	// Collect the open symbols of all definitions
	std::map<Definition*, std::set<PFSymbol*> > opens;
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		opens[*it] = DefinitionUtils::opens(*it);
	}

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
			// Remove opens that have a two-valued interpretation
			for (auto symbol = currentdefinition->second.begin(); symbol != currentdefinition->second.end();) {
				auto currentsymbol = symbol++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
				if (structure->inter(*currentsymbol)->approxTwoValued()) {
					currentdefinition->second.erase(currentsymbol);
				}
			}
			// If no opens are left, calculate the interpretation of the defined atoms
			if (currentdefinition->second.empty()) {
				bool satisfiable = calculateDefinition(currentdefinition->first, structure);
				if (not satisfiable) {
					return false;
				}
				theory->remove(currentdefinition->first);
				opens.erase(currentdefinition);
				fixpoint = false;
			}
		}
	}
	return true;
}

SATSolver* ModelExpansion::createsolver() const {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::SolverOption modes;
	modes.nbmodels = options->getValue(IntType::NRMODELS);
	modes.verbosity = options->getValue(IntType::SATVERBOSITY);
	modes.polarity = options->getValue(BoolType::MXRANDOMPOLARITYCHOICE) ? MinisatID::POL_RAND : MinisatID::POL_STORED;

	if (options->getValue(BoolType::GROUNDLAZILY)) {
		modes.lazy = true;
	}

//		modes.remap = false; // FIXME no longer allowed, because solver needs the remapping for extra literals.
	return new SATSolver(modes);
}

MinisatID::Solution* ModelExpansion::initsolution() const {
	auto options = GlobalData::instance()->getOptions();
	MinisatID::ModelExpandOptions opts;
	opts.nbmodelstofind = options->getValue(IntType::NRMODELS);
	opts.printmodels = MinisatID::PRINT_NONE;
	opts.savemodels = MinisatID::SAVE_ALL;
	opts.search = MinisatID::MODELEXPAND;
	return new MinisatID::Solution(opts);
}

void ModelExpansion::addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const {
	for (auto literal = model->literalinterpretations.cbegin(); literal != model->literalinterpretations.cend(); ++literal) {
		int atomnr = literal->getAtom().getValue();

		if (translator->isInputAtom(atomnr)) {
			PFSymbol* symbol = translator->getSymbol(atomnr);
			const ElementTuple& args = translator->getArgs(atomnr);
			if (typeid(*symbol) == typeid(Predicate)) {
				Predicate* pred = dynamic_cast<Predicate*>(symbol);
				if (literal->hasSign()) {
					init->inter(pred)->makeFalse(args);
				} else {
					init->inter(pred)->makeTrue(args);
				}
			} else {
				Function* func = dynamic_cast<Function*>(symbol);
				if (literal->hasSign()) {
					init->inter(func)->graphInter()->makeFalse(args);
				} else {
					init->inter(func)->graphInter()->makeTrue(args);
				}
			}
		}
	}
}

void ModelExpansion::addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const {
	for (auto cpvar = model->variableassignments.cbegin(); cpvar != model->variableassignments.cend(); ++cpvar) {
		Function* function = termtranslator->function(cpvar->variable);
		if (function == NULL) {
			continue;
		}
		const auto& gtuple = termtranslator->args(cpvar->variable);
		ElementTuple tuple;
		for (auto it = gtuple.cbegin(); it != gtuple.cend(); ++it) {
			if (it->isVariable) {
				int value = model->variableassignments[it->_varid].value;
				tuple.push_back(createDomElem(value));
			} else {
				tuple.push_back(it->_domelement);
			}
		}
		tuple.push_back(createDomElem(cpvar->value));
		init->inter(function)->graphInter()->makeTrue(tuple);
	}
}
