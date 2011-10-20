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
	}
}


const FOBDD* SymbolicStructure::evaluate(Formula* f, QueryType type) {
	_type = type;
	_result = 0;
	f->accept(this);
	return _result;
}

void SymbolicStructure::visit(const PredForm* atom) {
	if(_ctbounds.find(atom->symbol()) == _ctbounds.cend()) {
		FOBDDFactory factory(_manager);
		const FOBDD* bdd = factory.run(atom);
		if(_type == QT_CF || _type == QT_PF) bdd = _manager->negation(bdd);
		_result = bdd;
	}
	else {
		bool getct = (_type == QT_CT || _type == QT_PF);
		if(isNeg(atom->sign())){
			getct = !getct;
		}
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
	bool conjunction = boolform->isConjWithSign();
	conjunction = (_type == QT_PF || _type == QT_CF) ? !conjunction : conjunction;
	QueryType rectype = boolform->sign()==SIGN::POS ? _type : swapTF(_type);

	const FOBDD* currbdd;
	if(conjunction) currbdd = _manager->truebdd();
	else currbdd = _manager->falsebdd();
	
	for(auto it = boolform->subformulas().cbegin(); it != boolform->subformulas().cend(); ++it) {
		const FOBDD* newbdd = evaluate(*it,rectype);
		currbdd = conjunction ? _manager->conjunction(currbdd,newbdd) : _manager->disjunction(currbdd,newbdd);
	}
	_result = currbdd;
}

void SymbolicStructure::visit(const QuantForm* quantform) {
	bool universal = quantform->isUnivWithSign();
	universal = (_type == QT_PF || _type == QT_CF) ? !universal : universal;
	QueryType rectype = quantform->sign()==SIGN::POS ? _type : swapTF(_type);
	const FOBDD* subbdd = evaluate(quantform->subformula(),rectype);
	set<const FOBDDVariable*> vars = _manager->getVariables(quantform->quantVars());
	_result = universal ? _manager->univquantify(vars,subbdd) : _manager->existsquantify(vars,subbdd);
}

void SymbolicStructure::visit(const EqChainForm* eqchainform) {
	Formula* cloned = eqchainform->clone();
	cloned = FormulaUtils::removeEqChains(cloned);
	_result = evaluate(cloned,_type);
	cloned->recursiveDelete();
}

void SymbolicStructure::visit(const EquivForm* equivform) {
	Formula* cloned = equivform->clone();
	cloned = FormulaUtils::removeEquiv(cloned);
	_result = evaluate(cloned,_type);
	cloned->recursiveDelete();
}

void SymbolicStructure::visit(const AggForm*) {
	// TODO: better evaluation function?
	if(_type == QT_PT || _type == QT_PF) _result = _manager->truebdd();
	else _result =  _manager->falsebdd();
}

ostream& SymbolicStructure::put(ostream& output) const {
	for(auto it = _vars.cbegin(); it != _vars.cend(); ++it) {
		output << "   "; (it->first)->put(output); output << endl;
		output << "      vars:";
		for(auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
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

cerr << "filtering the bdd\n";
_manager->put(cerr,bdd);
cerr << "input variables are";
for(auto it = bddvars.cbegin(); it != bddvars.cend(); ++it) cerr << ' ' << *((*it)->variable());
cerr << endl;

		// 1. Optimize the query
		FOBDDManager optimizemanager;
		const FOBDD* copybdd = optimizemanager.getBDD(bdd,_manager);
		set<const FOBDDVariable*> copyvars;
		set<const FOBDDDeBruijnIndex*> indices;
		for(auto it = bddvars.cbegin(); it != bddvars.cend(); ++it) 
			copyvars.insert(optimizemanager.getVariable((*it)->variable()));
		optimizemanager.optimizequery(copybdd,copyvars,indices,structure);

cerr << "optimized version:\n";
optimizemanager.put(cerr,copybdd);
cerr << "estimated nr. of answers: " << optimizemanager.estimatedNrAnswers(copybdd,copyvars,indices,structure) << endl;
cerr << "estimated cost: " << optimizemanager.estimatedCostAll(copybdd,copyvars,indices,structure) << endl;

		// 2. Remove certain leaves
		const FOBDD* pruned = optimizemanager.make_more_false(copybdd,copyvars,indices,structure,mcpa);

cerr << "pruned version:\n";
optimizemanager.put(cerr,pruned);

		// 3. Replace result
		return _manager->getBDD(pruned,&optimizemanager);
}

void SymbolicStructure::filter(AbstractStructure* structure, double max_cost_per_answer) {
	for(auto it = _ctbounds.begin(); it != _ctbounds.end(); ++it) {
//cerr << "Filtering\n"; _manager->put(cerr,it->second);
		it->second = prunebdd(it->second,_vars[it->first],structure,max_cost_per_answer);
//cerr << "Result of filtering\n"; _manager->put(cerr,it->second);
	}
	for(auto it = _cfbounds.begin(); it != _cfbounds.end(); ++it) {
//cerr << "Filtering\n"; _manager->put(cerr,it->second);
		it->second = prunebdd(it->second,_vars[it->first],structure,max_cost_per_answer);
//cerr << "Result of filtering\n"; _manager->put(cerr,it->second);
	}
}


















