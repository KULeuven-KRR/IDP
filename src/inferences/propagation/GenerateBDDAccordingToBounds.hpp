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
	std::shared_ptr<FOBDDManager> _manager;

	Bound _ctbounds, _cfbounds;
	std::map<PFSymbol*, std::vector<const FOBDDVariable*> > _vars;

	TruthType _type;
	const FOBDD* _result;
	const Structure* _structure; //Can be NULL. If different from NULL, structure can be used to improve BDDs, e.g.~not generate <ct> and <cf> symbols for known predicates

	//All symbols that were already propagated in earlier stages
	// And hence, should never be replaced by their BDD
	// If NULL, all symbols can be replaced!
	Vocabulary* _symbolsThatCannotBeReplacedByBDDs;


protected:
	void visit(const PredForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);
	void visit(const EquivForm*);

public:
	GenerateBDDAccordingToBounds(std::shared_ptr<FOBDDManager> m, const Bound& ctbounds, const Bound& cfbounds,
			const std::map<PFSymbol*, std::vector<const FOBDDVariable*> >& v, Vocabulary* symbolsThatCannotBeReplacedByBDDs)
			: _manager(m), _ctbounds(ctbounds), _cfbounds(cfbounds), _vars(v), _type(TruthType::CERTAIN_TRUE), _result(NULL), _structure(NULL), _symbolsThatCannotBeReplacedByBDDs(symbolsThatCannotBeReplacedByBDDs) {
		Assert(m!=NULL);
	}
	~GenerateBDDAccordingToBounds();
	// Transfers ownership!
	std::shared_ptr<FOBDDManager> obtainManager() const {
		return _manager;
	}

	/**
	 * Generate a bdd which contains exactly all instances for which the given formula has the requested truth type.
	 * @pre: GRAPHED functions and aggregates!
	 */
	const FOBDD* evaluate(Formula* formula, TruthType truthvalue, const Structure*);

	std::ostream& put(std::ostream&) const;
};

#endif	/* SYMBOLICSTRUCTURE_HPP_ */
