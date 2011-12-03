#ifndef COLLECTOPENSOFDEFINITIONS_HPP_
#define COLLECTOPENSOFDEFINITIONS_HPP_

#include <set>

#include "visitors/TheoryVisitor.hpp"

class Definition;
class PFSymbol;

class CollectOpensOfDefinitions: public TheoryVisitor {
	VISITORFRIENDS()
private:
	Definition* _definition;
	std::set<PFSymbol*> _result;

public:
	const std::set<PFSymbol*>& execute(Definition* d);
protected:
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* COLLECTOPENSOFDEFINITIONS_HPP_ */
