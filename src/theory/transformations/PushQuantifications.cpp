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
void splitOverSameQuant(const VarSet& quantset, const FormulaList& formulasToSplit, FormulaList& notquantformulas, vector<FormulaList>& splitformulas, VarList& splitvariables) {
	// TODO analyse van de beste volgorde => ordenen volgens stijgend aantal formules dat die variabele bevat.
	for (auto i = quantset.cbegin(); i != quantset.cend(); ++i) {
		splitvariables.push_back(*i);
	}
	splitformulas.resize(splitvariables.size());
	for (auto i = formulasToSplit.cbegin(); i < formulasToSplit.cend(); ++i) {
		bool found = false;
		for (long j = (long)splitvariables.size() - 1; j > -1; --j) {
			if ((*i)->freeVars().find(splitvariables[j]) != (*i)->freeVars().cend()) {
				splitformulas[j].push_back(*i);
				found = true;
				break;
			}
		}
		if(not found){
			notquantformulas.push_back(*i);
		}
	}
}

Formula* PushQuantifications::visit(QuantForm* qf) {
	if(qf->quantVars().size()==0){
		return qf->subformula()->accept(this);
	}

	bool conj = true;
	vector<QuantForm*> quantforms;
	quantforms.push_back(qf);
	auto currentquant = qf;

	while (true) {
		auto qsubf = dynamic_cast<QuantForm*>(currentquant->subformula());
		if (qsubf != NULL) {
			quantforms.push_back(qsubf);
			currentquant = qsubf;
			continue;
		}

		auto bsubf = dynamic_cast<BoolForm*>(currentquant->subformula());
		if (bsubf == NULL || (bsubf->isConjWithSign() != qf->isUniv())) {
			//If the subformula is not a boolform, we cannot push the quantifications
			//If we have a situation !x y: P(x) | Q(x,y), we don't want to push the y down either as this introduces extra Tseitins
			return qf;
		}

		// For each of the subformulas of the boolform, find out on which 'level' of quantification they belong.
		// Level 0: before all quantified variables. This happens only if the subformula does not contain any
		//          quantified variable, from any of the level
		// Level X: This is the deepest level at which a a quantified variable is present in the subformula.
		//          E.g. the formula
		//				! x : ! y : ! z : !a : P(x) | Q(x,y) | R(x,y,z) | false.
		//
		//			should become:
		//				false | ! x : (P(x) | ! y (Q(x,y) | ! z : (R(x,y,z) | ! a : false))).
		//				      ^1            ^1            ^1                ^1        ^2
		//          P(x) occurs at level 1 (after the ! x)
		//          Q(x,y) occurs at level 2 (after the ! y)
		//          R(x,y,z) occurs at level 3 (after the ! z)
		//			'false' occurs at level 0 (before all quantifications)
		//			=>  These 'levels' have to be joined in a boolform with connectors of the same sign as
		//				the original bsubf. In the above example, these connectors are marked with "^1"
		//
		//          Note how no element occurs at level 4 (after the ! a), even though this level exists.
		//          This means this 'level' should contain 'false' if bsubf is a disjunction, and 'true' if
		//			bsubf is a conjunction. (This means it has to be an empty boolform of the same type).
		//          The contents of this last level is marked with "^2.


		// This vector stores, given an index, which formulas belong on this level.
		vector<vector<Formula*> > formulalevels(quantforms.size() + 1);
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
		for (long i = (long)quantforms.size() - 1; i > -1; --i) {
			vector<Formula*> nonquantformulas;
			vector<vector<Formula*> > splitformulalevels;
			vector<Variable*> splitvariables;
			splitOverSameQuant(quantforms[i]->quantVars(), formulalevels[i + 1], nonquantformulas, splitformulalevels, splitvariables);
			Assert(splitvariables.size()>0);
			for (long j = (long)splitvariables.size() - 1; j > -1; --j) {
				auto subformulas = splitformulalevels[j];
				if (prevform != NULL) {
					subformulas.push_back(prevform);
				} else {
					// The previous formula was NULL, so this is the very first element we're constructing of our new formula.
					if(subformulas.empty()){
					    // If no formulas are put into this level, create an empty boolform of the
						// same type (this is the construction of ^2 as seen above)
						subformulas.push_back(new BoolForm(SIGN::POS, conj, { }, FormulaParseInfo()));
					}
				}
				if (subformulas.size() == 1) {
					// If this "level" only contains a single subformula, put this subformula directly beneath the quantform
					prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), { splitvariables[j] }, subformulas.back(), quantforms[i]->pi());
				} else {
					// If this "level" only contains multiple subformulas, join them in a boolform using the same
					// connector as the original bsubf (this is the construction of ^1 connectors in the above example)
					prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), { splitvariables[j] },
							new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi()), quantforms[i]->pi());
				}
			}
			if(nonquantformulas.size()>0){
				nonquantformulas.push_back(prevform);
				prevform = new BoolForm(SIGN::POS, conj, nonquantformulas, prevform->pi());
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
