/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDERFACTORY_HPP
#define GROUNDERFACTORY_HPP

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
class AbstractStructure;
class AbstractGroundTheory;
class InstGenerator;
class InstChecker;
class DomElemContainer;

class InteractivePrintMonitor;
class TermGrounder;
class SetGrounder;
class QuantSetGrounder;
class HeadGrounder;
class RuleGrounder;
class FormulaGrounder;
class GenerateBDDAccordingToBounds;
class Grounder;
class Grounder;
class FOBDD;

struct GenAndChecker {
	const std::vector<const DomElemContainer*> _vars;
	InstGenerator* const _generator;
	InstChecker* const _checker;
	Universe _universe;

	GenAndChecker(const std::vector<const DomElemContainer*>& vars, InstGenerator* generator, InstChecker* checker, const Universe& universe)
			: 	_vars(vars),
				_generator(generator),
				_checker(checker),
				_universe(universe) {
	}
};

struct GroundInfo {
	AbstractTheory* theory;
	Term* minimizeterm;
	AbstractStructure* partialstructure;
	GenerateBDDAccordingToBounds* symbolicstructure;
	bool nbModelsEquivalent;

	GroundInfo(AbstractTheory* theory, AbstractStructure* partialstructure, GenerateBDDAccordingToBounds* symbolicstructure,
			bool nbModelsEquivalent, Term* minimizeterm = NULL)
			: 	theory(theory),
				minimizeterm(minimizeterm),
				partialstructure(partialstructure),
				symbolicstructure(symbolicstructure),
				nbModelsEquivalent(nbModelsEquivalent) {
	}
};

// NOTE: creates generators, which do a check on infinite grounding
struct GeneratorData { // NOTE: all have the same order!
	std::vector<SortTable*> tables;
	std::vector<Variable*> fovars, quantfovars;
	std::vector<Pattern> pattern;
	std::vector<const DomElemContainer*> containers;
};

class GrounderFactory: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool allowskolemize;
	std::map<Function*, Formula*> funcconstraints;
	AbstractTheory* _theory;
	Term* _minimizeterm;
	Vocabulary* _vocabulary;
	AbstractStructure* _structure; //!< The structure that will be used to reduce the grounding
	GenerateBDDAccordingToBounds* _symstructure; //!< Used approximation
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
	SetGrounder* _setgrounder;
	QuantSetGrounder* _quantsetgrounder;
	HeadGrounder* _headgrounder;
	RuleGrounder* _rulegrounder;
	Grounder* _topgrounder;

	void AggContext();
	void SaveContext(); // Push the current context onto the stack
	void RestoreContext(); // Set _context to the top of the stack and pop the stack
	void DeeperContext(SIGN sign);

	// Descend in the parse tree while taking care of the context
	template<typename T> void descend(T child);

	AbstractStructure* structure() const {
		return _structure;
	}

	const var2dommap& varmapping() const {
		return _varmapping;
	}

	DomElemContainer* createVarMapping(Variable * const var);

	template<class VarList>
	InstGenerator* createVarMapAndGenerator(const Formula* original, const VarList& vars);

	GeneratorData getPatternAndContainers(std::vector<Variable*> quantfovars, std::vector<Variable*> remvars);
	InstGenerator* getGenerator(Formula* subformula, TruthType generatortype, const GeneratorData& data);
	InstChecker* getChecker(Formula* subformula, TruthType generatortype, const GeneratorData& data);
	PredTable* createTable(Formula* subformula, TruthType type, const std::vector<Variable*>& quantfovars, bool approxvalue, const GeneratorData& data);
	template<typename OrigConstruct>
	GenAndChecker createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype);

	const FOBDD* improve(bool approxastrue, const FOBDD* bdd, const std::vector<Variable*>& fovars);

	template<typename Grounding>
	GrounderFactory(const GroundInfo& data, Grounding* grounding, bool nbModelsEquivalent);

	Grounder* getTopGrounder() const {
		Assert(_topgrounder!=NULL);
		return _topgrounder;
	}
	FormulaGrounder* getFormGrounder() {
		Assert(_formgrounder!=NULL);
		return _formgrounder;
	}
	SetGrounder* getSetGrounder() {
		Assert(_setgrounder!=NULL);
		return _setgrounder;
	}
	RuleGrounder* getRuleGrounder() {
		Assert(_rulegrounder!=NULL);
		return _rulegrounder;
	}
	TermGrounder* getTermGrounder() {
		Assert(_termgrounder!=NULL);
		return _termgrounder;
	}

public:
	virtual ~GrounderFactory();

	// Factory methods which return a Grounder able to generate the full grounding
	static Grounder* create(const GroundInfo& data);
	static Grounder* create(const GroundInfo& data, PCSolver* satsolver);
	static Grounder* create(const GroundInfo& data, InteractivePrintMonitor* printmonitor);

	bool recursive(const Formula*);

	void InitContext(); // Initialize the context - public for debugging purposes

	GroundingContext getContext() {
		return _context;
	}

protected:
	template<class GroundTheory>
	static Grounder* createGrounder(const GroundInfo& data, GroundTheory groundtheory);
	// IMPORTANT: only method (next to the visit methods) allowed to call "accept"!
	Grounder* ground();

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
	void createTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc);
	void createNonTopQuantGrounder(const QuantForm* qf, Formula* subformula, const GenAndChecker& gc);

	AggForm* rewriteSumOrCardIntoSum(AggForm* af, AbstractStructure* structure);
	void internalVisit(const PredForm* newaf);
};

#endif /* GROUNDERFACTORY_HPP */
