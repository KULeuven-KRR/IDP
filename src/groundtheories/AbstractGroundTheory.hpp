/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ABSTRACTGROUNDTHEORY_HPP_
#define ABSTRACTGROUNDTHEORY_HPP_

#include "commontypes.hpp"

#include "theory/theory.hpp"
#include "theory/ecnf.hpp"
#include "visitors/VisitorFriends.hpp"

class GroundTermTranslator;
class GroundTranslator;

//FIXME definition numbers are passed directly to the solver. In future, solver input change might render this invalid

// Enumeration used in GroundTheory::transformForAdd
enum VIType {
	VIT_DISJ, VIT_CONJ, VIT_SET
};

class ResidualAndFreeInst;
class LazyGroundingManager;
class LazyUnknBoundGrounder;

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory: public AbstractTheory {
	VISITORS()
private:
	AbstractStructure* _structure; //!< The ground theory may be partially reduced with respect to this structure.
	GroundTranslator* _translator; //!< Link between ground atoms and SAT-solver literals.
	GroundTermTranslator* _termtranslator; //!< Link between ground terms and CP-solver variables.

public:
	AbstractGroundTheory(AbstractStructure* str);
	AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str);

	~AbstractGroundTheory();

	void addEmptyClause();
	void addUnitClause(Lit l);
	virtual void add(const GroundClause& cl, bool skipfirst = false) = 0;
	virtual void add(const GroundDefinition& d) = 0;
	virtual void add(DefId defid, PCGroundRule* rule) = 0;
	virtual void add(GroundFixpDef*) = 0;
	virtual void add(Lit head, AggTsBody* body) = 0;
	virtual void add(Lit tseitin, CPTsBody* body) = 0;
	virtual void add(SetId setnr, DefId defnr, bool weighted) = 0;
	virtual void add(const Lit& head, TsType tstype, const litlist& clause, bool conj, DefId defnr) = 0;

	virtual void notifyUnknBound(const Lit& boundlit, const ElementTuple& args, std::vector<LazyUnknBoundGrounder*> grounders) = 0;
	virtual void notifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder) = 0;

	//NOTE: have to call these!
	//TODO check whether they are called correctly (currently in theorygrounder->run), but probably missing several usecases
	virtual void closeTheory() = 0;

	// Inspectors
	GroundTranslator* translator() const {
		return _translator;
	}
	GroundTermTranslator* termtranslator() const {
		return _termtranslator;
	}
	AbstractStructure* structure() const {
		return _structure;
	}
	AbstractGroundTheory* clone() const;
};

#endif /* ABSTRACTGROUNDTHEORY_HPP_ */
