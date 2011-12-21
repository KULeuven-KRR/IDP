/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef UNNESTTERMS_HPP_
#define UNNESTTERMS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include "commontypes.hpp"
#include <set>
#include <vector>
#include "theory.hpp"
#include "term.hpp"

class AbstractStructure;
class Vocabulary;
class Formula;
class Variable;
class Sort;
class Term;

/**
 * Moves nested terms out
 *
 * NOTE: equality is NOT rewritten! (rewriting f(x)=y to ?z: f(x)=z & z=y is quite useless)
 */
class UnnestTerms: public TheoryMutatingVisitor {
	VISITORFRIENDS()
protected:
	AbstractStructure*		_structure; //!< Used to find bounds on introduced variables for aggregates
	Vocabulary* 			_vocabulary; //!< Used to do type derivation during rewrites

private:
	Context 				_context; //!< Keeps track of the current context where terms are moved
	bool 					_allowedToUnnest; // Indicates whether in the current context, it is allowed to unnest terms
	std::vector<Formula*> 	_equalities; //!< used to temporarily store the equalities generated when moving terms
	std::set<Variable*> 	_variables; //!< used to temporarily store the freshly introduced variables
	Sort* 					_chosenVarSort;

	void contextProblem(Term* t);

protected:
	virtual bool shouldMove(Term* t);

	bool getAllowedToUnnest() const {
		return _allowedToUnnest;
	}
	void setAllowedToUnnest(bool allowed) {
		_allowedToUnnest = allowed;
	}
	const Context& getContext() const { 
		return _context; 
	}
	void setContext(const Context& context) { 
		_context = context; 
	}

public:
	UnnestTerms();

	template<typename T>
	T execute(T t, Context con = Context::POSITIVE, AbstractStructure* str = NULL, Vocabulary* voc = NULL) {
		_context = con;
		_structure = str;
		_vocabulary = voc;
		_allowedToUnnest = false;
		return t->accept(this);
	}

protected:
	Formula* rewrite(Formula* formula);

	VarTerm* move(Term* term);

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

private:
	template<typename T>
	Formula* doRewrite(T origformula);

	Sort* deriveSort(Term* term);
};

#endif /* UNNESTTERMS_HPP_ */
