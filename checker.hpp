#ifndef INSTCHECKER_H
#define INSTCHECKER_H

#include "structure.hpp"

class InstanceChecker {

	public:
		virtual bool run(const vector<domelement>&)	const = 0;
};

class FalseInstanceChecker : public InstanceChecker {
	
	public:
		bool run(const vector<domelement>&)	const { return false;	}
};

class TrueInstanceChecker : public InstanceChecker { 
	
	public:
		bool run(const vector<domelement>&)	const { return true;	}
};

class TableInstanceChecker : public InstanceChecker {

	private:
		PredTable*				_table;
	public:
		TableInstanceChecker(PredTable* t) : _table(t) { }
		bool run(const vector<domelement>& vd)	const { return _table->contains(vd);	}
};

class InvTableInstanceChecker : public InstanceChecker {

	private:
		PredTable*				_table;
	public:
		InvTableInstanceChecker(PredTable* t) : _table(t) { }
		bool run(const vector<domelement>& vd)	const { return !(_table->contains(vd));	}
};

class CheckerFactory {
	
	public:
		InstanceChecker*	create(PredInter*, bool ctpf, bool c);
};

#endif
