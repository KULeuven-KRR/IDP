/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDING_GROUNDTHEORY_HPP_
#define GROUNDING_GROUNDTHEORY_HPP_

#include "IncludeComponents.hpp"
#include "AbstractGroundTheory.hpp"

class ResidualAndFreeInst;
class LazyGroundingManager;

template<class Policy>
class GroundTheory: public AbstractGroundTheory, public Policy {
	std::set<Lit> _printedtseitins; //!< Tseitin atoms produced by the translator that occur in the theory.
	std::set<SetId> _printedsets; //!< Set numbers produced by the translator that occur in the theory.
	std::set<int> _printedconstraints; //!< Atoms for which a connection to CP constraints are added.
	std::set<CPTerm*> _foldedterms;
	std::set<VarId> _printedvarids;
	std::map<PFSymbol*, std::set<Atom> > _defined; //!< List of defined symbols and the heads which have a rule.

	std::set<PFSymbol*> needfalsedefinedsymbols;

public:
	GroundTheory(AbstractStructure* str);
	GroundTheory(Vocabulary* voc, AbstractStructure* str);

	virtual ~GroundTheory() {
	}

	void add(Formula*) {
		Assert(false);
	}
	void add(Definition*) {
		Assert(false);
	}
	void add(FixpDef*) {
		Assert(false);
	}

	virtual void recursiveDelete();

	virtual void closeTheory();

	virtual void add(const GroundClause& cl, bool skipfirst = false);
	virtual void add(const GroundDefinition& def);
	virtual void add(DefId defid, PCGroundRule* rule);
	virtual void add(GroundFixpDef*);
	virtual void add(Lit tseitin, CPTsBody* body);
	virtual void add(Lit setnr, DefId defnr, bool weighted);
	virtual void add(Lit head, AggTsBody* body);
	virtual void add(const Lit& head, TsType tstype, const litlist& clause, bool conj, DefId defnr);

	virtual void addOptimization(AggFunction function, SetId setid);
	virtual void addOptimization(VarId varid);

	virtual void notifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders);
	virtual void notifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder);

	std::ostream& put(std::ostream& s) const;

	void accept(TheoryVisitor* v) const;
	AbstractTheory* accept(TheoryMutatingVisitor* v);

	virtual void notifyNeedFalseDefineds(PFSymbol* pfs){
		needfalsedefinedsymbols.insert(pfs);
	}
	const std::set<PFSymbol*>& getNeedFalseDefinedSymbols() const { return needfalsedefinedsymbols; }

protected:
	void transformForAdd(const litlist& vi, VIType /*vit*/, DefId defnr, bool skipfirst = false);

	CPTerm* foldCPTerm(CPTerm* cpterm);

	void addFalseDefineds();

private:
	void notifyDefined(Atom inputatom);
	void addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable);
};

#endif /* GROUNDING_GROUNDTHEORY_HPP_ */
