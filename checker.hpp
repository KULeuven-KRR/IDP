#ifndef INSTCHECKER_H
#define INSTCHECKER_H

#include "structure.hpp"

class InstanceChecker {

	public:
		virtual bool run()	const = 0;
};

class FalseInstanceChecker : public InstanceChecker {
	
	public:
		bool run()	const { return false;	}
};

class TrueInstanceChecker : public InstanceChecker { 
	
	public:
		bool run()	const { return true;	}
};

class TableInstanceChecker : public InstanceChecker {

	private:
		PredTable*				_table;
		vector<TypedElement*>	_args;
	public:
		TableInstanceChecker(PredTable* t, const vector<TypedElement*>& a) : _table(t), _args(a) { }
		bool run()	const { return _table->contains(_args);	}
};

class InvTableInstanceChecker : public InstanceChecker {

	private:
		PredTable*				_table;
		vector<TypedElement*>	_args;
	public:
		InvTableInstanceChecker(PredTable* t, const vector<TypedElement*>& a) : _table(t), _args(a) { }
		bool run()	const { return !(_table->contains(_args));	}
};

#endif
