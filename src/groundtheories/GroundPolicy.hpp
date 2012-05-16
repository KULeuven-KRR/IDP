/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef GROUNDTHEORY_HPP_
#define GROUNDTHEORY_HPP_

#include "common.hpp"
#include "structure/fwstructure.hpp"
#include <sstream>

typedef std::vector<Lit> GroundClause;
typedef unsigned int VarId;

class GroundDefinition;
class GroundFixpDef;
class GroundSet;
class GroundAggregate;
class CPReification;
class GroundTranslator;
class GroundTermTranslator;
class AggTsBody;
class PCTsBody;
class TsSet;
class CPTsBody;
class PCGroundRule;
class AggGroundRule;

class LazyStoredInstantiation;
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
	void polNotifyUnknBound(Context, const Lit&, const ElementTuple&, std::vector<DelayGrounder*>){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polAddLazyAddition(const litlist&, int){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
	}
	void polNotifyLazyResidual(Lit, LazyStoredInstantiation*, TsType, bool){
		throw notyetimplemented("Storing ground theories with lazy ground elements");
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
	const std::map<int, GroundDefinition*>& getDefinitions() const {
		return _definitions;
	}

	void polStartTheory(GroundTranslator* translator);
	void polEndTheory();

	void polRecursiveDelete();

	void polAdd(const GroundClause& cl);
	void polAdd(Lit tseitin, AggTsBody* body);
	void polAdd(Lit tseitin, CPTsBody* body);
	void polAdd(const TsSet& tsset, SetId setnr, bool);
	void polAdd(DefId defnr, PCGroundRule* rule);
	void polAdd(DefId defnr, AggGroundRule* rule);

	void polAddOptimization(AggFunction, SetId);
	void polAddOptimization(VarId);
	void polAdd(const std::vector<std::map<Lit, Lit> >& symmetry);

	std::ostream& polPut(std::ostream& s, GroundTranslator* translator, GroundTermTranslator* termtranslator) const;

	std::string polToString(GroundTranslator* translator, GroundTermTranslator* termtranslator) const;
};

#endif /* GROUNDTHEORY_HPP_ */
