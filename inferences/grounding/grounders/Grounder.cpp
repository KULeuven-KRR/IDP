/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "common.hpp"
#include "Grounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

void ConjOrDisj::put(std::ostream& stream) const{
	bool begin = true;
	for(auto i=literals.cbegin(); i<literals.cend(); ++i){
		if(not begin){
			switch(type){
			case Conn::DISJ:
				stream <<" | ";
				break;
			case Conn::CONJ:
				stream <<" & ";
				break;
			}
		}
		begin = false;
		stream <<toString(*i);
	}
}

GroundTranslator* Grounder::getTranslator() const {
	return _grounding->translator();
}

void Grounder::toplevelRun() const{
	//Assert(context()._conjunctivePathFromRoot);
	ConjOrDisj formula;
	run(formula);
	if(formula.literals.size()==0){
		if(formula.type==Conn::DISJ){ // UNSAT
			getGrounding()->addUnitClause(1);
			getGrounding()->addUnitClause(-1);
		}
	}else if(formula.literals.size()==1){
		Lit l = formula.literals.back();

		if(l==_true || l==_false){
			if(formula.type==Conn::CONJ && l==_false){ // UNSAT
				getGrounding()->addUnitClause(1);
				getGrounding()->addUnitClause(-1);
			} // else SAT or irrelevant (TODO correct?)
		}else{
			getGrounding()->addUnitClause(l);
		}
	}else{
		if(formula.type==Conn::CONJ){
			for(auto i=formula.literals.cbegin(); i<formula.literals.cend(); ++i){
				getGrounding()->addUnitClause(*i);
			}
		}else{
			getGrounding()->add(formula.literals);
		}
	}
	getGrounding()->closeTheory(); // TODO very important and easily forgotten
}

Lit Grounder::groundAndReturnLit() const{
	ConjOrDisj formula;
	run(formula);
	if(formula.literals.size()==0){
		if(formula.type==Conn::DISJ){
			return _false;
		}else{
			return _true;
		}
	}else if(formula.literals.size()==1){
		return formula.literals.back();
	}else{
		return getGrounding()->translator()->translate(formula.literals, formula.type==Conn::CONJ, context()._tseitin);
	}
}
