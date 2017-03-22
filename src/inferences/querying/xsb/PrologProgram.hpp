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

#include <list>
#include <set>
#include "common.hpp"

class Structure;
class Definition;
class PFSymbol;
class Function;
class Sort;
class PrologClause;
class XSBToIDPTranslator;

using std::string;
using std::ostream;
using std::set;
using std::list;

class PrologProgram {
	friend ostream& operator<<(ostream& output, const PrologProgram& pp);
private:
	list<PrologClause*> _clauses;
	set<PFSymbol*> _tabled;
	set<Sort*> _sorts;
	Structure* _structure;
	Definition* _definition;
	set<string> _loaded;
	set<string> _all_predicates;
	XSBToIDPTranslator* _translator;
	void printSort(const Sort*, std::ostream&);
	void printOpenSymbol(PFSymbol*, std::ostream&);
	void printAsFacts(std::string, PFSymbol*, std::ostream&);
	void print3valFacts(std::string, PFSymbol*, std::ostream&);
	void print2valFacts(std::string, PFSymbol*, std::ostream&);
	void printTuple(const ElementTuple&, std::ostream&);
	void printDomainElement(const DomainElement*, std::ostream&);
	void printConstructedTypesRules(const Sort*, std::ostream&);
	void printConstructorRules(const Sort*, Function*, std::ostream&);

public:
	PrologProgram(Structure* structure, XSBToIDPTranslator* translator)
			: 	_structure(structure),
				_definition(NULL),
			  	_translator(translator) {
	}
	~PrologProgram();
	
	static std::string getCompilerCode();
	
	void table(PFSymbol*);
	void addClause(PrologClause* pc) {
		_clauses.push_back(pc);
	}
	string getCode();
	string getFacts();
	string getRanges();

	void setDefinition(Definition* d);
	bool isTabling(PFSymbol* p) {
		return _tabled.find(p) != _tabled.end();
	}
	void addClauses(list<PrologClause*> pcs) {
		for (list<PrologClause*>::iterator it = pcs.begin(); it != pcs.end(); ++it) {
			_clauses.push_back(*it);
		}
	}
	list<PrologClause*> clauses() {
		return _clauses;
	}
	Structure* structure() {
		return _structure;
	}
	const set<string>& allPredicates() const {
		return _all_predicates;
	}
	void addDomainDeclarationFor(Sort*);
};

class PrologTerm;

class PrologClause {
	friend ostream& operator<<(ostream& output, const PrologClause& pc);
private:
	PrologTerm* _head;
	list<PrologTerm*> _body;
	void reorder();
public:
	PrologClause(PrologTerm* head, list<PrologTerm*> body, bool reordering = true)
			: 	_head(head),
				_body(body) {
		if (reordering) {
			reorder();

		}
	}
	PrologClause(PrologTerm* head)
			: 	_head(head),
				_body() {

	}
	PrologClause(PrologTerm* head, PrologTerm* body)
			: 	_head(head),
				_body() {
		_body.push_back(body);
	}

	const PrologTerm* head() const {
		return _head;
	}

	const list<PrologTerm*>& body() const {
		return _body;
	}
};

