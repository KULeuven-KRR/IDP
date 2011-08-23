/************************************
	checker.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "parseinfo.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "checker.hpp"

using namespace std;

InstanceChecker* CheckerFactory::create(PredInter* inter, TABLE_VALUE type) {
	const PredTable* pt = NULL;
	switch(type){
		case POSS_FALSE: 	pt = inter->pf(); break;
		case POSS_TRUE: 	pt = inter->pt(); break;
		case CERTAIN_FALSE: pt = inter->cf(); break;
		case CERTAIN_TRUE:	pt = inter->ct(); break;
	}
	if(pt->approxempty()){
		return new FalseInstanceChecker();
	}else{
		return new TableInstanceChecker(pt);
	}
}

inline bool TableInstanceChecker::isInInterpretation(const ElementTuple& vd) const {
	return _table->contains(vd);
}
