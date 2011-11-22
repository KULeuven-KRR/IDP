#include <cassert>
#include "Grounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

void Grounder::toplevelRun() const{
	//assert(context()._conjunctivePathFromRoot);
	ConjOrDisj formula;
	run(formula);
	if(formula.literals.size()==0){
		if(formula.type==Conn::DISJ){ // UNSAT
			grounding()->addUnitClause(1);
			grounding()->addUnitClause(-1);
		}
	}else if(formula.literals.size()==1){
		Lit l = formula.literals.back();

		if(l==_true || l==_false){
			if(formula.type==Conn::CONJ && l==_false){ // UNSAT
				grounding()->addUnitClause(1);
				grounding()->addUnitClause(-1);
			} // else SAT or irrelevant (TODO correct?)
		}else{
			grounding()->addUnitClause(l);
		}
	}else{
		if(formula.type==Conn::CONJ){
			for(auto i=formula.literals.cbegin(); i<formula.literals.cend(); ++i){
				grounding()->addUnitClause(*i);
			}
		}else{
			grounding()->add(formula.literals);
		}
	}
	grounding()->closeTheory(); // TODO very important and easily forgotten
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
		return grounding()->translator()->translate(formula.literals, formula.type==Conn::CONJ, context()._tseitin);
	}
}
