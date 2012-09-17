/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SYMBOLICSTRUCTURE_HPP_
#define SYMBOLICSTRUCTURE_HPP_

#include <map>
#include <vector>

#include "visitors/TheoryVisitor.hpp"

class FOBDDManager;
class FOBDD;
class FOBDDVariable;

typedef std::map<PFSymbol*, const FOBDD*> Bound;

class GenerateBDDAccordingToBounds: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	mutable bool _ownsmanager;
	FOBDDManager* _manager;

	Bound _ctbounds, _cfbounds;
	std::map<PFSymbol*, std::vector<const FOBDDVariable*> > _vars;

	TruthType _type;
	const FOBDD* _result;

	//All symbols that were already propagated in earlier stages
	// And hence, should never be replaced by their BDD
	Vocabulary* _symbolsThatCannotBeReplacedByBDDs;

	const FOBDD* prunebdd(const FOBDD*, const std::vector<const FOBDDVariable*>&, AbstractStructure*, double);

	/** Make the symbolic structure less precise, based on the given structure **/
	void filter(AbstractStructure* structure, double max_cost_per_answer);

protected:
	void visit(const PredForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);
	void visit(const EquivForm*);

public:
	GenerateBDDAccordingToBounds(FOBDDManager* m, const Bound& ctbounds, const Bound& cfbounds,
			const std::map<PFSymbol*, std::vector<const FOBDDVariable*> >& v, Vocabulary* symbolsThatCannotBeReplacedByBDDs)
			: _ownsmanager(true), _manager(m), _ctbounds(ctbounds), _cfbounds(cfbounds), _vars(v), _type(TruthType::CERTAIN_TRUE), _result(NULL), _symbolsThatCannotBeReplacedByBDDs(symbolsThatCannotBeReplacedByBDDs) {
		Assert(m!=NULL);
		Assert(symbolsThatCannotBeReplacedByBDDs != NULL);
	}
	~GenerateBDDAccordingToBounds();
	// Transfers ownership!
	FOBDDManager* obtainManager() const {
		_ownsmanager = false;
		return _manager;
	}

	/**
	 * Generate a bdd which contains exactly all instances for which the given formula has the requested truth type.
	 * @pre: GRAPHED functions and aggregates!
	 */
	const FOBDD* evaluate(Formula* formula, TruthType truthvalue);

	std::ostream& put(std::ostream&) const;
};

#endif	/* SYMBOLICSTRUCTURE_HPP_ */
