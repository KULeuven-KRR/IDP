#ifndef ADDCOMPLETION_HPP_
#define ADDCOMPLETION_HPP_

#include <vector>
#include <map>
#include "TheoryMutatingVisitor.hpp"

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
