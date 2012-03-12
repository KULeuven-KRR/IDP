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
#include "parseinfo.hpp"
#include "errorhandling/error.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"

#include "GeneratorFactory.hpp"

#include "BDDBasedGeneratorFactory.hpp"

#include "SimpleFuncGenerator.hpp"
#include "TreeInstGenerator.hpp"
#include "InverseInstGenerator.hpp"
#include "SortInstGenerator.hpp"
#include "EnumLookupGenerator.hpp"
#include "SortLookupGenerator.hpp"
#include "TableGenerator.hpp"
#include "ComparisonGenerator.hpp"
#include "SimpleFuncGenerator.hpp"
#include "BasicGenerators.hpp"
#include "ArithmeticOperatorsGenerator.hpp"
#include "ArithmeticOperatorsChecker.hpp"
#include "InverseUnaFunctionGenerator.hpp"
#include "InvertNumericGenerator.hpp"
#include "InverseAbsValueGenerator.hpp"
#include "AbsValueChecker.hpp"
using namespace std;

// NOTE original can be NULL
template<typename Table>
bool isCertainlyFinite(Table t) {
	return t->finite();
}

InstGenerator* GeneratorFactory::create(const vector<const DomElemContainer*>& vars, const vector<SortTable*>& tabs, const Formula*) {
	Assert(vars.size()==tabs.size());
	if (vars.size() == 0) {
		return new FullGenerator(); // TODO check if this is always correct?
	}
	InstGenerator* gen = NULL;
	GeneratorNode* node = NULL;
	auto jt = tabs.crbegin();
	bool certainlyfinite = true;
	for (auto it = vars.crbegin(); it != vars.crend(); ++it, ++jt) {
		if (not isCertainlyFinite((*jt)->internTable())) {
			certainlyfinite = false;
		}
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
	if (not certainlyfinite) {
		gen->notifyIsInfiniteGenerator();
	}
	return gen;
}

// NOTE: becomes predtable owner!
InstGenerator* GeneratorFactory::create(const PredTable* pt, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe, const Formula*) {
	GeneratorFactory factory;

	// Check for infinite grounding
	bool certainlyfinite = true;
	for (size_t i = 0; i < universe.tables().size() && certainlyfinite; ++i) {
		if (pattern[i] == Pattern::OUTPUT) {
			if (not isCertainlyFinite(universe.tables()[i])) {
				certainlyfinite = false;
			}
		}
	}
	auto gen = factory.internalCreate(pt, pattern, vars, universe);
	if (not certainlyfinite) {
		gen->notifyIsInfiniteGenerator();
	}
	return gen;
}

InstGenerator* GeneratorFactory::create(const PredForm* atom, const AbstractStructure* structure, bool inverse, const vector<Pattern>& pattern,
		const vector<const DomElemContainer*>& vars, const Universe& universe) {
	PFSymbol* symbol = atom->symbol();
	const PredTable* table = NULL;
	if (sametypeid<Predicate>(*(atom->symbol()))) {
		auto predicate = dynamic_cast<Predicate*>(atom->symbol());
		auto inter = structure->inter(predicate);
		switch (predicate->type()) {
		case ST_NONE:
			table = inverse ? inter->cf() : inter->ct();
			break;
		case ST_CT:
			table = inverse ? inter->pf() : inter->ct();
			break;
		case ST_CF:
			table = inverse ? inter->pt() : inter->cf();
			break;
		case ST_PT:
			table = inverse ? inter->cf() : inter->pt();
			break;
		case ST_PF:
			table = inverse ? inter->ct() : inter->pf();
			break;
		}
	} else {
		Assert(sametypeid<Function>(*(atom->symbol())));
		auto inter = structure->inter(dynamic_cast<Function*>(symbol))->graphInter();
		table = inverse ? inter->cf() : inter->ct();
	}
	return GeneratorFactory::create(table, pattern, vars, universe, atom);
}

// NOTE: becomes predtable owner!
InstGenerator* GeneratorFactory::internalCreate(const PredTable* pt, vector<Pattern> pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe) {
	Assert(pt->arity()==pattern.size());
	Assert(pattern.size()==vars.size());
	Assert(pattern.size()==universe.tables().size());

	_table = pt;
	_pattern = pattern;
	_vars = vars;
	_universe = universe;

	for (size_t n = 0; n < _vars.size(); ++n) {
		_firstocc.push_back(n);
		for (size_t m = 0; m < n; ++m) {
			if (_vars[n] == _vars[m]) {
				_firstocc[n] = m;
				break;
			}
		}
	}

	size_t firstout = 0;
	for (; firstout < pattern.size(); ++firstout) {
		if (pattern[firstout] == Pattern::OUTPUT) {
			break;
		}
	}
	/*if (firstout == pattern.size()) { // no output variables
	 if (sametypeid<BDDInternalPredTable>(*(pt->internTable()))) {
	 return new LookupGenerator(pt, vars, _universe);
	 } else {
	 StructureVisitor::visit(pt);
	 return _generator;
	 }
	 } else {*/
	StructureVisitor::visit(pt);
	return _generator;
	//}
}

void GeneratorFactory::visit(const ProcInternalPredTable*) {
	_generator = new TableGenerator(_table, _pattern, _vars, _firstocc, _universe);
}

void GeneratorFactory::visit(const BDDInternalPredTable* table) {
	BddGeneratorData data;
	data.pattern = _pattern;
	data.vars = _vars;
	data.structure = table->structure();
	data.universe = _universe;

	// Add necessary types to the bdd to ensure, if possible, finite querying
	FOBDDManager optimizemanager;
	data.bdd = optimizemanager.getBDD(table->bdd(), table->manager());

	// Optimize the bdd for querying
	set<const FOBDDVariable*> outvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		const FOBDDVariable* var = optimizemanager.getVariable(table->vars()[n]);
		data.bddvars.push_back(var);
		if (_pattern[n] == Pattern::OUTPUT) {
			outvars.insert(var);
		}
	}

	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizeQuery(data.bdd, outvars, indices, table->structure());

	// Generate a generator for the optimized bdd
	BDDToGenerator btg(&optimizemanager);

	_generator = btg.create(data);
}

void GeneratorFactory::visit(const FullInternalPredTable*) {
	vector<const DomElemContainer*> outvars;
	vector<SortTable*> outtables;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_pattern[n] == Pattern::OUTPUT && _firstocc[n] == n) {
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
	throw notyetimplemented("Create a generator from a union pred table");
}

void GeneratorFactory::visit(const UnionInternalSortTable*) {
	throw notyetimplemented("Create a generator from a union sort table");
}

void GeneratorFactory::visit(const AllNaturalNumbers* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllIntegers* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllFloats* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllChars* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllStrings* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const EnumeratedInternalSortTable* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const IntRangeInternalSortTable* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortLookUpGenerator(t, _vars[0]);
	} else {
		_generator = new SortInstGenerator(t, _vars[0]);
	}
}

void GeneratorFactory::visit(const EnumeratedInternalPredTable*) {
	LookupTable lookuptab;
	vector<const DomElemContainer*> invars, outvars;

	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_firstocc[n] != n) {
			continue;
		}
		if (_pattern[n] == Pattern::INPUT) {
			invars.push_back(_vars[n]);
		} else {
			outvars.push_back(_vars[n]);
		}
	}

	// TODO make this cheaper by adding domelem to index mappings
	// TODO only create it ONCE! instead of multiple times

	lookuptab.reserve(_table->size()._size);
	for (auto it = _table->begin(); not it.isAtEnd(); ++it) {
		CHECKTERMINATION

		auto tuple = *it;
		bool validunderocc = true;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			// Skip tuples which do no have the same values for multiple occurrences of some variable
			// TODO: Use dynamic programming to improve this
			if (_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				validunderocc = false;
				break;
			}
		}
		if (not validunderocc) {
			continue;
		}
		ElementTuple intuple, outtuple;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_firstocc[n] != n) {
				continue;
			}
			if (_pattern[n] == Pattern::INPUT) {
				intuple.push_back(tuple[n]);
			} else if (_firstocc[n] == n) {
				outtuple.push_back(tuple[n]);
			}
		}
		lookuptab[intuple].push_back(outtuple);
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
	if (_firstocc[1] == 0) { // If both variables are the same, this will never hold any tuples.
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
	if (_pattern.back() == Pattern::OUTPUT) {
		// TODO: for the input positions, change universe to the universe of ft if this is smaller
		//TODO: and vice versa: if the other is smaller, we can also make the universe of ft smaller!
		_generator = new SimpleFuncGenerator(ft, _pattern, _vars, _universe, _firstocc);
	} else {
		ft->internTable()->accept(this);
	}
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
			if (_pattern[n] == Pattern::INPUT) {
				invars.push_back(_vars[n]);
			} else {
				outvars.push_back(_vars[n]);
			}
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
					if (_pattern[n] == Pattern::INPUT) {
						intuple.push_back(tuple[n]);
					} else {
						outtuple.push_back(tuple[n]);
					}
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab, invars, outvars);
}

void GeneratorFactory::visit(const PlusInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new PlusChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new MinusGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new MinusGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		// if (pift->getType() == NumType::CERTAINLYINT) { // TODO (optimize code)
		const DomainElement* two = createDomElem(2);
		const DomElemContainer* twopointer = new const DomElemContainer();
		*twopointer = two;
		_generator = new DivGenerator(_vars[2], twopointer, _vars[0], NumType::POSSIBLYINT, _universe.tables()[0]);
		//}
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortInstGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new MinusGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(xgen, new LeafGeneratorNode(ygen)));
		} else if (_universe.tables()[1]->approxFinite()) {
			auto ygen = new SortInstGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new MinusGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(ygen, new LeafGeneratorNode(xgen)));
		} else {
			throw notyetimplemented("Infinite generator for addition pattern (out,out,in)");
		}
	}
}

void GeneratorFactory::visit(const MinusInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new MinusChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new MinusGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new PlusGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x-x=y, with x output");
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortInstGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new MinusGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(xgen, new LeafGeneratorNode(ygen)));
		} else if (_universe.tables()[1]->approxFinite()) {
			auto ygen = new SortInstGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new PlusGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(ygen, new LeafGeneratorNode(xgen)));
		} else {
			throw notyetimplemented("Infinite generator for subtraction pattern (out,out,in)");
		}
	}
}

void GeneratorFactory::visit(const TimesInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new TimesChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new DivGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new DivGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x*x=y, with x output");
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortInstGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new DivGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(xgen, new LeafGeneratorNode(ygen)));
		} else if (_universe.tables()[1]->approxFinite()) {
			auto ygen = new SortInstGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new DivGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(ygen, new LeafGeneratorNode(xgen)));
		} else {
			throw notyetimplemented("Infinite generator for multiplication pattern (out,out,in)");
		}
	}
}

void GeneratorFactory::visit(const DivInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new DivChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new DivGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new TimesGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x/x=y, with x output");
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortInstGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new DivGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(xgen, new LeafGeneratorNode(ygen)));
		} else if (_universe.tables()[1]->approxFinite()) {
			auto ygen = new SortInstGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new TimesGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new TreeInstGenerator(new OneChildGeneratorNode(ygen, new LeafGeneratorNode(xgen)));
		} else {
			throw notyetimplemented("Infinite generator for division pattern (out,out,in)");
		}
	}
}

void GeneratorFactory::visit(const ExpInternalFuncTable*) {
	throw notyetimplemented("Infinite generator for exponentiation pattern (?,?,in)");
}

void GeneratorFactory::visit(const ModInternalFuncTable*) {
	throw notyetimplemented("Infinite generator for remainder pattern (?,?,in)");
}

void GeneratorFactory::visit(const AbsInternalFuncTable* aift) {
	if(_pattern[0]==Pattern::OUTPUT){
		_generator = new InverseAbsValueGenerator(_vars[1], _vars[0], _universe.tables()[0], aift->getType());
	}
	else{
		_generator = new AbsValueChecker(_vars[0], _vars[1], _universe);
	}
}

void GeneratorFactory::visit(const UminInternalFuncTable* uift) {
	Assert(_pattern[0]==Pattern::OUTPUT);
	_generator = new InvertNumericGenerator(_vars[1], _vars[0], _universe.tables()[0], uift->getType());
}

