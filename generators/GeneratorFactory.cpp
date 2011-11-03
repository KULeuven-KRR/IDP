/************************************
 generator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "common.hpp"
#include "parseinfo.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "checker.hpp"
#include "fobdd.hpp"
#include "term.hpp"
#include "theory.hpp"

#include "generators/GeneratorFactory.hpp"

#include "generators/BDDBasedGeneratorFactory.hpp"

#include "generators/SimpleFuncGenerator.hpp"
#include "generators/TreeInstGenerator.hpp"
#include "generators/InverseInstGenerator.hpp"
#include "generators/SortInstGenerator.hpp"
#include "generators/EnumLookupGenerator.hpp"
#include "generators/SortLookupGenerator.hpp"
#include "generators/TableGenerator.hpp"
#include "generators/ComparisonGenerator.hpp"
#include "generators/SimpleFuncGenerator.hpp"
#include "generators/EmptyGenerator.hpp"
#include "generators/ArithmeticOperatorsGenerator.hpp"
#include "generators/InverseUnaFunctionGenerator.hpp"
#include "generators/InvertNumericGenerator.hpp"
#include "generators/InverseAbsValueGenerator.hpp"
using namespace std;

InstGenerator* GeneratorFactory::create(const vector<const DomElemContainer*>& vars, const vector<SortTable*>& tabs) {
	InstGenerator* gen = NULL;
	GeneratorNode* node = NULL;
	auto jt = tabs.crbegin();
	for (auto it = vars.crbegin(); it != vars.crend(); ++it, ++jt) {
		auto tig = new SortInstGenerator((*jt)->internTable(), *it);
		if (vars.size() == 1) {
			gen = tig;
			break;
		} else if (it == vars.rbegin()) {
			node = new LeafGeneratorNode(tig);
		} else {
			node = new OneChildGeneratorNode(tig, node);
		}
	}
	if (gen == NULL) {
		gen = new TreeInstGenerator(node);
	}
	return gen;
}

InstGenerator* GeneratorFactory::create(const PredTable* pt, vector<Pattern> pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe) {
	GeneratorFactory factory;
	return factory.internalCreate(pt, pattern, vars, universe);
}

InstGenerator* GeneratorFactory::internalCreate(const PredTable* pt, vector<Pattern> pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe) {
	_table = pt;
	_pattern = pattern;
	_vars = vars;
	_universe = universe;
	for (unsigned int n = 0; n < _vars.size(); ++n) {
		_firstocc.push_back(n);
		for (unsigned int m = 0; m < n; ++m) {
			if (_vars[n] == _vars[m]) {
				_firstocc[n] = m;
				break;
			}
		}
	}

	unsigned int firstout = 0;
	for (; firstout < pattern.size(); ++firstout) {
		if (not pattern[firstout]) {
			break;
		}
	}
	if (firstout == pattern.size()) { // no output variables
		if (typeid(*(pt->internTable())) != typeid(BDDInternalPredTable)) {
			return new LookupGenerator(pt, vars, _universe);
		} else {
			StructureVisitor::visit(pt);
			return _generator;
		}
	} else {
		StructureVisitor::visit(pt);
		return _generator;
	}
}

void GeneratorFactory::visit(const ProcInternalPredTable*) {
	_generator = new TableGenerator(_table, _pattern, _vars, _firstocc, _universe);
}

void GeneratorFactory::visit(const BDDInternalPredTable* table) {

	// Add necessary types to the bdd to ensure, if possible, finite querying
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(table->bdd(), table->manager());
	// TODO

	// Optimize the bdd for querying
	set<const FOBDDVariable*> outvars;
	vector<const FOBDDVariable*> allvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		const FOBDDVariable* var = optimizemanager.getVariable(table->vars()[n]);
		allvars.push_back(var);
		if (!_pattern[n])
			outvars.insert(var);
	}
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizequery(copybdd, outvars, indices, table->structure());

	// Generate a generator for the optimized bdd
	BDDToGenerator btg(&optimizemanager);
	_generator = btg.create(copybdd, _pattern, _vars, allvars, table->structure(), _universe);
}

void GeneratorFactory::visit(const FullInternalPredTable*) {
	vector<const DomElemContainer*> outvars;
	vector<SortTable*> outtables;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if ((!_pattern[n]) && _firstocc[n] == n) {
			outvars.push_back(_vars[n]);
			outtables.push_back(_universe.tables()[n]);
		}
	}
	_generator = create(outvars, outtables);
}

void GeneratorFactory::visit(const FuncInternalPredTable* fipt) {
	visit(fipt->table());
}

void GeneratorFactory::visit(const UnionInternalPredTable*) {
	assert(false);
	// TODO
}

void GeneratorFactory::visit(const UnionInternalSortTable*) {
	assert(false);
	// TODO
}

void GeneratorFactory::visit(const AllNaturalNumbers* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const AllIntegers* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const AllFloats* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const AllChars* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const AllStrings* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const EnumeratedInternalSortTable* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}
void GeneratorFactory::visit(const IntRangeInternalSortTable* t) {
	if (_pattern[0])
		_generator = new SortLookUpGenerator(t, _vars[0]);
	else
		_generator = new SortInstGenerator(t, _vars[0]);
}

void GeneratorFactory::visit(const EnumeratedInternalPredTable*) {
	// TODO: Use dynamic programming to improve this
	LookupTable* lpt = new LookupTable();
	LookupTable& lookuptab = *lpt;
	vector<const DomElemContainer*> invars;
	vector<const DomElemContainer*> outvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_firstocc[n] == n) {
			if (_pattern[n])
				invars.push_back(_vars[n]);
			else
				outvars.push_back(_vars[n]);
		}
	}
	for (TableIterator it = _table->begin(); not it.isAtEnd(); ++it) {
		const ElementTuple& tuple = *it;
		bool ok = true;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				ok = false;
				break;
			}
		}
		if (ok) {
			ElementTuple intuple;
			ElementTuple outtuple;
			for (unsigned int n = 0; n < _pattern.size(); ++n) {
				if (_firstocc[n] == n) {
					if (_pattern[n]) {
						intuple.push_back(tuple[n]);
					} else if (_firstocc[n] == n) {
						outtuple.push_back(tuple[n]);
					}
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab, invars, outvars);
}

Input getBinaryPattern(Pattern left, Pattern right) {
	Input input = Input::BOTH;
	if (left == Pattern::OUTPUT && right == Pattern::OUTPUT) {
		input = Input::NONE;
	} else if (left == Pattern::INPUT && right == Pattern::OUTPUT) {
		input = Input::LEFT;
	} else if (left == Pattern::OUTPUT && right == Pattern::INPUT) {
		input = Input::RIGHT;
	}
	return input;
}

void GeneratorFactory::visit(const EqualInternalPredTable*) {
	if (_firstocc[1] == 0) {
		_generator = create(vector<const DomElemContainer*>(1, _vars[0]), vector<SortTable*>(1, _universe.tables()[0]));
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::EQ);
}

void GeneratorFactory::visit(const StrLessInternalPredTable*) {
	if (_firstocc[1] == 0) { // If both variables are the same, this will never hold any tuples. TODO can we check equality of variables (or should we be able to anyway?)?
		_generator = new EmptyGenerator();
		return;
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::LT);
}

void GeneratorFactory::visit(const StrGreaterInternalPredTable*) {
	if (_firstocc[1] == 0) {
		_generator = new EmptyGenerator();
		return;
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::GT);
}

void GeneratorFactory::visit(const InverseInternalPredTable* iip) {
	InternalPredTable* interntable = iip->table();
	if (typeid(*interntable) == typeid(InverseInternalPredTable)) {
		dynamic_cast<InverseInternalPredTable*>(interntable)->table()->accept(this);
	} else if (typeid(*interntable) == typeid(BDDInternalPredTable)) {
		BDDInternalPredTable* bddintern = dynamic_cast<BDDInternalPredTable*>(interntable);
		const FOBDD* invertedbdd = bddintern->manager()->negation(bddintern->bdd());
		BDDInternalPredTable* invertedbddtable = new BDDInternalPredTable(invertedbdd, bddintern->manager(), bddintern->vars(),
				bddintern->structure());
		visit(invertedbddtable);
	} else if (typeid(*interntable) == typeid(FullInternalPredTable)) {
		_generator = new EmptyGenerator();
	} else {
		PredTable* temp = new PredTable(iip->table(), _universe);
		_generator = new InverseInstGenerator(temp, _pattern, _vars);
	}
}

// NOTE: for any function, if the range is an output variable, we can use the simple func generator
// if the range is input, we need more specialized generators depending on the function type
void GeneratorFactory::visit(const FuncTable* ft) {
	if (not _pattern.back()) {
		// TODO: for the input positions, change universe to the universe of ft if this is smaller
		_generator = new SimpleFuncGenerator(ft, _pattern, _vars, _universe, _firstocc);
	} else
		ft->internTable()->accept(this);
}

void GeneratorFactory::visit(const ProcInternalFuncTable*) {
	_generator = new TableGenerator(_table, _pattern, _vars, _firstocc, _universe);
}

//
void GeneratorFactory::visit(const UNAInternalFuncTable*) {
	_generator = new InverseUNAFuncGenerator(_pattern, _vars, _universe);
}

void GeneratorFactory::visit(const EnumeratedInternalFuncTable*) {
	// TODO: Use dynamic programming to improve this
	LookupTable lookuptab;
	vector<const DomElemContainer*> invars;
	vector<const DomElemContainer*> outvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_firstocc[n] == n) {
			if (_pattern[n])
				invars.push_back(_vars[n]);
			else
				outvars.push_back(_vars[n]);
		}
	}
	for (TableIterator it = _table->begin(); not it.isAtEnd(); ++it) {
		const ElementTuple& tuple = *it;
		bool ok = true;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				ok = false;
				break;
			}
		}
		if (ok) {
			ElementTuple intuple;
			ElementTuple outtuple;
			for (unsigned int n = 0; n < _pattern.size(); ++n) {
				if (_firstocc[n] == n) {
					if (_pattern[n])
						intuple.push_back(tuple[n]);
					else
						outtuple.push_back(tuple[n]);
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab, invars, outvars);
}

void GeneratorFactory::visit(const PlusInternalFuncTable* pift) {
	if (_pattern[0]) {
		_generator = new MinusGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
	} else if (_pattern[1]) {
		_generator = new MinusGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		if (pift->getType() == NumType::INT) {
			assert(false);
			// TODO
		} else {
			const DomainElement* two = DomainElementFactory::instance()->create(2);
			const DomElemContainer* twopointer = new const DomElemContainer();
			*twopointer = two;
			_generator = new DivGenerator(_vars[2], twopointer, _vars[0], NumType::DOUBLE, _universe.tables()[0]);
		}
	} else {
		notyetimplemented("Infinite generator for addition pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const MinusInternalFuncTable* pift) {
	if (_pattern[0]) {
		_generator = new MinusGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
	} else if (_pattern[1]) {
		_generator = new PlusGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		assert(false);
		// TODO
	} else {
		notyetimplemented("Infinite generator for subtraction pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const TimesInternalFuncTable* pift) {
	if (_pattern[0]) {
		_generator = new DivGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
	} else if (_pattern[1]) {
		_generator = new DivGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		assert(false);
		// TODO
	} else {
		notyetimplemented("Infinite generator for multiplication pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const DivInternalFuncTable* pift) {
	if (_pattern[0]) {
		_generator = new DivGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
	} else if (_pattern[1]) {
		// TODO: wrong in case of integers. E.g., a / 2 = 1 should result in a \in { 2,3 } instead of a \in { 2 }
		_generator = new TimesGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		assert(false);
		// TODO
	} else {
		notyetimplemented("Infinite generator for division pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const ExpInternalFuncTable*) {
	notyetimplemented("Infinite generator for exponentiation pattern (?,?,in)");
	exit(1);
}

void GeneratorFactory::visit(const ModInternalFuncTable*) {
	notyetimplemented("Infinite generator for remainder pattern (?,?,in)");
	exit(1);
}

void GeneratorFactory::visit(const AbsInternalFuncTable* aift) {
	assert(!_pattern[0]);
	_generator = new InverseAbsValueGenerator(_vars[1], _vars[0], _universe.tables()[0], aift->getType());
}

void GeneratorFactory::visit(const UminInternalFuncTable* uift) {
	assert(!_pattern[0]);
	_generator = new InvertNumericGenerator(_vars[1], _vars[0], _universe.tables()[0], uift->getType());
}

