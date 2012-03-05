/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"
#include "PushQuantifications.hpp"

using namespace std;

/*std::set<Variable*> clone(const std::set<Variable*>& origvars) {
	std::set<Variable*> newvars;
	for (auto it = origvars.cbegin(); it != origvars.cend(); ++it) {
		auto v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newvars.insert(v);
	}
	return newvars;
}*/

// old code, absolutely not useful
/*Formula* PushQuantifications::visit(QuantForm* qf) {
	if (not sametypeid<BoolForm>(*(qf->subformula()))) {
		return TheoryMutatingVisitor::visit(qf);
	}

	auto bf = dynamic_cast<BoolForm*>(qf->subformula());
	auto u = qf->isUnivWithSign() ? QUANT::UNIV : QUANT::EXIST;
	auto c = bf->isConjWithSign();

	if ((u == QUANT::UNIV && not bf->isConjWithSign()) || (u == QUANT::EXIST && bf->isConjWithSign())) {
		return TheoryMutatingVisitor::visit(qf);
	}

	auto s = (qf->sign() == bf->sign()) ? SIGN::POS : SIGN::NEG;
	vector<Formula*> vf;
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		vf.push_back(new QuantForm(s, u, clone(qf->quantVars()), (*it)->clone(), FormulaParseInfo()));
	}
	auto nbf = new BoolForm(SIGN::POS, c, vf, (qf->pi()));
	qf->subformula()->recursiveDelete();
	delete (qf);
	return nbf->accept(this);
}*/

template<class VarList>
bool sharesVars(const VarList& listone, const VarList& listtwo){
	for(auto i=listone.cbegin(); i!=listone.cend(); ++i) {
		if(listtwo.find(*i)!=listtwo.cend()){
			return true;
		}
	}
	return false;
}

template<typename FormulaList, typename VarList, typename VarSet>
void splitOverSameQuant(const VarSet& quantset, const FormulaList& formulasToSplit, vector<FormulaList>& splitformulas, VarList& splitvariables){
	// TODO analyse van de beste volgorde => ordenen volgens stijgend aantal formules dat die variabele bevat.
	for(auto i=quantset.cbegin(); i!=quantset.cend(); ++i) {
		splitvariables.push_back(*i);
	}
	splitformulas.resize(splitvariables.size());
	for(auto i=formulasToSplit.cbegin(); i<formulasToSplit.cend(); ++i) {
		for(int j=splitvariables.size()-1; j>-1; --j){
			if((*i)->freeVars().find(splitvariables[j])!=(*i)->freeVars().cend()){
				splitformulas[j].push_back(*i);
				break;
			}
		}
	}
}

Formula* PushQuantifications::visit(QuantForm* qf) {
	// TODO requires PUSHING negations AND Flattening!
	bool conj = true;
	vector<QuantForm*> quantforms;
	quantforms.push_back(qf);

	while(true){
		auto qsubf = dynamic_cast<QuantForm*>(qf->subformula());
		if(qsubf!=NULL){
			quantforms.push_back(qsubf);
			qf = qsubf;
		}else{
			auto bsubf = dynamic_cast<BoolForm*>(qf->subformula());
			if(bsubf!=NULL){
				vector<vector<Formula*> > formulalevels(quantforms.size()+1); // Level at which they should be added (index 0 is before the first quantifier, ...)
				conj = bsubf->conj();
				Assert(bsubf->sign()==SIGN::POS);
				for(auto i=bsubf->subformulas().cbegin(); i<bsubf->subformulas().cend(); ++i) {
					auto subvars = (*i)->freeVars();
					int index = quantforms.size();
					while(index>0 && not sharesVars(subvars, quantforms[index-1]->quantVars())){
						index--;
					}
					formulalevels[index].push_back(*i);
				}
				// Construct new formula:
				Formula* prevform = NULL;
				for(int i=quantforms.size()-1; i>-1; --i){
					vector<vector<Formula*> > splitformulalevels;
					vector<Variable*> splitvariables;
					splitOverSameQuant(quantforms[i]->quantVars(), formulalevels[i+1], splitformulalevels, splitvariables);
					for(int j=splitvariables.size()-1; j>-1; --j){
						auto subformulas = splitformulalevels[j];
						if(prevform!=NULL){
							subformulas.push_back(prevform);
						}
						prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), {splitvariables[j]}, new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi()), quantforms[i]->pi());
					}
				}
				auto subformulas = formulalevels[0];
				Assert(prevform!=NULL);
				subformulas.push_back(prevform);
				return new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi());
			}else{
				return qf;
			}
		}
	}
}
