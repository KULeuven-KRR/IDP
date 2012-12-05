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

#pragma once

#include <vector>
#include <cmath>
#include "utils/NumericLimits.hpp"
#include "fobdds/Estimations.hpp"

#include "visitors/StructureVisitor.hpp"
#include "structure/StructureComponents.hpp"

#define InfCost getMaxElem<double>()

/**
 * Returns the cost of iterating over all solutions of the BDD given an input/output pattern.
 */
class EstimateEnumerationCost: public StructureVisitor {
private:
	const PredTable* _table;
	std::vector<bool> _pattern; //!< _pattern[n] == true iff the n'th column is an input column
	double _result;
	int luacost; // NOTE: estimation of the additional cost of a call through lua

public:
	double run(const PredTable* table, const std::vector<bool>& pattern) {
		_table = table;
		_pattern = pattern;
		luacost = 5;
		_result = -1;
		table->internTable()->accept(this);
		Assert(_result>=0);
		return _result;
	}

	double getTableCost(bool function) {
		double sz = 1;
		auto max = function ? _pattern.size() - 1 : _pattern.size();
		for (unsigned int n = 0; n < max; ++n) {
			if (_pattern[n]) {
				continue;
			}
			sz *= _table->universe().tables()[n]->size();
		}
		return sz;
	}

	void visit(const ProcInternalPredTable*) {
		_result = getTableCost(false) * luacost;
	}

	void visit(const ProcInternalFuncTable*) {
		_result = getTableCost(true) * luacost;
	}

	void visit(const BDDInternalPredTable* bipt) {
		auto manager = bipt->manager();
		auto vars = bipt->vars();
		Assert(_pattern.size() == vars.size());

		fobddvarset bddvars;
		int i = 0;
		for (auto v = vars.cbegin(); v != vars.cend(); ++v, ++i) {
			if (not _pattern[i]) {
				bddvars.insert(manager->getVariable(*v));
			}
		}
		_result = BddStatistics::estimateCostAll(bipt->bdd(), bddvars, { }, bipt->structure(), manager);
	}

	void visit(const FullInternalPredTable*) {
		_result = getTableCost(false);
	}

	void visit(const FuncInternalPredTable* t) {
		t->table()->internTable()->accept(this);
	}

	void visit(const UnionInternalPredTable* uipt) {
		double result = 0;
		for (auto it = uipt->inTables().cbegin(); result < InfCost&& it != uipt->inTables().cend(); ++it) {
			(*it)->accept(this);
			result = result + _result;
		}
		for (auto it = uipt->outTables().cbegin(); result < InfCost&& it != uipt->outTables().cend(); ++it) {
			(*it)->accept(this);
			result = result + _result;
		}
		_result = result;
	}

	void visit(const UnionInternalSortTable*) {
		throw notyetimplemented("BDD Inference cost for UnionInternalSortTable");
	}

	void visit(const EnumeratedInternalSortTable*) {
		if (_pattern[0]) {
			auto size = toDouble(_table->size());
			if(size==0){
				_result=1;
			}else{
				_result = log(size) / log(2);
			}
		} else {
			_result = toDouble(_table->size());
		}
	}

	void visit(const ConstructedInternalSortTable* cist) {
		if (_pattern[0]) {
			auto size = toDouble(_table->size());
			if(size==0){
				_result = 1;
			}else{
				_result = cist->nrOfConstructors() + log(ceil(size/cist->nrOfConstructors())) / log(2);
			}
		} else {
			_result = toDouble(_table->size());
		}
	}

	void visit(const IntRangeInternalSortTable*) {
		if (_pattern[0]) {
			_result = 1;
		} else {
			_result = toDouble(_table->size());
		}
	}

	void getEnumeratedTableCost(bool function) {
		auto sz = getTableCost(function);
		auto ts = toDouble(_table->size());
		double lookupsize = sz < ts ? sz : ts;
		if (lookupsize == 0) {
			_result = 1;
			return;
		}
		auto lookuptime = log(lookupsize) / log(2);
		if (lookuptime < InfCost) {
			auto iteratetime = ts / sz;
			if (lookuptime + iteratetime < InfCost) {
				_result = lookuptime + iteratetime;
				return;
			}
		}
		_result = sz;
	}

	void visit(const EnumeratedInternalPredTable*) {
		getEnumeratedTableCost(false);
	}

	void visit(const EnumeratedInternalFuncTable*) {
		getEnumeratedTableCost(true);
	}

	void visit(const EqualInternalPredTable*) {
		if (_pattern[0] || _pattern[1]) {
			_result = 1;
		} else {
			_result = toDouble(_table->universe().tables()[0]->size());
		}
	}

	void setInfSortComparisonCost() {
		if (_pattern[0]) {
			if (_pattern[1]) {
				_result = 1;
			} else {
				_result = toDouble(_table->universe().tables()[0]->size() / (double) 2);
			}
		} else if (_pattern[1]) {
			_result = toDouble(_table->universe().tables()[1]->size() / (double) 2);
		} else {
			_result = toDouble(_table->size());
		}
	}

	void visit(const StrLessInternalPredTable*) {
		setInfSortComparisonCost();
	}

	void visit(const StrGreaterInternalPredTable*) {
		setInfSortComparisonCost();
	}

	void visit(const InverseInternalPredTable* t) {
		if (isa<InverseInternalPredTable>(*t->table())) {
			auto nt = dynamic_cast<const InverseInternalPredTable*>(t->table());
			nt->table()->accept(this);
			return;
		}

		EstimateEnumerationCost tce;
		PredTable npt(t->table(), _table->universe());
		auto lookuptime = tce.run(&npt, std::vector<bool>(_pattern.size(), true));
		_result = getTableCost(false) * lookuptime;
	}

	int getNbInputs(bool alsolastelem) {
		unsigned int patterncount = 0;
		auto max = alsolastelem ? _pattern.size() - 1 : _pattern.size();
		for (unsigned int n = 0; n < max; ++n) {
			if (_pattern[n]) {
				++patterncount;
			}
		}
		return patterncount;
	}

	void visit(const UNAInternalFuncTable*) {
		if (_pattern.back()) {
			_result = getNbInputs(true);
		} else {
			_result = getTableCost(true);
		}
	}

	void setCostForMinNbOfInputs(int min, int costthen) {
		if (getNbInputs(false) >= min) {
			_result = costthen;
		} else {
			_result = InfCost;
		}
	}

	void visit(const PlusInternalFuncTable*) {
		setCostForMinNbOfInputs(2, 1);
	}

	void visit(const MinusInternalFuncTable*) {
		setCostForMinNbOfInputs(2, 1);
	}

	void visit(const TimesInternalFuncTable*) {
		setCostForMinNbOfInputs(2, 1);
	}

	void visit(const DivInternalFuncTable*) {
		setCostForMinNbOfInputs(2, 1);
	}

	void visit(const AbsInternalFuncTable*) {
		if (_pattern[0]) {
			_result = 1;
		} else if (_pattern[1]) {
			_result = 2;
		} else {
			_result = InfCost;
		}
	}

	void visit(const UminInternalFuncTable*) {
		setCostForMinNbOfInputs(1, 1);
	}

	void visit(const ExpInternalFuncTable*) {
		if (_pattern[0] && _pattern[1]) {
			_result = 1;
		} else {
			_result = InfCost;
		}
	}

	void visit(const ModInternalFuncTable*) {
		if (_pattern[0] && _pattern[1]) {
			_result = 1;
		} else {
			_result = InfCost;
		}
	}

	void visit(const AllIntegers*) {
		_result = InfCost;
	}
	void visit(const AllChars*) {
		_result = InfCost;
	}
	void visit(const AllFloats*) {
		_result = InfCost;
	}
	void visit(const AllNaturalNumbers*) {
		_result = InfCost;
	}
	void visit(const AllStrings*) {
		_result = InfCost;
	}
};
