/************************************
	checker.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSTCHECKER_HPP
#define INSTCHECKER_HPP

#include "structure.hpp"

class PredTable;
class PredInter;

/**
 * Class, associated with a relationship, which given a ElementTuple of domain elements, checks whether it is in the interpretation of that relationship
 */
class InstanceChecker {
	public:
		virtual bool isInInterpretation(const ElementTuple&) const = 0;
		virtual ~InstanceChecker(){}
};

class FalseInstanceChecker : public InstanceChecker {
	public:
		bool isInInterpretation(const ElementTuple&) const { return false;	}
};

class TrueInstanceChecker : public InstanceChecker { 
	public:
		bool isInInterpretation(const ElementTuple&) const { return true;	}
};

class TableInstanceChecker : public InstanceChecker {
	private:
		const PredTable* _table;
	public:
		TableInstanceChecker(const PredTable* t) : _table(t) { }
		bool isInInterpretation(const ElementTuple& vd) const;
};

class CheckerFactory {
	public:
		InstanceChecker* create(PredInter*, TruthType type);
};

#endif
