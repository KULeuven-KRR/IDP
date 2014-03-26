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

#include "CollectSymbolOccurences.hpp"

#include "IncludeComponents.hpp"

using namespace std;

void CollectSymbolOccurences::visit(const EqChainForm* f) {
	auto oldcontext = _context;
	if (f->sign() == SIGN::NEG) {
		_context = !_context;
	}
	traverse(f);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const EquivForm* f) {
	auto oldcontext = _context;
	_context = Context::BOTH;
	traverse(f);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const BoolForm* f) {
	auto oldcontext = _context;
	if (f->sign() == SIGN::NEG) {
		_context = !_context;
	}
	traverse(f);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const QuantForm* f) {
	auto oldcontext = _context;
	if (f->sign() == SIGN::NEG) {
		_context = !_context;
	}
	traverse(f);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const AggForm* f) {
	auto oldcontext = _context;
	_context = Context::BOTH;
	traverse(f);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const PredForm* f) {
	auto oldcontext = _context;
	if (f->sign() == SIGN::NEG) {
		_context = !_context;
	}
	_result.insert( { f->symbol(), _context });
	traverse (f);
	_context = oldcontext;
}
void CollectSymbolOccurences::visit(const FuncTerm* ft) {
	_result.insert( { ft->function(), _context });
	traverse(ft);
}
void CollectSymbolOccurences::visit(const Definition*d) {
	auto oldcontext = _context;
	_context = Context::BOTH;
	DefaultTraversingTheoryVisitor::visit(d);
	_context = oldcontext;
}

void CollectSymbolOccurences::visit(const Rule* r) {
	auto oldcontext = _context;
	_context = Context::BOTH;
	r->head()->accept(this);
	r->body()->accept(this);
	_context = oldcontext;
}
