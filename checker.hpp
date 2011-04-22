/************************************
	checker.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSTCHECKER_HPP
#define INSTCHECKER_HPP

#include <vector>

class PredTable;
class PredInter;
class DomainElement;

class InstanceChecker {
	public:
		virtual bool run(const std::vector<const DomainElement*>&)	const = 0;
};

class FalseInstanceChecker : public InstanceChecker {
	public:
		bool run(const std::vector<const DomainElement*>&)	const { return false;	}
};

class TrueInstanceChecker : public InstanceChecker { 
	public:
		bool run(const std::vector<const DomainElement*>&)	const { return true;	}
};

class TableInstanceChecker : public InstanceChecker {
	private:
		PredTable*	_table;
	public:
		TableInstanceChecker(PredTable* t) : _table(t) { }
		bool run(const std::vector<const DomainElement*>& vd)	const;
};

class CheckerFactory {
	public:
		InstanceChecker*	create(PredInter*, bool ctpf, bool c);
};

#endif
