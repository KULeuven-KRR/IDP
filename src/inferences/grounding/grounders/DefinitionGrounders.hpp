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

#ifndef DEFINITIONGROUNDERS_HPP_
#define DEFINITIONGROUNDERS_HPP_

#include "Grounder.hpp"
#include "inferences/grounding/GroundUtils.hpp"

#include "groundtheories/SolverTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp" // TODO Only for symboloffset
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
class PFSymbol;

class RuleGrounder;

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
	// NOTE: passed definition ownership
	DefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<RuleGrounder*> subgr, const GroundingContext& context);
	~DefinitionGrounder();
	void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request);
	DefId getDefinitionID() const {
		return getContext().getCurrentDefID();
	}

	virtual void put(std::ostream&) const;

	const std::vector<RuleGrounder*>& getSubGrounders() const{
		return _subgrounders;
	}

	void removeGrounder(RuleGrounder* grounder){
		for(auto i=_subgrounders.begin(); i<_subgrounders.cend(); ++i){
			if(*i==grounder){
				_subgrounders.erase(i);
				break;
			}
		}
	}
};

class HeadGrounder;

// NOTE: any rule grounder NOT guaranteed to add false for all false defineds, should request adding them to the groundtheory!
class RuleGrounder {
private:
	Rule* _origrule;

	HeadGrounder* _headgrounder;
	FormulaGrounder* _bodygrounder;
	InstGenerator* _nonheadgenerator;
	GroundingContext _context;

public:
	RuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct);
	virtual ~RuleGrounder();

	virtual void run(DefId defid, GroundDefinition* grounddefinition, LazyGroundingRequest& request) const = 0;

	virtual void groundForSetHeadInstance(GroundDefinition& def, const Lit& head, const ElementTuple& headinst) = 0;

	void put(std::ostream& stream) const;

	tablesize getMaxGroundSize() const;

	const std::vector<const DomElemContainer*>& getHeadVarContainers() const;

	Rule* getRule() const{
		return _origrule;
	}

	HeadGrounder* headgrounder() const {
		return _headgrounder;
	}
	FormulaGrounder* bodygrounder() const {
		return _bodygrounder;
	}
	InstGenerator* bodygenerator() const { // Generates all variables occurring root in the body
		return _nonheadgenerator;
	}
	GroundingContext context() const {
		return _context;
	}

	GroundTranslator* translator() const;


	PredForm* getHead() const {
		return _origrule->head();
	}

	int verbosity() const;
};

class FullRuleGrounder: public RuleGrounder {
private:
	InstGenerator* _headgenerator;
	std::vector<std::vector<uint> > samevalues;

	mutable bool done;
	void notifyRun() const {
		done = true;
	}
	bool hasRun() const {
		return done;
	}

protected:
	InstGenerator* headgenerator() const {
		return _headgenerator;
	}

public:
	FullRuleGrounder(const Rule* rule, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct);
	virtual ~FullRuleGrounder();

	virtual void run(DefId defid, GroundDefinition* grounddefinition, LazyGroundingRequest& request) const;

	virtual void groundForSetHeadInstance(GroundDefinition& def, const Lit& head, const ElementTuple& headinst);
};

/** Grounder for a head of a rule **/
class HeadGrounder {
private:
	AbstractGroundTheory* _grounding;
	std::vector<TermGrounder*> _subtermgrounders;
	SymbolOffset _symbol; // Stored for efficiency
	std::vector<SortTable*> _tables;
	PFSymbol* _pfsymbol;
	GroundingContext _context;

	std::vector<const DomElemContainer*> _headvarcontainers;

public:
	HeadGrounder(AbstractGroundTheory* gt, PFSymbol* s, const std::vector<TermGrounder*>&,
			const std::vector<SortTable*>&, GroundingContext& context);
	~HeadGrounder();

	Lit run() const;

	GroundTranslator* translator() const;

	const std::vector<const DomElemContainer*>& getHeadVarContainers() const;

	const std::vector<TermGrounder*>& subtermgrounders() const {
		return _subtermgrounders;
	}
	PFSymbol* pfsymbol() const {
		return _pfsymbol;
	}
	AbstractGroundTheory* grounding() const {
		return _grounding;
	}

	Universe getUniverse() const {
		return Universe(_tables);
	}
};

#endif /* DEFINITIONGROUNDERS_HPP_ */
