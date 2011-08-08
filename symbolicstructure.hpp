/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SYMBOLICSTRUCTURE_HPP_
#define SYMBOLICSTRUCTURE_HPP_

#include <map>
#include <vector>

enum QueryType { QT_CT, QT_CF, QT_PT, QT_PF };

class SymbolicStructure : public TheoryVisitor {
	private:
		FOBDDManager*											_manager;
		std::map<PFSymbol*,const FOBDD*>						_ctbounds;
		std::map<PFSymbol*,const FOBDD*>						_cfbounds;
		std::map<PFSymbol*,std::vector<const FOBDDVariable*> >	_vars;

		QueryType		_type;
		const FOBDD*	_result;
		void visit(const PredForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const EqChainForm*);
		void visit(const AggForm*);
		void visit(const EquivForm*);

	public:
		SymbolicStructure(FOBDDManager* m,const std::map<PFSymbol*,const FOBDD*>& ct, const std::map<PFSymbol*,const FOBDD*>& cf, const std::map<PFSymbol*,std::vector<const FOBDDVariable*> >& v) : 
			_manager(m), _ctbounds(ct), _cfbounds(cf), _vars(v) { }
		FOBDDManager*	manager()						const { return _manager;	}
		const FOBDD*	evaluate(Formula*,QueryType);
		std::ostream& put(std::ostream&) const;
};

#endif	/* SYMBOLICSTRUCTURE_HPP_ */
