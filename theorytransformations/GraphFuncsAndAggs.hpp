#ifndef GRAPHFUNCSANDAGGS_HPP_
#define GRAPHFUNCSANDAGGS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "parseinfo.hpp"
#include "commontypes.hpp"

class GraphFuncsAndAggs : public TheoryMutatingVisitor {
	VISITORFRIENDS()
//private:
//	bool _recursive;
public:
	template<typename T>
	T execute(T t/*, bool recursive = false*/){
//		_recursive = recursive;
		return t->accept(this);
	}
protected:
	Formula* visit(PredForm* pf);
	Formula* visit(EqChainForm* ef);
private:
	CompType getCompType(const PredForm* pf) const;
	PredForm* makeFuncGraph(SIGN, Term* functerm, Term* valueterm, const FormulaParseInfo&) const;
	AggForm* makeAggForm(Term* valueterm, CompType, Term* aggterm, const FormulaParseInfo&) const;
};

#endif /* GRAPHFUNCSANDAGGS_HPP_ */
