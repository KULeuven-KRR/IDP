/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef DEFINITIONGROUNDERS_HPP_
#define DEFINITIONGROUNDERS_HPP_

#include "Grounder.hpp"
#include "GroundUtils.hpp"

#include "groundtheories/SolverTheory.hpp"

class TermGrounder;
class InstChecker;
class InstanceChecker;
class GroundDefinition;
class FormulaGrounder;
class TermGrounder;
class InstGenerator;
class SetGrounder;
class PredInter;
class Formula;
class SortTable;
class GroundTranslator;
class GroundTermTranslator;
class PFSymbol;

class RuleGrounder;
typedef unsigned int DefId;

/** Grounder for a definition **/
// NOTE: definition printing code is based on the INVARIANT that a defintion is ALWAYS grounded as contiguous component: never ground def A a bit, then ground B, then return to A again (code should error on this)
// directly printing in idp language will be incorrect then.
// TODO optimize grounding of definitions by grouping rules with the same head and grounding them atom by atom
//	(using approximation to derive given a head query which bodies might be true). This would allow to write out rules without
//	first constructing and storing the full ground definition to remove duplicate heads
class DefinitionGrounder: public Grounder {
private:
	static DefId _currentdefnb;
	std::vector<RuleGrounder*> _subgrounders; //!< Grounders for the rules of the definition.
public:
	DefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<RuleGrounder*> subgr, const GroundingContext& context);
	~DefinitionGrounder();
	void run(ConjOrDisj& formula) const;
	DefId id() const {
		return context().getCurrentDefID();
	}

	virtual void put(std::ostream& stream) const{
		// TODO not yet implemented.
	}
};

class HeadGrounder;

class RuleGrounder {
private:
	Rule* origrule;

	HeadGrounder* _headgrounder;
	FormulaGrounder* _bodygrounder;
	InstGenerator* _bodygenerator;
	GroundingContext _context;

public:
	HeadGrounder* headgrounder() const {
		return _headgrounder;
	}
	FormulaGrounder* bodygrounder() const {
		return _bodygrounder;
	}
	InstGenerator* bodygenerator() const {
		return _bodygenerator;
	}
	GroundingContext context() const {
		return _context;
	}

public:
	RuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct);
	virtual ~RuleGrounder();
	virtual void run(DefId defid, GroundDefinition* grounddefinition) const = 0;

	void put(std::stringstream& stream);
};

class FullRuleGrounder: public RuleGrounder {
private:
	InstGenerator* _headgenerator;

protected:
	InstGenerator* headgenerator() const {
		return _headgenerator;
	}

public:
	FullRuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct);
	virtual ~FullRuleGrounder();
	virtual void run(DefId defid, GroundDefinition* grounddefinition) const;
};

/** Grounder for a head of a rule **/
class HeadGrounder {
private:
	AbstractGroundTheory* _grounding;
	std::vector<TermGrounder*> _subtermgrounders;
	const PredTable* _ct;
	const PredTable* _cf;
	unsigned int _symbol;
	std::vector<SortTable*> _tables;
	PFSymbol* _pfsymbol;

public:
	HeadGrounder(AbstractGroundTheory* gt, const PredTable* ct, const PredTable* cf, PFSymbol* s, const std::vector<TermGrounder*>&, const std::vector<SortTable*>&);
	~HeadGrounder();
	Lit run() const;

	const std::vector<TermGrounder*>& subtermgrounders() const {
		return _subtermgrounders;
	}
	PFSymbol* pfsymbol() const {
		return _pfsymbol;
	}
	AbstractGroundTheory* grounding() const {
		return _grounding;
	}
};

#endif /* DEFINITIONGROUNDERS_HPP_ */
