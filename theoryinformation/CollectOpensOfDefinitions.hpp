#ifndef COLLECTOPENSOFDEFINITIONS_HPP_
#define COLLECTOPENSOFDEFINITIONS_HPP_

#include <set>

#include "TheoryVisitor.hpp"

class Definition;
class PFSymbol;

class CollectOpensOfDefinitions: public TheoryVisitor {
private:
	Definition* _definition;
	std::set<PFSymbol*> _result;

public:
	const std::set<PFSymbol*>& run(Definition* d);

	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* COLLECTOPENSOFDEFINITIONS_HPP_ */
