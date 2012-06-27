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

class GroundTranslator;

//FIXME definition numbers are passed directly to the solver. In future, solver input change might render this invalid

// Enumeration used in GroundTheory::transformForAdd
enum VIType {
	VIT_DISJ, VIT_CONJ, VIT_SET
};

class LazyStoredInstantiation;
class DelayGrounder;

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory: public AbstractTheory {
	VISITORS()
private:
	AbstractStructure* _structure; // OWNER! The ground theory might be partially reduced with respect to this structure.

	GroundTranslator* _translator; //!< Link between ground atoms and SAT-solver literals.

public:
	// Non-owning structure pointer, can be NULL!
	AbstractGroundTheory(AbstractStructure const * const str);
	// Non-owning structure pointer, can be NULL!
	AbstractGroundTheory(Vocabulary* voc, AbstractStructure const * const str);

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
	virtual void addSymmetries(const std::vector<std::map<Lit, Lit> >& symmetry) = 0;

	virtual void addOptimization(AggFunction, SetId) = 0;
	virtual void addOptimization(VarId) = 0;

	virtual void notifyNeedFalseDefineds(PFSymbol* pfs) = 0;

	virtual void notifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders) = 0;
	virtual void notifyLazyAddition(const litlist& glist, int ID) = 0;
	virtual void notifyLazyResidual(Lit tseitin, LazyStoredInstantiation* inst, TsType type, bool conjunction) = 0;

	//NOTE: have to call these!
	//TODO check whether they are called correctly (currently in theorygrounder->run), but probably missing several usecases
	virtual void closeTheory() = 0;

	GroundTranslator* translator() const {
		Assert(_translator!=NULL);
		return _translator;
	}
	AbstractStructure* structure() const {
		return _structure;
	}
	AbstractGroundTheory* clone() const;
};

#endif /* ABSTRACTGROUNDTHEORY_HPP_ */
