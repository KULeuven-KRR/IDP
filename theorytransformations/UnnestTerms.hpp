/************************************
  	UnnestTerms.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MOVETERMS_HPP_
#define MOVETERMS_HPP_

#include <set>
#include <vector>
#include "commontypes.hpp"
#include "theory.hpp"
#include "term.hpp"

#include "visitors/TheoryMutatingVisitor.hpp"

class Vocabulary;
class Variable;

/**
 * Moves nested terms out
 *
 * NOTE: equality is NOT rewritten! (rewriting f(x)=y to ?z: f(x)=z & z=y is quite useless)
 */
class UnnestTerms: public TheoryMutatingVisitor {
private:
	Vocabulary* 			_vocabulary; //!< Used to do type derivation during rewrites
	Context 				_context; //!< Keeps track of the current context where terms are moved
	bool 					_allowedToUnnest; // Indicates whether in the current context, it is allowed to unnest terms
	std::vector<Formula*> 	_equalities; //!< used to temporarily store the equalities generated when moving terms
	std::set<Variable*> 	_variables; //!< used to temporarily store the freshly introduced variables

	void contextProblem(Term* t);

protected:
	virtual bool shouldMove(Term* t);
	bool getAllowedToUnnest(){
		return _allowedToUnnest;
	}
	void setAllowedToUnnest(bool allowed){
		_allowedToUnnest = allowed;
	}
	Context getContext() { return _context; }
	void setContext(const Context& context) { _context = context; }

public:
	UnnestTerms(Context context = Context::POSITIVE, Vocabulary* v = NULL) :
			_vocabulary(v), _context(context), _allowedToUnnest(false) {
	}

	VarTerm* move(Term* term);

	Formula* rewrite(Formula* formula) ;

	Theory* visit(Theory* theory);

	virtual Rule* visit(Rule* rule);

	virtual Formula* traverse(Formula* f);

	virtual Formula* traverse(PredForm* f);

	virtual Formula* visit(EquivForm* ef);

	virtual Formula* visit(AggForm* af);

	virtual Formula* visit(EqChainForm* ef);

	virtual Formula* visit(PredForm* predform);

	virtual Term* traverse(Term* term);

	VarTerm* visit(VarTerm* t);

	virtual Term* visit(DomainTerm* t) ;

	virtual Term* visit(AggTerm* t);

	virtual Term* visit(FuncTerm* ft);

	virtual SetExpr* visit(EnumSetExpr* s);

	virtual SetExpr* visit(QuantSetExpr* s);
};



#endif /* MOVETERMS_HPP_ */
