/************************************
 DefinitionGrounders.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef DEFINITIONGROUNDERS_HPP_
#define DEFINITIONGROUNDERS_HPP_

#include "ground.hpp"

/*** Definition grounders ***/

class RuleGrounder;
typedef GroundTheory<SolverPolicy> SolverTheory;

/** Grounder for a definition **/
// NOTE: definition printing code is based on the INVARIANT that a defintion is ALWAYS grounded as contiguous component: never ground def A a bit, then ground B, then return to A again (code should error on this)
// directly printing in idp language will be incorrect then.
// TODO optimize grounding of definitions by grouping rules with the same head and grounding them atom by atom
//	(using approximation to derive given a head query which bodies might be true). This would allow to write out rules without
//	first constructing and storing the full ground definition to remove duplicate heads
class DefinitionGrounder : public TopLevelGrounder {
	private:
		static unsigned int _currentdefnb;
		unsigned int _defnb;
		std::vector<RuleGrounder*>	_subgrounders;	//!< Grounders for the rules of the definition.
		GroundDefinition* _grounddefinition;
	public:
		DefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<RuleGrounder*> subgr,int verb);
		bool run() const;
		unsigned int id() const { return _defnb; }
};

class HeadGrounder;

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;
		InstGenerator*		_bodygenerator;
		GroundingContext	_context;

	protected:
		HeadGrounder* 		headgrounder() const { return _headgrounder; }
		FormulaGrounder* 	bodygrounder() const { return _bodygrounder; }
		InstGenerator* 		headgenerator() const { return _headgenerator; }
		InstGenerator* 		bodygenerator() const { return _bodygenerator; }
		GroundingContext	context() const { return _context; }

	public:
		RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct);
		virtual void run(unsigned int defid, GroundDefinition* grounddefinition) const;
};

/** Grounder for a head of a rule **/
class HeadGrounder {
	private:
		AbstractGroundTheory*			_grounding;
		std::vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*				_truechecker;
		InstanceChecker*				_falsechecker;
		unsigned int					_symbol;
		std::vector<SortTable*>			_tables;
		PFSymbol*						_pfsymbol;

	public:
		HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s,
					const std::vector<TermGrounder*>&, const std::vector<SortTable*>&);
		int	run() const;

		const std::vector<TermGrounder*>& subtermgrounders() const { return _subtermgrounders; }
		PFSymbol* pfsymbol() const { return _pfsymbol; }
		AbstractGroundTheory* grounding() const { return _grounding; }
};

class LazyRuleGrounder;

class LazyDefinitionGrounder : public TopLevelGrounder {
	private:
		unsigned int _defnb;
		std::vector<LazyRuleGrounder*>	_subgrounders;	//!< Grounders for the rules of the definition.
	public:
		LazyDefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<LazyRuleGrounder*> subgr,int verb);
		bool run() const;
		unsigned int id() const { return _defnb; }
};

class LazyRuleGrounder: public RuleGrounder{
private:
	SolverTheory* _grounding;
	SolverTheory* grounding() const { return _grounding; }
public:
	LazyRuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct);
	void run(unsigned int defid, GroundDefinition* grounddefinition) const;

	void ground(const Lit& head, const ElementTuple& headargs);
	void notify(const Lit& lit, const ElementTuple& headargs);

	dominstlist createInst(const ElementTuple& headargs);
};

#endif /* DEFINITIONGROUNDERS_HPP_ */
