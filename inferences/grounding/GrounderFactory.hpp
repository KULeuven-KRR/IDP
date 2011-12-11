#ifndef GROUND_HPP
#define GROUND_HPP

#include "commontypes.hpp"
#include <stack>
#include "theory.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "inferences/grounding/Utils.hpp"
#include "external/ExternalInterface.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"

class PFSymbol;
class Variable;
class FuncTable;
class AbstractStructure;
class AbstractGroundTheory;
class InstGenerator;
class InstChecker;
class InstanceChecker;
class SortTable;
class DomainElement;
class Options;

class LazyQuantGrounder;
class LazyRuleGrounder;

class InteractivePrintMonitor;
class TermGrounder;
class SetGrounder;
class HeadGrounder;
class RuleGrounder;
class FormulaGrounder;
class GenerateBDDAccordingToBounds;
class Grounder;
class FOBDD;

class GrounderFactory: public TheoryVisitor {
	VISITORFRIENDS()
private:
	// Data
	AbstractStructure* _structure; //!< The structure that will be used to reduce the grounding
	GenerateBDDAccordingToBounds* _symstructure; //!< Used approximation
	AbstractGroundTheory* _grounding; //!< The ground theory that will be produced

	// Options
	Options* _options;
	int _verbosity;
	bool _cpsupport;

	// Context
	GroundingContext _context;
	std::stack<GroundingContext> _contextstack;

	void AggContext();
	void SaveContext(); // Push the current context onto the stack
	void RestoreContext(); // Set _context to the top of the stack and pop the stack
	void DeeperContext(SIGN sign);

	// Descend in the parse tree while taking care of the context
	void descend(Formula* f);
	void descend(Term* t);
	void descend(Rule* r);
	void descend(SetExpr* s);

	// Symbols passed to CP solver
	std::set<const PFSymbol*> _cpsymbols;

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

	AbstractStructure* structure() const {
		return _structure;
	}

	const var2dommap& varmapping() const {
		return _varmapping;
	}
	//var2dommap& varmapping() { return _varmapping; }

	const DomElemContainer* createVarMapping(Variable * const var);
	template<class VarList>
	InstGenerator* createVarMapAndGenerator(const Formula* original, const VarList& vars);

	struct GenAndChecker {
		InstGenerator* _generator;
		InstChecker* _checker;

		GenAndChecker(InstGenerator* generator, InstChecker* checker) :
				_generator(generator), _checker(checker) {
		}
	};
	template<typename OrigConstruct>
	GenAndChecker createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype, TruthType checkertype);

	const FOBDD* improve_generator(const FOBDD*, const std::vector<Variable*>&, double);
	const FOBDD* improve_checker(const FOBDD*, double);

public:
	GrounderFactory(AbstractStructure* structure, GenerateBDDAccordingToBounds* symbstructure);
	virtual ~GrounderFactory() {
	}

	// Factory method
	Grounder* create(const AbstractTheory*);
	Grounder* create(const AbstractTheory*, MinisatID::WrappedPCSolver*);
	Grounder* create(const AbstractTheory*, InteractivePrintMonitor*);

	// Determine what should be passed to CP solver
	std::set<const PFSymbol*> findCPSymbols(const AbstractTheory*);
	bool isCPSymbol(const PFSymbol*) const;

	// Recursive check
	bool recursive(const Formula*);

	// Context
	void InitContext(); // Initialize the context - public for debugging purposes

	// Getters
	GroundingContext getContext(){
		return _context;
	}
	FormulaGrounder* getFormGrounder(){
		return _formgrounder;
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
};

#endif
