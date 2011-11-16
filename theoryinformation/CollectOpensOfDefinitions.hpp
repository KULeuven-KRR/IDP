/************************************
  	CollectOpensOfDefinitions.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COLLECTOPENSOFDEFINITIONS_HPP_
#define COLLECTOPENSOFDEFINITIONS_HPP_

#include <set>

#include "visitors/TheoryVisitor.hpp"

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
