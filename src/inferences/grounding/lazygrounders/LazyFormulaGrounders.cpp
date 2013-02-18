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

#include "LazyFormulaGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

LazyUnknUnivGrounder::LazyUnknUnivGrounder(const PredForm* pf, Context context, const var2dommap& varmapping, AbstractGroundTheory* groundtheory,
		FormulaGrounder* sub, const GroundingContext& ct)
		: 	FormulaGrounder(groundtheory, ct),
			DelayGrounder(pf->symbol(), pf->args(), context, -1, groundtheory),
			_subgrounder(sub) {
	if (verbosity() > 2) {
		clog << "Delaying the grounding " << print(sub) << " on " << print(pf) << ".\n";
	}

	for (auto i = pf->args().cbegin(); i < pf->args().cend(); ++i) {
		auto var = dynamic_cast<VarTerm*>(*i)->var();
		_varcontainers.push_back(varmapping.at(var));
	}
}

void LazyUnknUnivGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
}

// set the variable instantiations
dominstlist LazyUnknUnivGrounder::createInst(const ElementTuple& args) {
	dominstlist domlist;
	for (size_t i = 0; i < args.size(); ++i) {
		domlist.push_back(dominst { _varcontainers[i], args[i] });
	}
	return domlist;
}

// Returns a list of indices in List which contain the same variable
std::vector<pair<int, int> > findSameArgs(const vector<Term*>& terms) {
	std::vector<pair<int, int> > sameargs;
	std::map<Variable*, int> vartofirstocc;
	int index = 0;
	for (auto i = terms.cbegin(); i < terms.cend(); ++i, ++index) {
		auto varterm = dynamic_cast<VarTerm*>(*i);
		if (varterm == NULL) { // If not a var, it cannot contain free variables!
			Assert((*i)->freeVars().size()==0);
			continue;
		}

		auto first = vartofirstocc.find(varterm->var());
		if (first == vartofirstocc.cend()) {
			vartofirstocc[varterm->var()] = index;
		} else {
			sameargs.push_back( { first->second, index });
		}
	}
	return sameargs;
}

DelayGrounder::DelayGrounder(PFSymbol* symbol, const vector<Term*>& terms, Context context, DefId id, AbstractGroundTheory* gt)
		: 	_id(id),
			_context(context),
			_isGrounding(false),
			_grounding(gt) {
	Assert(gt!=NULL);
	getGrounding()->translator()->notifyDelay(symbol, this);
	sameargs = findSameArgs(terms);
}

void DelayGrounder::notify(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders) {
	getGrounding()->notifyUnknBound(_context, lit, args, grounders);
}

void DelayGrounder::ground(const Lit& boundlit, const ElementTuple& args) {
	_stilltoground.push( { boundlit, args });
	if (not _isGrounding) {
		doGrounding();
	}
}

void DelayGrounder::doGrounding() {
	_isGrounding = true;
	while (not _stilltoground.empty()) {
		auto elem = _stilltoground.front();
		_stilltoground.pop();

		doGround(elem.first, elem.second);
	}
	_isGrounding = false;
}

void LazyUnknUnivGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);
	for (auto i = getSameargs().cbegin(); i < getSameargs().cend(); ++i) {
		if (headargs[i->first] != headargs[i->second]) {
			return;
		}
	}

	dominstlist boundvarinstlist = createInst(headargs);

	vector<const DomainElement*> originst;
	overwriteVars(originst, boundvarinstlist);

	ConjOrDisj formula;
	_subgrounder->wrapRun(formula);
	addToGrounding(Grounder::getGrounding(), formula);

	restoreOrigVars(originst, boundvarinstlist);
}

// @Precon: terms should be first those of the first predform, followed by those of the second
LazyTwinDelayUnivGrounder::LazyTwinDelayUnivGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, const var2dommap& varmapping,
		AbstractGroundTheory* groundtheory, FormulaGrounder* sub, const GroundingContext& ct)
		: 	FormulaGrounder(groundtheory, ct),
			DelayGrounder(symbol, terms, context, -1, groundtheory),
			_subgrounder(sub) {
	if (verbosity() > 1) {
		clog << "Delaying the grounding " << print(sub) << " on " << "TODO" << " and " << "TODO" << ".\n"; // TODO
	}
	for (auto i = terms.cbegin(); i < terms.cend(); ++i) {
		auto var = dynamic_cast<VarTerm*>(*i)->var();
		Assert(var!=NULL);
		Assert(varmapping.find(var)!=varmapping.cend());
		_varcontainers.push_back(varmapping.at(var));
	}
}

void LazyTwinDelayUnivGrounder::run(ConjOrDisj& formula) const {
	formula.setType(Conn::CONJ);
}

// set the variable instantiations
dominstlist LazyTwinDelayUnivGrounder::createInst(const ElementTuple& args) {
	dominstlist domlist;
	for (size_t i = 0; i < args.size(); ++i) {
		domlist.push_back(dominst { _varcontainers[i], args[i] });
	}
	return domlist;
}

void LazyTwinDelayUnivGrounder::doGround(const Lit& head, const ElementTuple& headargs) {
	Assert(head!=_true && head!=_false);

	_seen.push_back(headargs);

	for (auto other = _seen.cbegin(); other < _seen.cend(); ++other) {
		auto tuple = *other;
		insertAtEnd(tuple, headargs);

		// If multiple vars are the same, checks that their instantiation are also the same!
		bool different = false;
		for (auto i = getSameargs().cbegin(); not different && i < getSameargs().cend(); ++i) {
			if (tuple[i->first] != tuple[i->second]) {
				different = true;
			}
		}
		if (different) {
			continue;
		}

		dominstlist boundvarinstlist = createInst(tuple);

		vector<const DomainElement*> originst;
		overwriteVars(originst, boundvarinstlist);

		ConjOrDisj formula;
		_subgrounder->wrapRun(formula);
		addToGrounding(Grounder::getGrounding(), formula);

		restoreOrigVars(originst, boundvarinstlist);
	}
}
