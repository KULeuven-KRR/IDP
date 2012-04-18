/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "AddCompletion.hpp"

#include "IncludeComponents.hpp"

using namespace std;

Theory* AddCompletion::visit(Theory* theory) {
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		(*it)->accept(this);
		for (auto jt = _result.cbegin(); jt != _result.cend(); ++jt)
			theory->add(*jt);
		(*it)->recursiveDelete();
	}
	theory->definitions().clear();
	return theory;
}

Definition* AddCompletion::visit(Definition* def) {
	_headvars.clear();
	_interres.clear();
	_result.clear();
	for (auto it = def->defsymbols().cbegin(); it != def->defsymbols().cend(); ++it) {
		vector<Variable*> vv;
		for (auto jt = (*it)->sorts().cbegin(); jt != (*it)->sorts().cend(); ++jt) {
			vv.push_back(new Variable(*jt));
		}
		_headvars[*it] = vv;
	}
	for (auto it = def->rules().cbegin(); it != def->rules().cend(); ++it) {
		(*it)->accept(this);
	}

	for (auto it = _interres.cbegin(); it != _interres.cend(); ++it) {
		Assert(!it->second.empty());
		Formula* b = it->second[0];
		if (it->second.size() > 1)
			b = new BoolForm(SIGN::POS, false, it->second, FormulaParseInfo());
		PredForm* h = new PredForm(SIGN::POS, it->first, TermUtils::makeNewVarTerms(_headvars[it->first]), FormulaParseInfo());
		EquivForm* ev = new EquivForm(SIGN::POS, h, b, FormulaParseInfo());
		if (it->first->sorts().empty()) {
			_result.push_back(ev);
		} else {
			set<Variable*> qv(_headvars[it->first].cbegin(), _headvars[it->first].cend());
			QuantForm* qf = new QuantForm(SIGN::POS, QUANT::UNIV, qv, ev, FormulaParseInfo());
			_result.push_back(qf);
		}
	}

	return def;
}

Rule* AddCompletion::visit(Rule* rule) {
	vector<Formula*> vf;
	vector<Variable*> vv = _headvars[rule->head()->symbol()];
	set<Variable*> freevars = rule->quantVars();
	map<Variable*, Variable*> mvv;

	for (size_t n = 0; n < rule->head()->subterms().size(); ++n) {
		Term* t = rule->head()->subterms()[n];
		if (typeid(*t) != typeid(VarTerm)) {
			VarTerm* bvt = new VarTerm(vv[n], TermParseInfo());
			vector<Term*> args;
			args.push_back(bvt);
			args.push_back(t->clone());
			Predicate* p = Vocabulary::std()->pred("=/2")->resolve(vector<Sort*>(2, vv[n]->sort()));
			PredForm* pf = new PredForm(SIGN::POS, p, args, FormulaParseInfo());
			vf.push_back(pf);
		} else {
			Variable* v = *(t->freeVars().cbegin());
			if (mvv.find(v) == mvv.cend()) {
				mvv[v] = vv[n];
				freevars.erase(v);
			} else {
				VarTerm* bvt1 = new VarTerm(vv[n], TermParseInfo());
				VarTerm* bvt2 = new VarTerm(mvv[v], TermParseInfo());
				vector<Term*> args;
				args.push_back(bvt1);
				args.push_back(bvt2);
				Predicate* p = Vocabulary::std()->pred("=/2")->resolve(vector<Sort*>(2, v->sort()));
				PredForm* pf = new PredForm(SIGN::POS, p, args, FormulaParseInfo());
				vf.push_back(pf);
			}
		}
	}
	Formula* b = rule->body();
	if (!vf.empty()) {
		vf.push_back(b);
		b = new BoolForm(SIGN::POS, true, vf, FormulaParseInfo());
	}
	if (!freevars.empty()) {
		b = new QuantForm(SIGN::POS, QUANT::EXIST, freevars, b, FormulaParseInfo());
	}
	b = b->clone(mvv);
	_interres[rule->head()->symbol()].push_back(b);

	return rule;
}
