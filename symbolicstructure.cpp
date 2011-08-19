/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <iostream>
#include "vocabulary.hpp"
#include "theory.hpp"
#include "fobdd.hpp"
#include "symbolicstructure.hpp"
using namespace std;

QueryType swapTF(QueryType type) {
	switch(type) {
		case QT_PF: return QT_PT;
		case QT_PT: return QT_PF;
		case QT_CF: return QT_CT;
		case QT_CT: return QT_CF;
		default: assert(false);
	}
}


const FOBDD* SymbolicStructure::evaluate(Formula* f, QueryType type) {
	_type = type;
	_result = 0;
	f->accept(this);
	return _result;
}

void SymbolicStructure::visit(const PredForm* atom) {
	if(_ctbounds.find(atom->symbol()) == _ctbounds.end()) {
		FOBDDFactory factory(_manager);
		const FOBDD* bdd = factory.run(atom);
		if(_type == QT_CF || _type == QT_PF) bdd = _manager->negation(bdd);
		_result = bdd;
	}
	else {
		bool getct = (_type == QT_CT || _type == QT_PF);
		if(!atom->sign()) getct = !getct;
		const FOBDD* bdd = getct ? _ctbounds[atom->symbol()] : _cfbounds[atom->symbol()];
		map<const FOBDDVariable*, const FOBDDArgument*> mva;
		const vector<const FOBDDVariable*>& vars = _vars[atom->symbol()];
		FOBDDFactory factory(_manager);
		for(unsigned int n = 0; n < vars.size(); ++n) {
			mva[vars[n]] = factory.run(atom->subterms()[n]);
		}
		bdd = _manager->substitute(bdd,mva);
		if(_type == QT_PT || _type == QT_PF) bdd = _manager->negation(bdd);
		_result = bdd;
	}
}

void SymbolicStructure::visit(const BoolForm* boolform) {
	bool conjunction = boolform->sign() == boolform->conj();
	const FOBDD* currbdd;
	if(conjunction) currbdd = _manager->truebdd();
	else currbdd = _manager->falsebdd();
	QueryType rectype = boolform->sign() ? _type : swapTF(_type);
	
	for(auto it = boolform->subformulas().begin(); it != boolform->subformulas().end(); ++it) {
		const FOBDD* newbdd = evaluate(*it,rectype);
		currbdd = conjunction ? _manager->conjunction(currbdd,newbdd) : _manager->disjunction(currbdd,newbdd);
	}
	_result = currbdd;
}

void SymbolicStructure::visit(const QuantForm* quantform) {
	bool universal = quantform->sign() == quantform->univ();
	QueryType rectype = quantform->sign() ? _type : swapTF(_type);
	const FOBDD* subbdd = evaluate(quantform->subf(),rectype);
	set<const FOBDDVariable*> vars = _manager->getVariables(quantform->quantvars());
	_result = universal ? _manager->univquantify(vars,subbdd) : _manager->existsquantify(vars,subbdd);
}

void SymbolicStructure::visit(const EqChainForm* eqchainform) {
	Formula* cloned = eqchainform->clone();
	cloned = FormulaUtils::remove_eqchains(cloned);
	_result = evaluate(cloned,_type);
	cloned->recursiveDelete();
}

void SymbolicStructure::visit(const EquivForm* equivform) {
	Formula* cloned = equivform->clone();
	cloned = FormulaUtils::remove_equiv(cloned);
	_result = evaluate(cloned,_type);
	cloned->recursiveDelete();
}

void SymbolicStructure::visit(const AggForm*) {
	// TODO: better evaluation function?
	if(_type == QT_PT || _type == QT_PF) _result = _manager->truebdd();
	else _result =  _manager->falsebdd();
}

ostream& SymbolicStructure::put(ostream& output) const {
	for(map<PFSymbol*,vector<const FOBDDVariable*> >::const_iterator it = _vars.begin(); it != _vars.end(); ++it) {
		output << "   "; (it->first)->put(output); output << endl;
		output << "      vars:";
		for(auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
			output << ' ';
			_manager->put(output,*jt);
		}
		output << '\n';
		output << "      ct:" << endl;
		_manager->put(output,_ctbounds.find(it->first)->second,10);
		output << "      cf:" << endl;
		_manager->put(output,_cfbounds.find(it->first)->second,10);
		output << "\n";
	}
	return output;
}

const FOBDD* SymbolicStructure::prunebdd(const FOBDD* bdd, const vector<const FOBDDVariable*>& bddvars,AbstractStructure* structure, double mcpa) {
		// 1. Optimize the query
		FOBDDManager optimizemanager;
		const FOBDD* copybdd = optimizemanager.getBDD(bdd,_manager);
		set<const FOBDDVariable*> copyvars;
		set<const FOBDDDeBruijnIndex*> indices;
		for(auto it = bddvars.begin(); it != bddvars.end(); ++it) 
			copyvars.insert(optimizemanager.getVariable((*it)->variable()));
		optimizemanager.optimizequery(copybdd,copyvars,indices,structure);

		// 2. Remove certain leaves
		const FOBDD* pruned = optimizemanager.make_more_false(copybdd,copyvars,indices,structure,mcpa);

		// 3. Replace result
		return _manager->getBDD(pruned,&optimizemanager);
}

void SymbolicStructure::filter(AbstractStructure* structure, double max_cost_per_answer) {
	for(auto it = _ctbounds.begin(); it != _ctbounds.end(); ++it) {
cerr << "Filtering\n"; _manager->put(cerr,it->second);
		it->second = prunebdd(it->second,_vars[it->first],structure,max_cost_per_answer);
cerr << "Result of filtering\n"; _manager->put(cerr,it->second);
	}
	for(auto it = _cfbounds.begin(); it != _cfbounds.end(); ++it) {
cerr << "Filtering\n"; _manager->put(cerr,it->second);
		it->second = prunebdd(it->second,_vars[it->first],structure,max_cost_per_answer);
cerr << "Result of filtering\n"; _manager->put(cerr,it->second);
	}
}


















