#ifndef SYMBOLICSTRUCTURE_HPP_
#define SYMBOLICSTRUCTURE_HPP_

#include <map>
#include <vector>

#include "visitors/TheoryVisitor.hpp"

class FOBDDManager;
class FOBDD;
class FOBDDVariable;

typedef std::map<PFSymbol*, const FOBDD*> Bound;

class GenerateBDDAccordingToBounds: public TheoryVisitor {
	VISITORFRIENDS()
private:
	FOBDDManager* _manager;
	Bound _ctbounds, _cfbounds;
	std::map<PFSymbol*, std::vector<const FOBDDVariable*> > _vars;

	TruthType _type;
	const FOBDD* _result;

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
			const std::map<PFSymbol*, std::vector<const FOBDDVariable*> >& v) :
			_manager(m), _ctbounds(ctbounds), _cfbounds(cfbounds), _vars(v) {
	}
	FOBDDManager* manager() const {
		return _manager;
	}

	/**
	 * Generate a bdd which contains exactly all instances for which the given formula has the requested truth type.
	 */
	const FOBDD* evaluate(Formula* formula, TruthType truthvalue);

	std::ostream& put(std::ostream&) const;
};

#endif	/* SYMBOLICSTRUCTURE_HPP_ */
