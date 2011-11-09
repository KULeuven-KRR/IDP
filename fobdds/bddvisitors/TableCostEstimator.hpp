/************************************
	TableCostEstimator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TABLECOSTESTIMATOR_HPP_
#define TABLECOSTESTIMATOR_HPP_

class TableCostEstimator : public StructureVisitor {
	private:
		const PredTable*	_table;
		vector<bool>		_pattern;	//!< _pattern[n] == true iff the n'th column is an input column
		double				_result;
	public:

		double run(const PredTable* t, const vector<bool>& p) {
			_table = t;
			_pattern = p;
			t->internTable()->accept(this);
			return _result;
		}

		void visit(const ProcInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(not _pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			// NOTE: We assume that evaluation of a lua function has cost 5
			// Can be adapted if this turns out to be unrealistic
			if(5 * sz < maxdouble) _result = 5 * sz;
			else _result = maxdouble;
		}

		void visit(const BDDInternalPredTable*) {
			// TODO
		}

		void visit(const FullInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
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

		void visit(const UnionInternalPredTable* ) {
			// TODO
		}

		void visit(const UnionInternalSortTable* ) {
			// TODO
		}

		void visit(const EnumeratedInternalSortTable* ) {
			if(_pattern[0]) {
				_result = log(double(_table->size()._size)) / log(2);
			}
			else {
				_result = _table->size()._size;
			}
		}

		void visit(const IntRangeInternalSortTable* ) {
			if(_pattern[0]) _result = 1;
			else _result = _table->size()._size;
		}

		void visit(const EnumeratedInternalPredTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double inputunivsize = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
						if(inputunivsize * ts._size < maxdouble) {
							inputunivsize = inputunivsize * ts._size;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			tablesize ts = _table->size();
			double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
			if(log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = log(lookupsize) / log(2);
				double iteratetime = ts._size / inputunivsize;
				if(lookuptime + iteratetime < maxdouble) _result = lookuptime + iteratetime;
				else _result = maxdouble;
			}
			else _result = maxdouble;
		}

		void visit(const EqualInternalPredTable* ) {
			if(_pattern[0]) _result = 1;
			else if(_pattern[1]) _result = 1;
			else {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) _result = ts._size;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrLessInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const StrGreaterInternalPredTable* ) {
			if(_pattern[0]) {
				if(_pattern[1]) _result = 1;
				else {
					tablesize ts = _table->universe().tables()[0]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
					else _result = numeric_limits<double>::max();
				}
			}
			else if(_pattern[1]) {
				tablesize ts = _table->universe().tables()[0]->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size / double(2);
				else _result = numeric_limits<double>::max();
			}
			else {
				tablesize ts = _table->size();
				if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) _result = ts._size;
				else _result = numeric_limits<double>::max();
			}
		}

		void visit(const InverseInternalPredTable* t) {
			if(typeid(*(t->table())) == typeid(InverseInternalPredTable)) {
				const InverseInternalPredTable* nt = dynamic_cast<const InverseInternalPredTable*>(t->table());
				nt->table()->accept(this);
			}
			else {
				double maxdouble = numeric_limits<double>::max();
				TableCostEstimator tce;
				PredTable npt(t->table(),_table->universe());
				double lookuptime = tce.run(&npt,vector<bool>(_pattern.size(),true));
				double sz = 1;
				for(unsigned int n = 0; n < _pattern.size(); ++n) {
					if(!_pattern[n]) {
						tablesize ts = _table->universe().tables()[n]->size();
						if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
							if(sz * ts._size < maxdouble) {
								sz = sz * ts._size;
							}
							else {
								_result = maxdouble;
								return;
							}
						}
						else {
							_result = maxdouble;
							return;
						}
					}
				}
				if(sz * lookuptime < maxdouble) _result = sz * lookuptime;
				else _result = maxdouble;
			}
		}

		void visit(const ProcInternalFuncTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double sz = 1;
			for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
				if(!_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if(sz * ts._size < maxdouble) {
							sz = sz * ts._size;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			// NOTE: We assume that evaluation of a lua function has cost 5
			// Can be adapted if this turns out to be unrealistic
			if(5 * sz < maxdouble) _result = 5 * sz;
			else _result = maxdouble;
		}

		void visit(const UNAInternalFuncTable* ) {
			if(_pattern.back()) {
				unsigned int patterncount = 0;
				for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
					if(_pattern[n]) ++patterncount;
				}
				_result = patterncount;
			}
			else {
				double maxdouble = numeric_limits<double>::max();
				double sz = 1;
				for(unsigned int n = 0; n < _pattern.size() - 1; ++n) {
					if(!_pattern[n]) {
						tablesize ts = _table->universe().tables()[n]->size();
						if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
							if(sz * ts._size < maxdouble) {
								sz = sz * ts._size;
							}
							else {
								_result = maxdouble;
								return;
							}
						}
						else {
							_result = maxdouble;
							return;
						}
					}
				}
				_result = sz;
			}
		}

		void visit(const EnumeratedInternalFuncTable* ) {
			double maxdouble = numeric_limits<double>::max();
			double inputunivsize = 1;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) {
					tablesize ts = _table->universe().tables()[n]->size();
					if(ts._type == TST_APPROXIMATED || ts._type == TST_EXACT) {
						if(inputunivsize * ts._size < maxdouble) {
							inputunivsize = inputunivsize * ts._size;
						}
						else {
							_result = maxdouble;
							return;
						}
					}
					else {
						_result = maxdouble;
						return;
					}
				}
			}
			tablesize ts = _table->size();
			double lookupsize = inputunivsize < ts._size ? inputunivsize : ts._size;
			if(log(lookupsize) / log(2) < maxdouble) {
				double lookuptime = log(lookupsize) / log(2);
				double iteratetime = ts._size / inputunivsize;
				if(lookuptime + iteratetime < maxdouble) _result = lookuptime + iteratetime;
				else _result = maxdouble;
			}
			else _result = maxdouble;
		}

		void visit(const PlusInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const MinusInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const TimesInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const DivInternalFuncTable* ) {
			unsigned int patterncount = 0;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_pattern[n]) ++patterncount;
			}
			if(patterncount >= 2) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const AbsInternalFuncTable* ) {
			if(_pattern[0]) _result = 1;
			else if(_pattern[1]) _result = 2;
			else _result = numeric_limits<double>::max();
		}

		void visit(const UminInternalFuncTable* ) {
			if(_pattern[0] || _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const ExpInternalFuncTable* ) {
			if(_pattern[0] && _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}

		void visit(const ModInternalFuncTable* ) {
			if(_pattern[0] && _pattern[1]) _result = 1;
			else _result = numeric_limits<double>::max();
		}
};

#endif /* TABLECOSTESTIMATOR_HPP_ */
