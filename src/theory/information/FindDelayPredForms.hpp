/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "creation/cppinterface.hpp"

/**
 *	Given a formula F, find a literal L such that,
 *		for each interpretation I in which L^I=unknown,
 *			then either I[L=T] \models F or I[L=F] \models F.
 */

class FindDelayPredForms: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _invaliddelay;
	bool _rootquant, _onlyexistsquant, _somequant;
	bool _allquantvars;
	const LazyGroundingManager* _manager;
	var2dommap _varmapping;

	varset _quantvars;
	TruthValue _context;

	std::shared_ptr<Delay> delay;

public:
	template<typename T>
	std::shared_ptr<Delay> execute(T t, const var2dommap& varmap, const LazyGroundingManager* manager) {
		if (getOption(VERBOSE_GROUNDING) > 3) {
			clog << "Searching for delays for " << print(t) << ".\n";
		}
		_rootquant = true;
		_onlyexistsquant = true;
		_somequant = false;
		_invaliddelay = true;
		_manager = manager;
		_varmapping = varmap;

		t->accept(this);

		if (_invaliddelay) {
			delay.reset();
		}

		return delay;
	}

protected:
	virtual void visit(const PredForm* pf) {
		if (_somequant && _onlyexistsquant) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << print(pf) << " not eligible for delays because only existential quantification.\n";
			}
			return;
		}

		auto newpf = pf;
//		// TODO to be enabled when functions can be used as delays
//		if (is(pf->symbol(), STDPRED::EQ) && TermUtils::isFunc(pf->args()[0])) {
//			auto ft = dynamic_cast<FuncTerm*>(pf->args()[0]);
//			auto newargs = ft->args();
//			newargs.push_back(pf->args()[1]);
//			bool nested = false;
//			for (auto arg : newargs) {
//				if (not TermUtils::isVarOrDom(arg)) {
//					nested = true;
//					break;
//				}
//			}
//			if (not nested) {
//				newpf = new PredForm(pf->sign(), ft->function(), newargs, pf->pi());
//			}
//		}

		auto symbol = newpf->symbol();
		const auto& args = newpf->args();
		auto sign = newpf->sign();

		auto watchtrue = true;
		if (isNeg(sign)) {
			watchtrue = not watchtrue;
		}

		if (not _manager->canBeDelayedOn(symbol, not watchtrue) || pf->symbol()->builtin() || pf->symbol()->overloaded()) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << print(pf) << " not eligible for delays because builtin, overloaded or negation already constructed.\n";
			}
			return;
		}

		if (not _invaliddelay) {
			for (auto delelem : delay->condition) {
				if (delelem.symbol == symbol && delelem.watchedvalue != not watchtrue) {
					if (getOption(VERBOSE_GROUNDING) > 3) {
						clog << print(pf) << " not eligible for delays because negation is already watched.\n";
					}
					return;
				}
			}
		}

		if (_manager->getStructure() != NULL && getOption(GROUNDWITHBOUNDS) && _manager->getStructure()->inter(symbol)->approxTwoValued()) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << print(pf) << " not eligible for delays because the structure is twovalued (and we expect the approximation to handle it then).\n";
			}
			return;
		}
		if (_manager->getFiredLits(pf->symbol(), not watchtrue).size() >= toDouble(_manager->getStructure()->inter(pf->symbol())->universe().size()) / 2) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << print(pf) << " not eligible for delays because most than half of the lits already fired.\n";
			}
			return;
		}

		_allquantvars = true;
		for (auto arg : args) {
			arg->accept(this);
		}
		if (not _allquantvars) { // TODO should remove this requirement?
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << print(pf) << " not eligible for delays because it does not cover all quantified variables.\n";
			}
			return;
		}

		if (_invaliddelay) {
			delay = make_shared<Delay>();
			_invaliddelay = false;
		}

		std::vector<const DomElemContainer*> containers;
		std::vector<SortTable*> tables;
		for (auto term : args) {
			auto vt = dynamic_cast<VarTerm*>(term);
			Assert(vt!=NULL);
			Assert(_varmapping.find(vt->var()) != _varmapping.cend());
			containers.push_back(_varmapping.at(vt->var()));
			tables.push_back(new SortTable(_manager->getStructure()->inter(vt->var()->sort())->internTable()));
		}
		delay->condition.push_back( { symbol, tables, containers, not watchtrue });
		if (getOption(VERBOSE_GROUNDING) > 3) {
			clog << print(pf) << " eligible as construction.\n";
		}
	}

	virtual void visit(const BoolForm* formula) {
		_rootquant = false;
		if (formula->conj()) {
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << "The formula " << print(formula) << " was not considered to find delays because it is conjunctive.\n";
			}
			return;
		}

		for (auto subformula : formula->subformulas()) {
			subformula->accept(this);
		}
	}
	virtual void visit(const QuantForm* formula) {
		if (not _rootquant) { // The quantform was encountered nested in another formula which is a not quantification. (so both !x y: and !x: !y: are still handled)
			if (getOption(VERBOSE_GROUNDING) > 3) {
				clog << "The formula " << print(formula) << " was not considered to find delays because it is not a root quantification.\n";
			}
			return;
		}
		auto savedsome = _somequant;
		_somequant = true;
		auto savedonlyexists = _onlyexistsquant;
		_onlyexistsquant &= not formula->isUnivWithSign();
		for (auto var : formula->quantVars()) {
			_quantvars.insert(var);
		}
		formula->subformula()->accept(this);
		for (auto var : formula->quantVars()) {
			Gen::atom(var->sort()->pred(), { var }).accept(this);
		}
		_onlyexistsquant = savedonlyexists;
		_somequant = savedsome;
		return;
	}

	virtual void visit(const VarTerm* vt) {
		_rootquant = false;
		if (not contains(_varmapping, vt->var())) {
			_allquantvars = false;
		}
		return;
	}
	virtual void visit(const FuncTerm*) {
		_rootquant = false;
		_allquantvars = false;
		return;
	}
	virtual void visit(const DomainTerm*) {
		_rootquant = false;
		_allquantvars = false;
		return;
	}
	virtual void visit(const AggTerm*) {
		_rootquant = false;
		_allquantvars = false;
		return;
	}

	virtual void visit(const EquivForm*) {
		_rootquant = false;
	}
	virtual void visit(const EqChainForm*) {
		_rootquant = false;
	}
	virtual void visit(const Theory*) {
		_rootquant = false;
	}
	virtual void visit(const AbstractGroundTheory*) {
		_rootquant = false;
	}
	virtual void visit(const GroundTheory<GroundPolicy>*) {
		_rootquant = false;
	}
	virtual void visit(const AggForm*) {
		_rootquant = false;
	}
	virtual void visit(const GroundDefinition*) {
		_rootquant = false;
	}
	virtual void visit(const PCGroundRule*) {
		_rootquant = false;
	}
	virtual void visit(const AggGroundRule*) {
		_rootquant = false;
	}
	virtual void visit(const GroundSet*) {
		_rootquant = false;
	}
	virtual void visit(const GroundAggregate*) {
		_rootquant = false;
	}
	virtual void visit(const CPReification*) {
		_rootquant = false;
	}
	virtual void visit(const Rule*) {
		_rootquant = false;
	}
	virtual void visit(const Definition*) {
		_rootquant = false;
	}
	virtual void visit(const FixpDef*) {
		_rootquant = false;
	}
	virtual void visit(const CPVarTerm*) {
		_rootquant = false;
	}
	virtual void visit(const CPSetTerm*) {
		_rootquant = false;
	}
	virtual void visit(const EnumSetExpr*) {
		_rootquant = false;
	}
	virtual void visit(const QuantSetExpr*) {
		_rootquant = false;
	}
};
