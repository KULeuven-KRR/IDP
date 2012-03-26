/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef TABLECOSTESTIMATOR_HPP_
#define TABLECOSTESTIMATOR_HPP_

#include <vector>
#include <cmath>
#include "utils/NumericLimits.hpp"

#include "visitors/StructureVisitor.hpp"

// TODO review

class EstimateBDDInferenceCost: public StructureVisitor {
private:
	const PredTable* _table;
	std::vector<bool> _pattern; //!< _pattern[n] == true iff the n'th column is an input column
	double _result;

	double maxCost() {
		return getMaxElem<double>();
	}
public:

	double run(const PredTable* t, const std::vector<bool>& p) {
		_table = t;
		_pattern = p;
		t->internTable()->accept(this);
		return _result;
	}

	void visit(const ProcInternalPredTable*) {
		double maxdouble = maxCost();
		double sz = 1;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (not _pattern[n]) {
				tablesize ts = _table->universe().tables()[n]->size();
				if (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
					if (sz * ts._size < maxdouble) {
						sz = sz * ts._size;
					} else {
						_result = maxdouble;
						return;
					}
				} else {
					_result = maxdouble;
					return;
				}
			}
		}
		// NOTE: We assume that evaluation of a lua function has cost 5
		// Can be adapted if this turns out to be unrealistic
		if (5 * sz < maxdouble)
			_result = 5 * sz;
		else
			_result = maxdouble;
	}

	void visit(const BDDInternalPredTable* bipt) {
		auto manager = bipt->manager();
		auto bdd = bipt->bdd();
		auto vars = bipt->vars();
		std::set<const FOBDDVariable*> bddvars;
		int i = 0;
		Assert(_pattern.size() == vars.size());
		for (auto v = vars.cbegin(); v != vars.cend(); ++v, ++i) {
			if (not _pattern[i]) {
				bddvars.insert(manager->getVariable(*v));
			}
		}
		_result = manager->estimatedCostAll(bdd, bddvars, { }, bipt->structure());
	}

	void visit(const FullInternalPredTable*) {
		double maxdouble = maxCost();
		double sz = 1;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (!_pattern[n]) {
				tablesize ts = _table->universe().tables()[n]->size();
				if (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
					if (sz * ts._size < maxdouble) {
						sz = sz * ts._size;
					} else {
						_result = maxdouble;
						return;
					}
				} else {
					_result = maxdouble;
					return;
				}
			}
		}
		_result = sz;
	}

	void visit(const FuncInternalPredTable* t) {
		t->table()->internTable()->accept(this);
	}

	void visit(const UnionInternalPredTable*) {
		throw notyetimplemented("EstimateBDDInference for UnionInternalPredTable");
		// TODO
	}

	void visit(const UnionInternalSortTable*) {
		throw notyetimplemented("EstimateBDDInference for UnionInternalSortTable");
		// TODO
	}

	void visit(const EnumeratedInternalSortTable*) {
		if (_pattern[0]) {
			_result = log(double(_table->size()._size)) / log(2);
		} else {
			_result = _table->size()._size;
		}
	}

	void visit(const IntRangeInternalSortTable*) {
		if (_pattern[0])
			_result = 1;
		else
			_result = _table->size()._size;
	}

	void visit(const EnumeratedInternalPredTable*) {
		double maxdouble = maxCost();
		double inputunivsize = 1;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n]) {
				tablesize ts = _table->universe().tables()[n]->size();
				if (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
					if (inputunivsize * ts._size < maxdouble) {
						inputunivsize = inputunivsize * ts._size;
					} else {
						_result = maxdouble;
						return;
					}
				} else {
					_result = maxdouble;
					return;
				}
			}
		}
		tablesize ts = _table->size();
		double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
		if (log(lookupsize) / log(2) < maxdouble) {
			double lookuptime = log(lookupsize) / log(2);
			double iteratetime = ts._size / inputunivsize;
			if (lookuptime + iteratetime < maxdouble)
				_result = lookuptime + iteratetime;
			else
				_result = maxdouble;
		} else
			_result = maxdouble;
	}

	void visit(const EqualInternalPredTable*) {
		if (_pattern[0])
			_result = 1;
		else if (_pattern[1])
			_result = 1;
		else {
			tablesize ts = _table->universe().tables()[0]->size();
			if (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED)
				_result = ts._size;
			else
				_result = maxCost();
		}
	}

	void visit(const StrLessInternalPredTable*) {
		if (_pattern[0]) {
			if (_pattern[1])
				_result = 1;
			else {
				tablesize ts = _table->universe().tables()[0]->size();
				if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
					_result = ts._size / double(2);
				else
					_result = maxCost();
			}
		} else if (_pattern[1]) {
			tablesize ts = _table->universe().tables()[0]->size();
			if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
				_result = ts._size / double(2);
			else
				_result = maxCost();
		} else {
			tablesize ts = _table->size();
			if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
				_result = ts._size;
			else
				_result = maxCost();
		}
	}

	void visit(const StrGreaterInternalPredTable*) {
		if (_pattern[0]) {
			if (_pattern[1])
				_result = 1;
			else {
				tablesize ts = _table->universe().tables()[0]->size();
				if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
					_result = ts._size / double(2);
				else
					_result = maxCost();
			}
		} else if (_pattern[1]) {
			tablesize ts = _table->universe().tables()[0]->size();
			if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
				_result = ts._size / double(2);
			else
				_result = maxCost();
		} else {
			tablesize ts = _table->size();
			if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT)
				_result = ts._size;
			else
				_result = maxCost();
		}
	}

	void visit(const InverseInternalPredTable* t) {
		if (typeid(*(t->table())) == typeid(InverseInternalPredTable)) {
			const InverseInternalPredTable* nt = dynamic_cast<const InverseInternalPredTable*>(t->table());
			nt->table()->accept(this);
		} else {
			double maxdouble = maxCost();
			EstimateBDDInferenceCost tce;
			PredTable npt(t->table(), _table->universe());
			double lookuptime = tce.run(&npt, std::vector<bool>(_pattern.size(), true));
			double sz = 1;
			for (unsigned int n = 0; n < _pattern.size(); ++n) {
				if (!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if (sz * ts._size < maxdouble) {
							sz = sz * ts._size;
						} else {
							_result = maxdouble;
							return;
						}
					} else {
						_result = maxdouble;
						return;
					}
				}
			}
			if (sz * lookuptime < maxdouble)
				_result = sz * lookuptime;
			else
				_result = maxdouble;
		}
	}

	void visit(const ProcInternalFuncTable*) {
		double maxdouble = maxCost();
		double sz = 1;
		for (unsigned int n = 0; n < _pattern.size() - 1; ++n) {
			if (!_pattern[n]) {
				tablesize ts = _table->universe().tables()[n]->size();
				if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
					if (sz * ts._size < maxdouble) {
						sz = sz * ts._size;
					} else {
						_result = maxdouble;
						return;
					}
				} else {
					_result = maxdouble;
					return;
				}
			}
		}
		// NOTE: We assume that evaluation of a lua function has cost 5
		// Can be adapted if this turns out to be unrealistic
		if (5 * sz < maxdouble)
			_result = 5 * sz;
		else
			_result = maxdouble;
	}

	void visit(const UNAInternalFuncTable*) {
		if (_pattern.back()) {
			unsigned int patterncount = 0;
			for (unsigned int n = 0; n < _pattern.size() - 1; ++n) {
				if (_pattern[n])
					++patterncount;
			}
			_result = patterncount;
		} else {
			double maxdouble = maxCost();
			double sz = 1;
			for (unsigned int n = 0; n < _pattern.size() - 1; ++n) {
				if (!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if (sz * ts._size < maxdouble) {
							sz = sz * ts._size;
						} else {
							_result = maxdouble;
							return;
						}
					} else {
						_result = maxdouble;
						return;
					}
				}
			}
			_result = sz;
		}
	}

	void visit(const EnumeratedInternalFuncTable*) {
		double maxdouble = maxCost();
		double inputunivsize = 1;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n]) {
				tablesize ts = _table->universe().tables()[n]->size();
				if (ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
					if (inputunivsize * ts._size < maxdouble) {
						inputunivsize = inputunivsize * ts._size;
					} else {
						_result = maxdouble;
						return;
					}
				} else {
					_result = maxdouble;
					return;
				}
			}
		}
		tablesize ts = _table->size();
		double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
		if (log(lookupsize) / log(2) < maxdouble) {
			double lookuptime = log(lookupsize) / log(2);
			double iteratetime = ts._size / inputunivsize;
			if (lookuptime + iteratetime < maxdouble)
				_result = lookuptime + iteratetime;
			else
				_result = maxdouble;
		} else
			_result = maxdouble;
	}

	void visit(const PlusInternalFuncTable*) {
		unsigned int patterncount = 0;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n])
				++patterncount;
		}
		if (patterncount >= 2)
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const MinusInternalFuncTable*) {
		unsigned int patterncount = 0;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n])
				++patterncount;
		}
		if (patterncount >= 2)
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const TimesInternalFuncTable*) {
		unsigned int patterncount = 0;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n])
				++patterncount;
		}
		if (patterncount >= 2)
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const DivInternalFuncTable*) {
		unsigned int patterncount = 0;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_pattern[n])
				++patterncount;
		}
		if (patterncount >= 2)
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const AbsInternalFuncTable*) {
		if (_pattern[0])
			_result = 1;
		else if (_pattern[1])
			_result = 2;
		else
			_result = maxCost();
	}

	void visit(const UminInternalFuncTable*) {
		if (_pattern[0] || _pattern[1])
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const ExpInternalFuncTable*) {
		if (_pattern[0] && _pattern[1])
			_result = 1;
		else
			_result = maxCost();
	}

	void visit(const ModInternalFuncTable*) {
		if (_pattern[0] && _pattern[1])
			_result = 1;
		else
			_result = maxCost();
	}
};

#endif /* TABLECOSTESTIMATOR_HPP_ */
