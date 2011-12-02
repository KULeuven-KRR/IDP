#ifndef ABSTRACTGROUNDTHEORY_HPP_
#define ABSTRACTGROUNDTHEORY_HPP_

#include "commontypes.hpp"

#include "theory.hpp"
#include "ecnf.hpp"

class GroundTermTranslator;
class GroundTranslator;

//FIXME definition numbers are passed directly to the solver. In future, solver input change might render this invalid

// Enumeration used in GroundTheory::transformForAdd
enum VIType {
	VIT_DISJ, VIT_CONJ, VIT_SET
};

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory: public AbstractTheory {
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
	virtual void add(GroundDefinition* d) = 0;
	virtual void add(GroundFixpDef*) = 0;
	virtual void add(int head, AggTsBody* body) = 0;
	virtual void add(int tseitin, CPTsBody* body) = 0;
	virtual void add(int setnr, unsigned int defnr, bool weighted) = 0;

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
