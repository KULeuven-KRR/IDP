/************************************
 DefinitionGrounders.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef DEFINITIONGROUNDERS_HPP_
#define DEFINITIONGROUNDERS_HPP_

#include "ground.hpp"

/*** Definition grounders ***/

/** Grounder for a head of a rule **/
class HeadGrounder {
	private:
		AbstractGroundTheory*			_grounding;
		std::vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*				_truechecker;
		InstanceChecker*				_falsechecker;
		unsigned int					_symbol;
		std::vector<SortTable*>			_tables;
	public:
		HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s,
					const std::vector<TermGrounder*>&, const std::vector<SortTable*>&);
		int	run() const;

};

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;
		InstGenerator*		_bodygenerator;
		GroundingContext	_context;
	public:
		RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
			: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) { }
		bool run(unsigned int defid, GroundDefinition* grounddefinition) const;

		// Mutators
		void addTrueRule(GroundDefinition* grounddefinition, int head) const;
		void addFalseRule(GroundDefinition* grounddefinition, int head) const;
		void addPCRule(GroundDefinition* grounddefinition, int head, const std::vector<int>& body, bool conj, bool recursive) const;
		void addAggRule(GroundDefinition* grounddefinition, int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) const;
};

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

#endif /* DEFINITIONGROUNDERS_HPP_ */
