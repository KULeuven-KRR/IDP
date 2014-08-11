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

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <cstdio>
#include <sstream>
#include <ostream>
#include <string>
#include <vector>
#include <set>
#include <assert.h>

#include "visitors/TheoryVisitor.hpp"
#include "theory/ecnf.hpp"

class Options;
class Vocabulary;
class Structure;
class Namespace;
class Formula;
class AbstractTheory;
class InteractivePrintMonitor;
class Query;
class GroundFixpDef;
class GroundDefinition;
class CPReification;
class GroundSet;
class GroundAggregate;
class GroundTranslator;
class PredTable;
class FOBDD;

// NOTE: open and close theory have to be called externally, to guarantee the printer that it is closed correctly (and not reopened too soon)
class Printer: public TheoryVisitor {
	VISITORFRIENDS()
private:
	DefId opendef_; //the id of the currently open definition
	bool theoryopen_;
	std::set<DefId> _pastopendefs;
protected:
	Printer()
			: opendef_(-1), theoryopen_(false) {
	}

	bool isDefClosed() const {
		return opendef_ == -1;
	}
	bool isDefOpen(DefId defid) const {
		return opendef_ == defid;
	}
	void closeDef() {
		opendef_ = -1;
	}
	void openDef(DefId defid) {
		opendef_ = defid;
	}

	bool isTheoryOpen() const {
		return theoryopen_;
	}
	virtual void closeTheory() {
		theoryopen_ = false;
	}
	void openTheory() {
		theoryopen_ = true;
	}

	void visit(const Formula*);
	void visit(const AbstractTheory*);

	virtual void visit(const Vocabulary*) = 0;
	virtual void visit(const Structure*) = 0;
	virtual void visit(const Query*) = 0;
	virtual void visit(const FOBDD*) = 0;
	virtual void visit(const Compound*) = 0;
	virtual void visit(const Namespace*) = 0;
	virtual void visit(const UserProcedure*) = 0;
	virtual void visit(const GroundClause&) = 0;
	virtual void visit(const PredTable*) = 0;
	virtual void visit(const GroundFixpDef*) = 0;
	virtual void visit(const GroundSet*) = 0;
	virtual void visit(const PCGroundRule*) = 0;
	virtual void visit(const AggGroundRule*) = 0;
	virtual void visit(const GroundAggregate*) = 0;
	virtual void visit(const CPReification*) = 0;
	virtual void visit(const GroundEquivalence& geq){
		// no support for equalities yet, so we convert to clauses
		std::vector<GroundClause> clauses;
		geq.getClauses(clauses);
		for(auto cl:clauses){
			visit(cl);
		}
	}

public:
	virtual ~Printer(){}

	// Factory method
	template<class Stream> static Printer* create(Stream& stream);

	virtual void checkOrOpen(DefId defid) {
		if (!isDefOpen(defid)) {
			_pastopendefs.insert(opendef_);
			Assert(_pastopendefs.find(defid)==_pastopendefs.cend());
			openDef(defid);
		}
	}

	virtual void startTheory() = 0;
	virtual void endTheory() = 0;

	virtual void setTranslator(GroundTranslator*) {
	}
	virtual void setStructure(Structure*) {
	}

	template<typename T>
	void print(const T* t) {
		t->accept(this);
	}
	void print(const PredTable* t){
		visit(t);
	}
	void print(const GroundClause& clause) {
		visit(clause);
	}
	void print(const GroundEquivalence& geq){
		visit(geq);
	}
};

// FIXME: these should get normal accept methods (but NOT THEORYvisitors!)
template<> void Printer::print(const Structure* b);
template<> void Printer::print(const Namespace* b);
template<> void Printer::print(const Vocabulary* b);
template<> void Printer::print(const Query* b);
template<> void Printer::print(const UserProcedure* b);
template<> void Printer::print(const FOBDD* b);
template<> void Printer::print(const Compound* b);

template<typename Stream>
class StreamPrinter: public Printer {
	VISITORFRIENDS()
private:
	Stream& _out;
	unsigned int indentation_;

protected:
	StreamPrinter(Stream& stream)
			: _out(stream), indentation_(0) {
	} //default indentation = 0

	Stream& output() {
		return _out;
	}

	// Indentation
	void indent() {
		indentation_++;
	}
	void unindent() {
		indentation_--;
	}
	unsigned int getIndentation() const {
		return indentation_;
	}
	void printTab() {
		for (unsigned int n = 0; n < getIndentation(); ++n) {
			output() << "  ";
		}
	}

	virtual void closeTheory() {
		_out.flush();
		Printer::closeTheory();
	}
};

#endif /* PRINT_HPP_ */
