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

#ifndef GROUNDING_GROUNDTHEORY_HPP_
#define GROUNDING_GROUNDTHEORY_HPP_

#include "IncludeComponents.hpp"
#include "AbstractGroundTheory.hpp"

struct LazyInstantiation;
struct GroundTerm;

template<class Policy>
class GroundTheory: public AbstractGroundTheory, public Policy {
	std::set<Lit> _addedTseitins; //!< Tseitin atoms produced by the translator that occur in the theory.
	std::set<SetId> _addedSets; //!< Set numbers produced by the translator that occur in the theory.
	std::set<int> _printedconstraints; //!< Atoms for which a connection to CP constraints are added.
	std::set<CPTerm*> _foldedterms;
	std::set<VarId> _printedvarids, _addedvarinterpretation;
	bool _nbModelsEquivalent;

	long _nbofatoms;
	void notifyAtomsAdded(long number){
		_nbofatoms += number; // TODO check overflow
	}

public:
	GroundTheory(StructureInfo structure, bool nbModelsEquivalent);
	GroundTheory(Vocabulary* voc, StructureInfo structure, bool nbModelsEquivalent);

	virtual ~GroundTheory() {
	}

	virtual long getSize(){
		return _nbofatoms;
	}

	void add(TheoryComponent*) {
		throw notyetimplemented("Cannot add components to a ground theory");
	}
	std::vector<TheoryComponent*> getComponents() const {
		throw notyetimplemented("Cannot retrieve components to a ground theory");
	}

	virtual void recursiveDelete();

	virtual void closeTheory();

	virtual void add(const GroundClause& cl, bool skipfirst = false);
	virtual void add(const GroundDefinition& def);
	virtual void add(DefId defid, const PCGroundRule& rule);
	virtual void add(GroundFixpDef*);
	virtual void add(Lit tseitin, CPTsBody* body);
	virtual void add(Lit tseitin, VarId varid);
	virtual void add(SetId setnr, DefId defnr, bool weighted);
	virtual void add(Lit head, AggTsBody* body);
	virtual void add(const Lit& head, TsType tstype, const litlist& clause, bool conj, DefId defnr);

	virtual void addOptimization(AggFunction function, SetId setid);
	virtual void addOptimization(VarId varid);

	virtual void startLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction);
	virtual void notifyLazyAddition(const litlist& glist, int ID);
	virtual void notifyLazyResidual(LazyInstantiation* inst, TsType type);
	virtual void addLazyElement(Lit head, PFSymbol* symbol, const std::vector<GroundTerm>& args, bool recursive);

	virtual void notifyLazyWatch(Atom atom, TruthValue watches, LazyGroundingManager* manager);

	void accept(TheoryVisitor* v) const;
	AbstractTheory* accept(TheoryMutatingVisitor* v);

	bool containsComplexDefinitions() const{
		return false;
	}

	std::ostream& put(std::ostream&) const{
		throw notyetimplemented("Printing of ground theories");
	}

protected:
	/**
	 * Adds the theory interpretation of tseitins that have not been added to the ground theory before.
	 */
	struct TseitinInfo{
		int lit;
		DefId defid;
		bool rootlevel;
	};
	std::queue<TseitinInfo> tseitinqueue; // pair of tseitin literal and its definition id
	bool addingTseitins;

	// If propagated is true, handles the literals as if they should already have been propagated true
	// in that case, if they are not dropped, they have to be added as a unit clause to the policy
	void addTseitinInterpretations(const litlist& vi, DefId defnr, bool skipfirst = false, bool propagated = false);

	void addVarIdInterpretation(VarId id);

	void addFoldedVarEquiv(VarId id);
	CPTerm* foldCPTerm(CPTerm* cpterm, DefId defnr);

private:
	void addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable);
};

#endif /* GROUNDING_GROUNDTHEORY_HPP_ */
