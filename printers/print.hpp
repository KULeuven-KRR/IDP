/************************************
	print.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <cstdio>
#include <sstream>
#include <ostream>
#include <string>
#include <vector>
#include <set>
#include <assert.h>

#include "theory.hpp" // for TheoryVisitor

class Options;
class Vocabulary;
class AbstractStructure;
class Namespace;
class Formula;
class AbstractTheory;
class InteractivePrintMonitor;
class GroundFixpDef;
class GroundDefinition;
class CPReification;
class GroundSet;
class GroundAggregate;
class GroundTermTranslator;
class GroundTranslator;
typedef std::vector<int> GroundClause;

int getIDForUndefined();

class Printer : public TheoryVisitor {
private:
	int	opendef_; 	//the id of the currenlty open definition
	bool theoryopen_;
	std::set<int> _pastopendefs;
protected:
	Printer(): opendef_(-1), theoryopen_(false){}

	bool isDefClosed() const { return opendef_ == -1; }
	bool isDefOpen(int defid) const { return opendef_==defid; }
	void closeDef() { opendef_ = -1; }
	void openDef(int defid) { opendef_ = defid; }

	bool isTheoryOpen() const { return theoryopen_; }
	void closeTheory() { theoryopen_ = false; }
	void openTheory() { theoryopen_ = true; }

public:
	// Factory method
	template<class Stream> static Printer* create(Options* opts, Stream& stream);
	template<class Stream> static Printer* create(Options* opts, Stream& stream, bool arithmetic);

	// Print methods
	void visit(const Formula*);
	void visit(const AbstractTheory*);

	virtual void visit(const Vocabulary*) = 0;
	virtual void visit(const AbstractStructure*) = 0;
	virtual void visit(const Namespace*) = 0;
	virtual void visit(const GroundClause& g) = 0;
	virtual void visit(const GroundFixpDef*) = 0;
	virtual void visit(const GroundSet*) = 0;
	virtual void visit(const PCGroundRule* b) = 0;
	virtual void visit(const AggGroundRule* b) = 0;
	virtual void visit(const GroundAggregate* cpr) = 0;
	virtual void visit(const CPReification* cpr) = 0;

	void checkOrOpen(int defid) {
		if(!isDefOpen(defid)){
			_pastopendefs.insert(opendef_);
			assert(_pastopendefs.find(defid)==_pastopendefs.end());
		}
	}

	virtual void startTheory() = 0;
	virtual void endTheory() = 0;

	virtual void setTranslator(GroundTranslator*){}
	virtual void setTermTranslator(GroundTermTranslator*){}
	virtual void setStructure(AbstractStructure*){}
};

template<typename Stream>
class StreamPrinter : public Printer {
private:
	Stream& _out;
	unsigned int indentation_;

protected:
	StreamPrinter(Stream& stream):
		_out(stream), indentation_(0){} //default indentation = 0

	Stream& output(){ return _out; }

	// Indentation
	void indent() 				{ indentation_++; }
	void unindent()				{ indentation_--; }
	unsigned int getIndentation() const 	{ return indentation_; }
	void printtab() {
		for(unsigned int n = 0; n < getIndentation(); ++n){
			output() << "  ";
		}
	}
};

#endif /* PRINT_HPP_ */
