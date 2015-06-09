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

#include "common.hpp"
#include "structure/fwstructure.hpp"
#include <sstream>

typedef std::vector<Lit> GroundClause;
struct GroundEquivalence;

class PFSymbol;
class AbstractGroundTheory;

class GroundDefinition;
class GroundFixpDef;
class GroundSet;
class GroundAggregate;
class CPReification;
class GroundTranslator;
class AggTsBody;
class PCTsBody;
class TsSet;
class CPTsBody;
class PCGroundRule;
class AggGroundRule;
struct GroundTerm;
class LazyGroundingManager;

struct LazyInstantiation;
class DelayGrounder;

class GroundPolicy {
private:
	std::vector<GroundClause> _clauses;
	std::map<DefId, GroundDefinition*> _definitions;
	std::vector<GroundFixpDef*> _fixpdefs;
	std::vector<GroundSet*> _sets;
	std::vector<GroundAggregate*> _aggregates;
	std::vector<CPReification*> _cpreifications;

	GroundTranslator* _translator;
	GroundTranslator* polTranslator() const {
		return _translator;
	}

protected:
	void polAddLazyAddition(const litlist&, int){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polStartLazyFormula(LazyInstantiation*, TsType, bool){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polNotifyLazyResidual(LazyInstantiation*, TsType){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polAddLazyElement(Lit, PFSymbol*, const std::vector<GroundTerm>&, AbstractGroundTheory*, bool){
		throw notyetimplemented("Storing ground theories with lazy element constraints");
	}
	void polNotifyLazyWatch(Atom, TruthValue, LazyGroundingManager*){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polAdd(Lit, VarId){
		throw notyetimplemented("Storing denotation tseitins");
	}

public:
	const std::vector<GroundClause>& getClauses() const {
		return _clauses;
	}
	const std::vector<GroundSet*>& getSets() const {
		return _sets;
	}
	const std::vector<GroundAggregate*>& getAggregates() const {
		return _aggregates;
	}
	const std::vector<GroundFixpDef*>& getFixpDefinitions() const {
		return _fixpdefs;
	}
	const std::vector<CPReification*>& getCPReifications() const {
		return _cpreifications;
	}
	const std::map<DefId, GroundDefinition*>& getDefinitions() const {
		return _definitions;
	}

	void polStartTheory(GroundTranslator* translator);
	void polEndTheory();

	void polRecursiveDelete();

	void polAdd(const GroundClause& cl);
	void polAdd(const GroundEquivalence& geq);
	void polAdd(Lit tseitin, AggTsBody* body);
	void polAdd(Lit tseitin, CPTsBody* body);
	void polAdd(const TsSet& tsset, SetId setnr, bool);
	void polAdd(DefId defnr, const PCGroundRule& rule);
	void polAdd(DefId defnr, AggGroundRule* rule);

	void polAddOptimization(AggFunction, SetId);
	void polAddOptimization(VarId);

	std::ostream& polPut(std::ostream& s, GroundTranslator* translator) const;
};
