/************************************
	AddCompletion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ADDCOMPLETION_HPP_
#define ADDCOMPLETION_HPP_

#include <vector>
#include <map>
#include "visitors/TheoryMutatingVisitor.hpp"

class Variable;
class PFSymbol;

class AddCompletion: public TheoryMutatingVisitor {
private:
	std::vector<Formula*> _result;
	std::map<PFSymbol*, std::vector<Variable*> > _headvars;
	std::map<PFSymbol*, std::vector<Formula*> > _interres;

public:
	AddCompletion() {
	}

	Theory* visit(Theory*);
	Definition* visit(Definition*);
	Rule* visit(Rule*);
};

#endif /* ADDCOMPLETION_HPP_ */
