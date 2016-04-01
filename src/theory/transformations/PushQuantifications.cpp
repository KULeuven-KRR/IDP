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
		for (long j = (long) splitvariables.size() - 1; j > -1; --j) {
			if ((*i)->freeVars().find(splitvariables[j]) != (*i)->freeVars().cend()) {
				splitformulas[j].push_back(*i);
				found = true;
				break;
			}
		}
		if (not found) {
			notquantformulas.push_back(*i);
		}
	}
}

Rule* PushQuantifications::visit(Rule* rule) {
    // push universal quantifiers with only variables in the body
	varset bodyonlyvars, rem;
	for (auto var : rule->quantVars()) {
		if (not contains(rule->head()->freeVars(), var)) {
            if(contains(rule->body()->freeVars(), var)){
                bodyonlyvars.insert(var);
            }// else variable also not occurs in the body -> ignore it
		} else {
			rem.insert(var);
		}
	}
	rule->setQuantVars(rem);
	if (not bodyonlyvars.empty()) {
		rule->body(new QuantForm(SIGN::POS, QUANT::EXIST, bodyonlyvars, rule->body(), FormulaParseInfo()));
	}
    
    // visit body
	rule->body(rule->body()->accept(this));
    
	return rule;
}

Formula* PushQuantifications::visit(QuantForm* qf) {
	if (qf->quantVars().size() == 0) { // if formula does not contain quantified vars, strip all quantifiers.
		return qf->subformula()->accept(this);
	}

	bool conj = true;
	vector<QuantForm*> quantforms;
	quantforms.push_back(qf);
	auto currentquant = qf;

	while (true) {
		auto qsubf = dynamic_cast<QuantForm*> (currentquant->subformula());
		if (qsubf != NULL) {
			quantforms.push_back(qsubf); // build vector of quantified subformula's
			currentquant = qsubf;
			continue;
		}

		auto bsubf = dynamic_cast<BoolForm*> (currentquant->subformula());
		if (bsubf == NULL || bsubf->isConjWithSign() != qf->isUniv()) {
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
		Assert(bsubf->sign() == SIGN::POS);
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
		for (long i = (long) quantforms.size() - 1; i > -1; --i) {
			vector<Formula*> nonquantformulas;
			vector<vector<Formula*> > splitformulalevels;
			vector<Variable*> splitvariables;
			splitOverSameQuant(quantforms[i]->quantVars(), formulalevels[i + 1], nonquantformulas, splitformulalevels, splitvariables);
			Assert(splitvariables.size() > 0);
			for (long j = (long) splitvariables.size() - 1; j > -1; --j) {
				auto subformulas = splitformulalevels[j];
				if (prevform != NULL) {
					subformulas.push_back(prevform);
				} else {
					// The previous formula was NULL, so this is the very first element we're constructing of our new formula.
					if (subformulas.empty()) {
						// If no formulas are put into this level, create an empty boolform of the
						// same type (this is the construction of ^2 as seen above)
						subformulas.push_back(new BoolForm(SIGN::POS, conj,{}, FormulaParseInfo()));
					}
				}
				if (subformulas.size() == 1) {
					// If this "level" only contains a single subformula, put this subformula directly beneath the quantform
					prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), {
						splitvariables[j] }, subformulas.back(), quantforms[i]->pi());
				} else {
					// If this "level" only contains multiple subformulas, join them in a boolform using the same
					// connector as the original bsubf (this is the construction of ^1 connectors in the above example)
					prevform = new QuantForm(SIGN::POS, quantforms[i]->quant(), {
						splitvariables[j] },
					new BoolForm(SIGN::POS, conj, subformulas, bsubf->pi()), quantforms[i]->pi());
				}
			}
			if (nonquantformulas.size() > 0) {
				nonquantformulas.push_back(prevform);
				prevform = new BoolForm(SIGN::POS, conj, nonquantformulas, prevform->pi());
			}
		}
		auto subformulas = formulalevels[0];
		Assert(prevform != NULL);
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

// Recursive method that pushes a quantified variable as deep as possible.
// returns true if the variable was pushable through f and the push was performed
bool pushQuantifiedVariableThrough(Variable* v, bool univ/*ersally quantified*/, Formula* f) {
	if (f->sign() == SIGN::NEG) {
		return false; // this method currently works best if all negations are pushed TODO: fix this
	}
	if (f->freeVars().count(v) == 0) { // variable doesn't occur, so pushable
		return true;
	}

	// try to push a quantified variable v as deep as possible
	// which is possible when
	// - a quantified subformula has the same quantifier as v
	// - a boolean subformula's conjunctivity status is equal to v's universality
	// - a boolean subformula's conjunctivity status is opposite to v's universality, but not all subformulas use v

	auto quantf = dynamic_cast<QuantForm*> (f);
	if (quantf != NULL) {
		if (quantf->isUniv() != univ) {
			return false;
		} else { // we can push through this one, e.g. !x y: P(x,y) | Q(y) --> !y x: P(x,y) | Q(y) (now x can be pushed past Q(y))
			if (!pushQuantifiedVariableThrough(v, univ, quantf->subformula())) {
				quantf->addQuantVar(v); // add v to the current quantifiers.
			}
			return true;
		}
	}

	auto boolf = dynamic_cast<BoolForm*> (f);
	if (boolf != NULL) {
		// first, gather all indexes for subformulas in which v occurs:
		std::vector<size_t> v_inds;
		for (size_t i = 0; i < boolf->subformulas().size(); ++i) {
			if (boolf->subformulas().at(i)->freeVars().count(v) > 0) {
				v_inds.push_back(i);
			}
		}
		// NOTE: at least one subformula has v as free variable, since we would have returned true at the start of this method otherwise
		Assert(v_inds.size() > 0);
		if (boolf->conj() == univ) { // we can push to every subf with index in v_inds , e.g. !x: P(x) & Q(x) & P(y) --> !x: P(x) & !x': Q(x') & P(y)
			// no need to introduce new variable for first subformula
			size_t firstIndex = v_inds.at(0);
			Formula* subf = boolf->subformulas().at(firstIndex);
			if (!pushQuantifiedVariableThrough(v, univ, subf)) {
				QuantForm* newQF = new QuantForm(SIGN::POS, univ ? QUANT::UNIV : QUANT::EXIST,{v}, subf, FormulaParseInfo());
				boolf->subformula(firstIndex, newQF);
			}
			for (size_t i = 1; i < v_inds.size(); ++i) {
				// introduce a new variable for other subformulas
				size_t index = v_inds.at(i);
				auto subf = boolf->subformulas().at(index);
				Variable* newV = new Variable(v->sort());
				std::map<Variable*, Variable*> tmpmap;
				tmpmap.insert({v, newV});
				FormulaUtils::substituteVarWithVar(subf, tmpmap);
				if (!pushQuantifiedVariableThrough(newV, univ, subf)) { // then this is the level to make the quantification over newV
					QuantForm* newQF = new QuantForm(SIGN::POS, univ ? QUANT::UNIV : QUANT::EXIST,{newV}, subf, FormulaParseInfo());
					boolf->subformula(index, newQF);
				}
			}
			return true;
		} else { // now for the case where boolf->conj()!=univ
            if (v_inds.size() == boolf->subformulas().size()) {
				return false; // cannot push through any subformula
            } else if (v_inds.size() == 1) {// e.g. !y: P(x) | Q(y) --> P(x) | !y:Q(y)
				size_t firstIndex = v_inds.at(0);
				Formula* subf = boolf->subformulas().at(firstIndex);
				if (!pushQuantifiedVariableThrough(v, univ, subf)) {
					QuantForm* newQF = new QuantForm(SIGN::POS, univ ? QUANT::UNIV : QUANT::EXIST,{v}, subf, FormulaParseInfo());
					boolf->subformula(firstIndex, newQF);
				}
				return true;
			} else { // can push through at least one, but also not through at least two
				// example where pushing would be useful: ?x: !y: P(x) | Q(y) | R(y) --> [?x:P(x)] | [!y:Q(y)|R(y)]
				// create a new sibling (to boolf) quantform with boolform child (with all boolf subforms that contain v)
				std::vector<Formula*> childs;
				std::vector<Formula*> siblings;
				for (auto sf : boolf->subformulas()) {
					if (sf->freeVars().count(v) > 0) {
						childs.push_back(sf);
					} else {
						siblings.push_back(sf);
					}
				}
				BoolForm* newBF = new BoolForm(SIGN::POS, boolf->conj(), childs, FormulaParseInfo());
				QuantForm* newQF = new QuantForm(SIGN::POS, univ ? QUANT::UNIV : QUANT::EXIST,{v}, newBF, FormulaParseInfo());
				siblings.push_back(newQF);
				boolf->subformulas(siblings);
				return true;
			}
		}
	}
	return false;
}

Rule* PushQuantificationsCompletely::visit(Rule* rule) {
	// first handle the body:
	Formula* newBod = rule->body()->accept(this);
	if (newBod != rule->body()) {
		delete rule->body(); // a shallow delete, since subsubformulas are still needed
		rule->body(newBod);
	}

	// now check which quantifiers can be pushed through the rule
	varset unpushableBodyVars, rem;
	for (auto var : rule->quantVars()) {
		if (not contains(rule->head()->freeVars(), var)) { // we can push the quantifier to the body
			if (!pushQuantifiedVariableThrough(var, false, rule->body())) {
				unpushableBodyVars.insert(var);
			}
		} else {
			rem.insert(var);
		}
	}
	if (unpushableBodyVars.size() > 0) { // introduce an existential quantor
		rule->body(new QuantForm(SIGN::POS, QUANT::EXIST, unpushableBodyVars, rule->body(), FormulaParseInfo()));
		rule->setQuantVars(rem);
	}
    rule->body()->accept(this);
	return rule;
}

Formula* PushQuantificationsCompletely::visit(QuantForm* qf) {
	// first handle the inner formula:
	Formula* newSub = qf->subformula()->accept(this);
	if (newSub != qf->subformula()) {
		delete qf->subformula(); // hoping for a shallow delete, since subsubformulas are still needed
		qf->subformula(newSub);
	}

	std::vector<Variable*> quantVars; // local copy
	for (auto v : qf->quantVars()) {
		quantVars.push_back(v);
	}
	qf->quantVars({}); // erase all quantified variables, we're going to readd them when needed

	for (auto v : quantVars) {
		pushQuantifiedVariableThrough(v, qf->isUniv(), qf); // NOTE: answer is always true, since qf is quantified with the right quantor (being its own ;)
	}

	Formula* result = nullptr;
	if (qf->quantVars().size() == 0) {
		result = qf->subformula();
	} else {
		result = qf;
	}
	return FormulaUtils::flatten(result);
}