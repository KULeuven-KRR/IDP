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

#pragma once

#include <stack>
#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "GroundUtils.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "utils/ListUtils.hpp"
#include "generators/InstGenerator.hpp" // For PATTERN
#include "inferences/SolverInclude.hpp"

class PFSymbol;
class Variable;
class Structure;
class AbstractGroundTheory;
class InstGenerator;
class InstChecker;
class DomElemContainer;

class InteractivePrintMonitor;
class TermGrounder;
class EnumSetGrounder;
class QuantSetGrounder;
class HeadGrounder;
class RuleGrounder;
class FormulaGrounder;
class GenerateBDDAccordingToBounds;
class Grounder;
class Grounder;
class FOBDD;

typedef std::shared_ptr<GenerateBDDAccordingToBounds> SymbolicStructure;

struct GenAndChecker {
	const std::set<const DomElemContainer*> _generates;
	InstGenerator* const _generator;
	InstChecker* const _checker;
	Universe _universe;

	GenAndChecker(const std::set<const DomElemContainer*>& outputvars, InstGenerator* generator, InstChecker* checker, const Universe& universe)
			: 	_generates(outputvars),
				_generator(generator),
				_checker(checker),
				_universe(universe) {
	}
};

struct GroundInfo {
	AbstractTheory* theory;
	Term* minimizeterm;
	StructureInfo structure;
	Vocabulary* outputvocabulary;
	bool nbModelsEquivalent;

	GroundInfo(AbstractTheory* theory, StructureInfo structure, Vocabulary* outputvocabulary,
			bool nbModelsEquivalent, Term* minimizeterm = NULL)
			: 	theory(theory),
				minimizeterm(minimizeterm),
				structure(structure),
				outputvocabulary(outputvocabulary),
				nbModelsEquivalent(nbModelsEquivalent) {
	}
};

// NOTE: creates generators, which do a check on infinite grounding
struct GeneratorData { // NOTE: all have the same order!
	std::vector<SortTable*> tables;
	std::vector<Variable*> fovars, quantfovars;
	std::vector<const DomElemContainer*> containers;
	const Structure* structure;
	Context funccontext;
};

class GrounderFactory: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::map<Function*, Formula*> funcconstraints;
	Vocabulary* _vocabulary;
	StructureInfo _structure;
	AbstractGroundTheory* _grounding; //!< The ground theory that will be produced
	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	GroundingContext _context;
	std::stack<GroundingContext> _contextstack;
	bool _nbmodelsequivalent; //If true, the produced grounding will have as many models as the original theory, if false, the grounding might have more models.

	// Variable mapping
	var2dommap _varmapping; // Maps variables to their counterpart during grounding.
							// That is, the corresponding const DomElemContainer* acts as a variable+value.

							// Return values
	FormulaGrounder* _formgrounder;
	TermGrounder* _termgrounder;
	EnumSetGrounder* _setgrounder;
	QuantSetGrounder* _quantsetgrounder;
	HeadGrounder* _headgrounder;
	RuleGrounder* _rulegrounder;
	Grounder* _topgrounder;

	LazyGroundingManager* _groundingmanager;

	void AggContext();
	void SaveContext(); // Push the current context onto the stack
	void RestoreContext(); // Set _context to the top of the stack and pop the stack
	void DeeperContext(SIGN sign);

	// Descend in the parse tree while taking care of the context
	template<typename T> void descend(T child);

	Structure* getConcreteStructure() const {
		return _structure.concrstructure;
	}

	SymbolicStructure getSymbolicStructure() const {
		return _structure.symstructure;
	}

	const var2dommap& varmapping() const {
		return _varmapping;
	}

	DomElemContainer* createVarMapping(Variable * const var);

	/**
	 * NOTE: safely-reuse indicates that the user of the generator knows that at least one other variable is already generated
	 * 	by some other generator.
	 */
	template<class VarList>
	InstGenerator* createVarMapAndGenerator(const Formula* original, const VarList& vars, bool safelyreuse = false);

	std::pair<GeneratorData, std::vector<Pattern>> getPatternAndContainers(std::vector<Variable*> quantfovars, std::vector<Variable*> remvars);
public:
	/**
	 * Create generator for the formula based on the SYMBOLIC structure
	 * Option "exact": for symbolic bounds, not allowed to simplify the bdd.
	 * 		This is essential for the translator, where it is crucial that the bounds used for generating are the same as the bounds used for checking.
	 */
	static InstGenerator* getGenerator(Formula* subformula, TruthType generatortype, const GeneratorData& data, const std::vector<Pattern>& pattern, SymbolicStructure symstructure, const Structure* structure, std::set<PFSymbol*> definedsymbols, bool exact = false);
	/**
	 * Create checker for the formula based on the SYMBOLIC structure
	 * Option "exact": for symbolic bounds, not allowed to simplify the bdd.
	 * 		This is essential for the translator, where it is crucial that the bounds used for generating are the same as the bounds used for checking.
	 */
	static InstChecker* getChecker(Formula* subformula, TruthType generatortype, const GeneratorData& data, SymbolicStructure symstructure, const Structure* structure, std::set<PFSymbol*> definedsymbols, bool exact = false);
private:
	static InstGenerator* createGen(const std::string& name, TruthType type, const GeneratorData& data, PredTable* table, Formula*,
			const std::vector<Pattern>& pattern);
	static PredTable* createTable(Formula* subformula, TruthType type, const std::vector<Variable*>& quantfovars, bool approxvalue, const GeneratorData& data,
			SymbolicStructure symstructure, const Structure* structure, std::set<PFSymbol*> definedsymbols, bool exact = false);
	template<typename OrigConstruct>
	GenAndChecker createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype);

	static const FOBDD* improve(bool approxastrue, const FOBDD* bdd, const std::vector<Variable*>& fovars, const Structure* structure, SymbolicStructure symstructure, std::set<PFSymbol*> definedsymbols);

	template<typename Grounding>
	GrounderFactory(const Vocabulary* outputvocabulary, StructureInfo structures, Grounding* grounding, bool nbModelsEquivalent);
	GrounderFactory(LazyGroundingManager* manager);

	Grounder* getTopGrounder() const;
	FormulaGrounder* getFormGrounder() const;
	EnumSetGrounder* getSetGrounder() const;
	RuleGrounder* getRuleGrounder() const;
	TermGrounder* getTermGrounder() const;

	void setTopGrounder(Grounder* grounder);

public:
	virtual ~GrounderFactory();

	// Factory methods which return a Grounder able to generate the full grounding
	static LazyGroundingManager* create(const GroundInfo& data);
	static LazyGroundingManager* create(const GroundInfo& data, PCSolver* satsolver);
	static LazyGroundingManager* create(const GroundInfo& data, InteractivePrintMonitor* printmonitor);

	static FormulaGrounder* createSentenceGrounder(LazyGroundingManager* manager, Formula* sentence);

	bool recursive(const Formula*);
	bool recursive(const Term*);

	void InitContext(); // Initialize the context - public for debugging purposes

	GroundingContext getContext() {
		return _context;
	}

protected:
	template<class GroundTheory>
	static LazyGroundingManager* createGrounder(const GroundInfo& data, GroundTheory groundtheory);
	// IMPORTANT: only method (next to the visit methods) allowed to call "accept"!
	LazyGroundingManager* ground(AbstractTheory* theory, Term* minimizeterm);

	void visit(const Theory*);

	void visit(const PredForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const EquivForm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);

	void visit(const VarTerm*);
	void visit(const DomainTerm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);

	void visit(const EnumSetExpr*);
	void visit(const QuantSetExpr*);

	void visit(const Definition*);
	void visit(const Rule*);

private:
	void createBoolGrounderConjPath(const BoolForm* bf);
	void createBoolGrounderDisjPath(const BoolForm* bf);

	FormulaGrounder* checkDenotationGrounder(const QuantForm* qf);
	void createTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc);
	void createNonTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc);

	void handleWithComparisonGenerator(const PredForm* pf);
	void handleGeneralPredForm(const PredForm* pf);
	void groundAggWithCP(SIGN sign, Term* bound, CompType comp, AggTerm* agg);
	void groundAggWithoutCP(bool antimono, bool recursive, SIGN sign, Term* bound, CompType comp, AggTerm* agg);
	AggForm* tryToTurnIntoAggForm(const PredForm* pf);

	static const FOBDD* simplify(const std::vector<Variable*>& fovars, std::shared_ptr<FOBDDManager> manager, bool approxastrue, const FOBDD* bdd,
			const std::set<PFSymbol*>& definedsymbols, double cost_per_answer, const Structure* structure);

	void checkAndAddAsTopGrounder();
};
