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
#include "Utils.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "utils/ListUtils.hpp"

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
class HeadGrounder;
class RuleGrounder;
class FormulaGrounder;
class GenerateBDDAccordingToBounds;
class Grounder;
class FOBDD;

struct GenAndChecker {
	const std::vector<const DomElemContainer*> _vars;
	InstGenerator* const _generator;
	InstChecker* const _checker;
	Universe _universe;

	GenAndChecker(const std::vector<const DomElemContainer*>& vars, InstGenerator* generator, InstChecker* checker, const Universe& universe)
			: _vars(vars), _generator(generator), _checker(checker), _universe(universe) {
	}
};

struct GroundStructureInfo{
	AbstractStructure* partialstructure;
	GenerateBDDAccordingToBounds* symbolicstructure;
};

struct GroundInfo{
	const AbstractTheory* theory;
	AbstractStructure* partialstructure;
	GenerateBDDAccordingToBounds* symbolicstructure;
};

class GrounderFactory: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	AbstractStructure* _structure; //!< The structure that will be used to reduce the grounding
	GenerateBDDAccordingToBounds* _symstructure; //!< Used approximation
	AbstractGroundTheory* _grounding; //!< The ground theory that will be produced

	GroundingContext _context;
	std::stack<GroundingContext> _contextstack;

	// Variable mapping
	var2dommap _varmapping; // Maps variables to their counterpart during grounding.
							// That is, the corresponding const DomElemContainer* acts as a variable+value.

	// Return values
	FormulaGrounder* _formgrounder;
	TermGrounder* _termgrounder;
	SetGrounder* _setgrounder;
	HeadGrounder* _headgrounder;
	RuleGrounder* _rulegrounder;
	Grounder* _topgrounder;

	void AggContext();
	void SaveContext(); // Push the current context onto the stack
	void RestoreContext(); // Set _context to the top of the stack and pop the stack
	void DeeperContext(SIGN sign);

	// Descend in the parse tree while taking care of the context
	template <typename T> void descend(T* child);

	AbstractStructure* structure() const {
		return _structure;
	}

	const var2dommap& varmapping() const {
		return _varmapping;
	}

	DomElemContainer* createVarMapping(Variable * const var);

	template<class VarList>
	InstGenerator* createVarMapAndGenerator(const Formula* original, const VarList& vars);

	// NOTE: creates generators, which do a check on infinite grounding
	template<typename OrigConstruct>
	GenAndChecker createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype);

	const FOBDD* improveGenerator(const FOBDD*, const std::vector<Variable*>&, double);
	const FOBDD* improveChecker(const FOBDD*, double);

	template<typename Grounding>
	GrounderFactory(const GroundStructureInfo& data, Grounding* grounding);

	Grounder* getTopGrounder() const { return _topgrounder; }
	FormulaGrounder* getFormGrounder() {
		return _formgrounder;
	}
	SetGrounder* getSetGrounder() {
		return _setgrounder;
	}

public:
	virtual ~GrounderFactory();

	// Factory methods which return a toplevelgrounder able to generate the full grounding
	static Grounder* create(const GroundInfo& data);
	static Grounder* create(const GroundInfo& data, PCSolver* satsolver);
	//static Grounder* create(const GroundInfo& data, FZRewriter* flatzincprinter);
	static Grounder* create(const GroundInfo& data, InteractivePrintMonitor* printmonitor);
	static SetGrounder* create(const SetExpr* set, const GroundStructureInfo& data, AbstractGroundTheory* grounding);

	bool recursive(const Formula*);

	void InitContext(); // Initialize the context - public for debugging purposes

	GroundingContext getContext() {
		return _context;
	}

protected:
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
};

#endif /* GROUNDERFACTORY_HPP */
