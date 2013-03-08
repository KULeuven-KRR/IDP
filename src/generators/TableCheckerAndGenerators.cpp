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

#include "TableCheckerAndGenerators.hpp"
#include "GeneratorFactory.hpp"

/**
 * A generator which checks whether a fully instantiated list of variables is a valid tuple for a certain predicate
 */
TableChecker::TableChecker(const PredTable* t, const std::vector<const DomElemContainer*>& vars, const Universe& univ)
		: 	_table(t),
			_vars(vars),
			_universe(univ),
			_reset(true),
			_currargs(vars.size(), NULL) {
	Assert(t->arity() == vars.size());
}

TableChecker* TableChecker::clone() const {
	return new TableChecker(*this);
}

void TableChecker::internalSetVarsAgain() {
	// NO-OP
}

void TableChecker::reset() {
	_reset = true;
}

void TableChecker::next() {
	if (_reset) {
		_reset = false;
		for(uint i=0; i<_vars.size(); ++i){
			_currargs[i] = (_vars[i])->get();
		}
		auto allowedvalue = (_table->contains(_currargs) && _universe.contains(_currargs));
		if (not allowedvalue) {
			notifyAtEnd();
		}
	} else {
		notifyAtEnd();
	}
}

void TableChecker::put(std::ostream& stream) const {
	stream << print(_table) << "(";
	bool begin = true;
	for (unsigned int n = 0; n < _vars.size(); ++n) {
		if (not begin) {
			stream << ", ";
		}
		begin = false;
		stream << _vars[n] << "(in)";
	}
	stream << ")";
}

TableGenerator::TableGenerator(const PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars,
		const std::vector<unsigned int>& firstocc, const Universe& univ)
		: 	_fulltable(table),
			_allvars(vars),
			_reset(true) {
	std::vector<SortTable*> outuniv;
	for (unsigned int n = 0; n < pattern.size(); ++n) {
		if (pattern[n] == Pattern::OUTPUT) {
			_outvars.push_back(vars[n]);
			_outvaroccurence.push_back(n);
			_uniqueoutvarindex.push_back(_outvars.size() - 1);
			if (firstocc[n] == n) {
				outuniv.push_back(univ.tables()[n]);
			}
		}
	}
	_outputtable = TableUtils::createFullPredTable(Universe(outuniv));
}

TableGenerator::~TableGenerator() {
	delete (_outputtable);
}

TableGenerator* TableGenerator::clone() const {
	auto t = new TableGenerator(*this);
	t->_outputtable = new PredTable(*_outputtable);
	return t;
}

void TableGenerator::reset() {
	_reset = true;
}

void TableGenerator::next() {
	if (_reset) {
		_reset = false;
		_currenttuple.clear();
		for (auto i = _allvars.begin(); i < _allvars.end(); ++i) {
			_currenttuple.push_back((*i)->get());
		}
		_current = _outputtable->begin();
	}else{
		++_current;
	}
	while (not _current.isAtEnd() && not inFullTable()) {
		++_current;
	}
	if (_current.isAtEnd()) {
		notifyAtEnd();
	}else{
		const auto& values = *_current;
		for(uint i=0; i<values.size(); ++i){
			*_outvars[i] = values[i];
		}
	}
}

bool TableGenerator::inFullTable() {
	const auto& values = *_current;
	for (unsigned int i = 0; i < values.size(); ++i) {
		_currenttuple[_outvaroccurence[i]] = values[_uniqueoutvarindex[i]];
	}
	return _fulltable->contains(_currenttuple);
}

InverseTableGenerator::InverseTableGenerator(PredTable* table, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>& vars)
		: _reset(true) {
	std::vector<const DomElemContainer*> outvars;
	std::vector<SortTable*> temptables;
	for (unsigned int i = 0; i < pattern.size(); ++i) {
		if (pattern[i] == Pattern::OUTPUT) {
			outvars.push_back(vars[i]);
			temptables.push_back(table->universe().tables()[i]);
		}
	}
	Universe universe(temptables);
	auto temp = TableUtils::createFullPredTable(universe);
	_universegen = GeneratorFactory::create(temp, std::vector<Pattern>(outvars.size(), Pattern::OUTPUT), outvars, universe);
	_predchecker = new TableChecker(table, vars, table->universe());
	// TODO deletion of temp
}

InverseTableGenerator* InverseTableGenerator::clone() const {
	auto gen = new InverseTableGenerator(*this);
	gen->_universegen = _universegen->clone();
	gen->_predchecker = _predchecker->clone();
	return gen;
}

void InverseTableGenerator::internalSetVarsAgain() {
	_universegen->setVarsAgain();
}

void InverseTableGenerator::reset() {
	_reset = true;
}

void InverseTableGenerator::next() {
	if (_reset) {
		_reset = false;
		_universegen->begin();
	} else {
		_universegen->operator ++();
	}

	for (; not _universegen->isAtEnd(); _universegen->operator ++()) {
		if (not _predchecker->check()) { // It is NOT a tuple in the table
			return;
		}
	}
	if (_universegen->isAtEnd()) {
		notifyAtEnd();
	}
}
void InverseTableGenerator::put(std::ostream& stream) const {
	stream << "Inverse instance generator: inverse of" << nt() << print(_predchecker);
}

