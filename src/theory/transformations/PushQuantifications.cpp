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

#include "theory/TheoryUtils.hpp"

using namespace std;

template<class VarList>
bool sharesVars(const VarList& listone, const VarList& listtwo) {
	for (auto i = listone.cbegin(); i != listone.cend(); ++i) {
		if (listtwo.find(*i) != listtwo.cend()) {
			return true;
		}
	}
	return false;
}

template<typename FormulaList, typename VarList, typename VarSet>
void splitOverSameQuant(const VarSet& quantset, const FormulaList& formulasToSplit, vector<FormulaList>& splitformulas, VarList& splitvariables) {
	// TODO analyse van de beste volgorde => ordenen volgens stijgend aantal formules dat die variabele bevat.
	for (auto i = quantset.cbegin(); i != quantset.cend(); ++i) {
		splitvariables.push_back(*i);
	}
	splitformulas.resize(splitvariables.size());
	for (auto i = formulasToSplit.cbegin(); i < formulasToSplit.cend(); ++i) {
		for (size_t j = splitvariables.size() - 1; j > -1; --j) {
			if ((*i)->freeVars().find(splitvariables[j]) != (*i)->freeVars().cend()) {
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
	auto currentquant = qf;

	while (true) {
		auto qsubf = dynamic_cast<QuantForm*>(currentquant->subformula());
		if (qsubf != NULL) {
			quantforms.push_back(qsubf);
			currentquant = qsubf;
		} else {
			auto bsubf = dynamic_cast<BoolForm*>(currentquant->subformula());
			if (bsubf == NULL) {
				return qf;
			}
			vector<vector<Formula*> > formulalevels(quantforms.size() + 1); // Level at which they should be added (index 0 is before the first quantifier, ...)
			conj = bsubf->conj();
			Assert(bsubf->sign()==SIGN::POS);
			for (auto i = bsubf->subformulas().cbegin(); i < bsubf->subformulas().cend(); ++i) {
				auto subvars = (*i)->freeVars();
				int index = quantforms.size();
				while (index > 0 && not sharesVars(subvars, quantforms[index - 1]->quantVars())) {
					index--;
				}
				formulalevels[index].push_back(*i);
			}
			// Construct new formula:
			Formula* prevform = NULL;
			for (size_t i = quantforms.size() - 1; i > -1; --i) {
				vector<vector<Formula*> > splitformulalevels;
				vector<Variable*> splitvariables;
				splitOverSameQuant(quantforms[i]->quantVars(), formulalevels[i + 1], splitformulalevels, splitvariables);
				Assert(splitvariables.size()>0);
				for (size_t j = splitvariables.size() - 1; j > -1; --j) {
					auto subformulas = splitformulalevels[j];
					if (prevform != NULL) {
						subformulas.push_back(prevform);
					} else {
						Assert(subformulas.size()>0);
					}
					if (subformulas.size() == 1) {
						prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), { splitvariables[j] }, subformulas.back(), quantforms[i]->pi());
					} else {
						prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), { splitvariables[j] },
								new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi()), quantforms[i]->pi());
					}
				}
			}
			auto subformulas = formulalevels[0];
			Assert(prevform!=NULL);
			subformulas.push_back(prevform);

			Formula* result = NULL;
			if (subformulas.size() == 1) {
				result = subformulas.back();
			} else {
				result = new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi());
			}

			return FormulaUtils::flatten(result);
		}
	}
}
