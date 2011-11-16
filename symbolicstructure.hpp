/************************************
  	symbolicstructure.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SYMBOLICSTRUCTURE_HPP_
#define SYMBOLICSTRUCTURE_HPP_

#include <map>
#include <vector>

#include "visitors/TheoryVisitor.hpp"

class FOBDDManager;
class FOBDD;
class FOBDDVariable;

class SymbolicStructure : public TheoryVisitor {
	private:
		FOBDDManager*											_manager;
		std::map<PFSymbol*,const FOBDD*>						_ctbounds;
		std::map<PFSymbol*,const FOBDD*>						_cfbounds;
		std::map<PFSymbol*,std::vector<const FOBDDVariable*> >	_vars;

		TruthType		_type;
		const FOBDD*	_result;
		void visit(const PredForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const EqChainForm*);
		void visit(const AggForm*);
		void visit(const EquivForm*);
		const FOBDD* prunebdd(const FOBDD*, const std::vector<const FOBDDVariable*>&,AbstractStructure*,double);

	public:
		SymbolicStructure(FOBDDManager* m,const std::map<PFSymbol*,const FOBDD*>& ct, const std::map<PFSymbol*,const FOBDD*>& cf, const std::map<PFSymbol*,std::vector<const FOBDDVariable*> >& v) : 
			_manager(m), _ctbounds(ct), _cfbounds(cf), _vars(v) { }
		FOBDDManager*	manager()						const { return _manager;	}
		const FOBDD*	evaluate(Formula*,TruthType);

		/** Make the symbolic structure less precise, based on the given structure **/
		void filter(AbstractStructure* structure, double max_cost_per_answer);

		std::ostream& put(std::ostream&) const;
};

#endif	/* SYMBOLICSTRUCTURE_HPP_ */
