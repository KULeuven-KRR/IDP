#ifndef GROUNDTHEORY_HPP_
#define GROUNDTHEORY_HPP_

#include "common.hpp"
#include <sstream>

typedef std::vector<Lit> GroundClause;
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

class GroundPolicy {
private:
	std::vector<GroundClause> _clauses;
	std::map<int, GroundDefinition*> _definitions;
	std::vector<GroundFixpDef*> _fixpdefs;
	std::vector<GroundSet*> _sets;
	std::vector<GroundAggregate*> _aggregates;
	std::vector<CPReification*> _cpreifications;

	GroundTranslator* _translator;
	GroundTranslator* polTranslator() const {
		return _translator;
	}

public:
	const std::vector<GroundClause>& getClauses() const { return _clauses; }
	const std::vector<GroundSet*>& getSets() const { return _sets; }
	const std::vector<GroundAggregate*>& getAggregates() const { return _aggregates; }
	const std::vector<GroundFixpDef*>& getFixpDefinitions() const { return _fixpdefs; }
	const std::vector<CPReification*>& getCPReifications() const { return _cpreifications; }
	const std::map<int, GroundDefinition*>& getDefinitions() const { return _definitions; }

	void polStartTheory(GroundTranslator* translator);
	void polEndTheory();

	void polRecursiveDelete();

	void polAdd(const GroundClause& cl);
	void polAdd(int head, AggTsBody* body);
	void polAdd(int tseitin, CPTsBody* body);
	void polAdd(const TsSet& tsset, int setnr, bool);
	void polAdd(GroundDefinition* d);
	void polAdd(int defnr, PCGroundRule* rule);
	void polAdd(int defnr, AggGroundRule* rule);

	std::ostream& polPut(std::ostream& s, GroundTranslator* translator, GroundTermTranslator* termtranslator, bool longnames) const;

	std::string polToString(GroundTranslator* translator, GroundTermTranslator* termtranslator, bool longnames) const;
};

#endif /* GROUNDTHEORY_HPP_ */
