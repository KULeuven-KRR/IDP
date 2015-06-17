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

#include "commontypes.hpp"

#include "theory/theory.hpp"
#include "theory/ecnf.hpp"
#include "visitors/VisitorFriends.hpp"
#include "inferences/grounding/GroundUtils.hpp" // TODO for structureinfo

class GroundTranslator;
struct SymbolOffset;
struct GroundTerm;

//FIXME definition numbers are passed directly to the solver. In future, solver input change might render this invalid

struct LazyInstantiation;
class DelayGrounder;
class GenerateBDDAccordingToBounds;
class LazyGroundingManager;

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory: public AbstractTheory {
	VISITORS()
private:
	Structure* _structure; // OWNER! The ground theory might be partially reduced with respect to this structure.

	GroundTranslator* _translator; //!< Link between ground atoms and SAT-solver literals.

public:
	AbstractGroundTheory(StructureInfo structure);
	AbstractGroundTheory(Vocabulary* voc, StructureInfo structure);
	void initializeTheory();

	~AbstractGroundTheory();

	// Returns the size in number of atoms the theory contains
	virtual long getSize() = 0;

	void addEmptyClause();
	void addUnitClause(Lit l);
	virtual void add(const GroundClause& cl, bool skipfirst = false) = 0;
	virtual void add(const GroundDefinition& d) = 0;
	virtual void add(DefId defid, const PCGroundRule& rule) = 0;
	virtual void add(GroundFixpDef*) = 0;
	virtual void add(Lit head, AggTsBody* body) = 0;
	virtual void add(Lit tseitin, CPTsBody* body) = 0;
	virtual void add(SetId setnr, DefId defnr, bool weighted) = 0;
	virtual void add(const Lit& head, TsType tstype, const litlist& clause, bool conj, DefId defnr) = 0;

	virtual void addOptimization(AggFunction, SetId) = 0;
	virtual void addOptimization(VarId) = 0;

	virtual void startLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction) = 0;
	virtual void notifyLazyAddition(const litlist& glist, int ID) = 0;
	virtual void notifyLazyResidual(LazyInstantiation* inst, TsType type) = 0;
	virtual void addLazyElement(Lit head, PFSymbol* symbol, const std::vector<GroundTerm>& args, bool recursive) = 0;

	virtual void notifyLazyWatch(Atom atom, TruthValue watches, LazyGroundingManager* manager) = 0;

	//NOTE: have to call these!
	//TODO check whether they are called correctly (currently in theorygrounder->run), but probably missing several usecases
	virtual void closeTheory() = 0;

	GroundTranslator* translator() const {
		return _translator;
	}
	Structure* structure() const {
		return _structure;
	}
	AbstractGroundTheory* clone() const;
};
