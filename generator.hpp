#ifndef INSTGENERATOR_H
#define INSTGENERATOR_H

#include "structure.hpp"

class InstGenerator {
	private:
		vector<TypedElement*>	_invalues;
		vector<TypedElement*>	_outvalues;
	public:
		InstGenerator() { }
		virtual bool first() = 0;
		virtual bool next() = 0; 
};

class TableInstGenerator : public InstGenerator { 
	private:
		PredTable*	_table;
	public:
		TableInstGenerator(PredTable* t) : _table(t) { }
		bool first();
		bool next();
};



#endif
