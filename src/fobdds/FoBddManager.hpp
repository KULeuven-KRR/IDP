/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "FoBddUtils.hpp"
#include "FoBddVariable.hpp"
#include "vocabulary/VarCompare.hpp"
#include "FoBddIndex.hpp"

class FOBDDTerm;
class FOBDDFuncTerm;
class FOBDDDeBruijnIndex;
class FOBDDDomainTerm;

class FOBDDKernel;
class FOBDDAtomKernel;
class FOBDDQuantKernel;
class FOBDDAggKernel;
class FOBDDAggTerm;
class FOBDDSetExpr;
class FOBDDQuantSetExpr;
class FOBDDEnumSetExpr;
class FOBDD;
class PFSymbol;
class Variable;
class Sort;
class Formula;
class Term;
class Structure;
class DomainTerm;
class DomainElement;
struct tablesize;
class Function;
struct CompareBDDVars;
enum class CompType;

typedef std::map<const FOBDD*, FOBDD*> MBDDBDD;
typedef std::map<const FOBDD*, MBDDBDD> MBDDMBDDBDD;
// The BDDTable takes a Kernel, and two BDDs and maps it to the corresponding BDD
// FIXME: currently, this table takes FIRST the falsbranch, then the truebranch.
// It would be more logical the other way round (to be consistent with the order of the arguments in all bdd methods)
typedef std::map<const FOBDDKernel*, MBDDMBDDBDD> BDDTable;

typedef std::map<std::vector<const FOBDDTerm*>, FOBDDAtomKernel*> MVTAK;
typedef std::map<AtomKernelType, MVTAK> MAKTMVTAK;
typedef std::map<PFSymbol*, MAKTMVTAK> AtomKernelTable;
typedef std::map<const FOBDD*, FOBDDQuantKernel*> MBDDQK;
typedef std::map<Sort*, MBDDQK> QuantKernelTable;
typedef std::map<const FOBDDTerm*, std::map<CompType, std::map<const FOBDDAggTerm*, FOBDDAggKernel*> > > AggKernelTable;

typedef std::map<unsigned int, FOBDDKernel*> MIK;
typedef std::map<KernelOrderCategory, MIK> KernelTable;

typedef std::map<Variable*, FOBDDVariable*> VariableTable;
typedef std::map<unsigned int, FOBDDDeBruijnIndex*> MUIDB;
typedef std::map<Sort*, MUIDB> DeBruijnIndexTable;
typedef std::map<const DomainElement*, FOBDDDomainTerm*> MTEDT;
typedef std::map<Sort*, MTEDT> DomainTermTable;
typedef std::map<std::vector<const FOBDDTerm*>, FOBDDFuncTerm*> MVAFT;
typedef std::map<Function*, MVAFT> FuncTermTable;
typedef std::map<AggFunction, std::map<const FOBDDEnumSetExpr*, FOBDDAggTerm*> > AggTermTable;

typedef pair<bool, const FOBDDKernel*> Choice;
typedef vector<Choice> Path;

/**
 * Class to create and manage first-order BDDs
 */
class FOBDDManager : public std::enable_shared_from_this<FOBDDManager> {
private:
	uint _maxid;

	//Option
	bool _rewriteArithmetic; //<!normally, this is true, only false if explicitly asked not to do rewritings

	// Leaf nodes
	FOBDD* _truebdd; //!< the BDD 'true'
	FOBDD* _falsebdd; //!< the BDD 'false'
	FOBDDKernel* _truekernel; //!< the kernel 'true'
	FOBDDKernel* _falsekernel; //!< the kernel 'false'

	// Order
	std::map<KernelOrderCategory, unsigned int> _nextorder;

	// Global tables
	BDDTable _bddtable;
	AtomKernelTable _atomkerneltable;
	QuantKernelTable _quantkerneltable;
	AggKernelTable _aggkerneltable;
	VariableTable _variabletable;
	DeBruijnIndexTable _debruijntable;
	FuncTermTable _functermtable;
	AggTermTable _aggtermtable;
	DomainTermTable _domaintermtable;
	KernelTable _kernels;

	// Dynamic programming tables
	std::map<const FOBDD*, const FOBDD*> _negationtable;
	std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > _conjunctiontable;
	std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > _disjunctiontable;
	//_ifthenelsetable is a map:
	// kernel -> ( truebdd -> (falsebdd -> result))
	//Or, said differently: (kernel, truebdd, falsebdd) -> result
	std::map<const FOBDDKernel*, std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > > _ifthenelsetable;
	std::map<Sort*, std::map<const FOBDD*, const FOBDD*> > _quanttable;
	double getTotalWeigthedCost(const FOBDD* bdd, const fobddvarset& vars, const fobddindexset& indices,
			const Structure* structure, double weightPerAns);
	//Private since this does no merging.  If you want to create a BDD, use IfThenElse
	const FOBDD* getBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch);

public:
	//NOTE: if rewriteArithmetic is false, a lot of operations on bdds are no longer as efficient (or even possible) (for example solve)
	//Only set this to false is you want to simplify a formula by formula->bdd->formula
	~FOBDDManager();
	static std::shared_ptr<FOBDDManager> createManager(bool rewriteArithmetic= true);
	const FOBDD* truebdd() const {
		return _truebdd;
	}
	const FOBDD* falsebdd() const {
		return _falsebdd;
	}

	bool isGoalbdd(bool goal, const FOBDD* bdd) const {
		return (goal && isTruebdd(bdd)) || ((not goal) && isFalsebdd(bdd));
	}

	bool isTruebdd(const FOBDD* bdd) const {
		return _truebdd == bdd;
	}
	bool isFalsebdd(const FOBDD* bdd) const {
		return _falsebdd == bdd;
	}

	const FOBDDKernel* getAtomKernel(PFSymbol*, AtomKernelType, const std::vector<const FOBDDTerm*>&);
	const FOBDDKernel* getQuantKernel(Sort* sort, const FOBDD* bdd);
	const FOBDDKernel* getAggKernel(const FOBDDTerm* left, CompType comp, const FOBDDTerm* right);

	const FOBDDTerm* getFOBDDTerm(Term*);

	const FOBDDEnumSetExpr* getEnumSetExpr(const std::vector<const FOBDDQuantSetExpr*>& subsets, Sort* sort);
	//This method assumes that the formula is already bumped and that all quantified variables are already replaced by their debruynindices.
	//If this is not the case, use setquantify!
	const FOBDDQuantSetExpr* getQuantSetExpr(const std::vector<Sort*>& varsorts, const FOBDD* formula, const FOBDDTerm* term, Sort* sort);

	const FOBDDVariable* getVariable(Variable* var);
	const FOBDDDeBruijnIndex* getDeBruijnIndex(Sort* sort, unsigned int index);
	const FOBDDTerm* getFuncTerm(Function* func, const std::vector<const FOBDDTerm*>& args);
	const FOBDDTerm* getAggTerm(AggFunction func, const FOBDDEnumSetExpr* set);
	const FOBDDDomainTerm* getDomainTerm(const DomainTerm* dt);
	const FOBDDDomainTerm* getDomainTerm(Sort* sort, const DomainElement* value);

	fobddvarset getVariables(const varset& vars);

	const FOBDD* negation(const FOBDD*);
	const FOBDD* conjunction(const FOBDD*, const FOBDD*);
	const FOBDD* disjunction(const FOBDD*, const FOBDD*);
	const FOBDD* univquantify(const FOBDDVariable*, const FOBDD*);
	const FOBDD* existsquantify(const FOBDDVariable*, const FOBDD*);
	const FOBDD* univquantify(const fobddvarset&, const FOBDD*);
	const FOBDD* existsquantify(const fobddvarset&, const FOBDD*);
	const FOBDD* ifthenelse(const FOBDDKernel*, const FOBDD* truebranch, const FOBDD* falsebranch);
	//Does the same as ifthenelse but puts kernel above true and falsebranch
	const FOBDD* ifthenelseTryMaintainOrder(const FOBDDKernel*, const FOBDD* truebranch, const FOBDD* falsebranch);
	const FOBDD* replaceFreeVariablesByIndices(const fobddvarset&, const FOBDD*);

	const FOBDDQuantSetExpr* setquantify(const std::vector<const FOBDDVariable*>& vars, const FOBDD* formula, const FOBDDTerm* term, Sort* sort);

	//All of the "subsitute" methods substitute their first argument (or the first argument of the map) by the second.
	const FOBDD* substitute(const FOBDD*, const std::map<const FOBDDVariable*, const FOBDDVariable*>&);
	const FOBDD* substitute(const FOBDD*, const FOBDDVariable*, const FOBDDDeBruijnIndex*);
	const FOBDDKernel* substitute(const FOBDDKernel*, const FOBDDDomainTerm*, const FOBDDVariable*);
	const FOBDD* substitute(const FOBDD*, const std::map<const FOBDDVariable*, const FOBDDTerm*>&);

	// The substituteIndex methods substitute the index with the variable and decrement all higher indices
	const FOBDDTerm* substituteIndex(const FOBDDTerm*, const FOBDDDeBruijnIndex* index, const FOBDDVariable* variable);
	const FOBDD* substituteIndex(const FOBDD*, const FOBDDDeBruijnIndex*, const FOBDDVariable*);

	bool contains(const FOBDDKernel*, Variable*);
	bool contains(const FOBDDKernel*, const FOBDDVariable*);
	bool contains(const FOBDD*, const FOBDDVariable*);
	bool contains(const FOBDDTerm*, const FOBDDVariable*);
	bool contains(const FOBDDTerm*, const FOBDDTerm*);
	bool containsFuncTerms(const FOBDDKernel*);
	bool containsFuncTerms(const FOBDD*);

	Formula* toFormula(const FOBDD*);
	Formula* toFormula(const FOBDDKernel*);
	Term* toTerm(const FOBDDTerm*);

	/*
	 * Optimizes a bdd for being queried. The BDDBasedGeneratorfactory (the only class querying BDDs) will execute this. Hence execution of this method is not needed in most cases.
	 * However, if you want to make more true/false it's best to optimize first (to be sure not to throw away too much information)
	 */
	void optimizeQuery(const FOBDD*, const fobddvarset&, const fobddindexset&,
			const Structure*);

	const FOBDD* makeMoreFalse(const FOBDD*, const fobddvarset&, const fobddindexset&,
			const Structure*, double weight_per_ans);
	const FOBDD* makeMoreTrue(const FOBDD*, const fobddvarset&, const fobddindexset&,
			const Structure*, double weight_per_ans);

	//Makes more parts of a bdd false. The resulting bdd will contain no symbols from the given list of symbols to remove
	const FOBDD* makeMoreFalse(const FOBDD*, const std::set<PFSymbol*>& symbolsToRemove);
	//Makes more parts of a bdd true. The resulting bdd will contain no symbols from the given list of symbols to remove
	const FOBDD* makeMoreTrue(const FOBDD*, const std::set<PFSymbol*>& symbolsToRemove );

	const FOBDD* simplify(const FOBDD*); //!< apply arithmetic simplifications to the given bdd

	const FOBDD* getBDD(const FOBDD* bdd, std::shared_ptr<FOBDDManager>); //!< Given a bdd and the manager that created the bdd,
														  //!< this function returns the same bdd, but created
														  //!< by the manager 'this'
	const FOBDD* getBDDTryMaintainOrder(const FOBDD* bdd, std::shared_ptr<FOBDDManager>); //!< Given a bdd and the manager that created the bdd,
														  //!< this function returns the same bdd, but created
														  //!< by the manager 'this', and tries to maintain order
	/**
	 * Try to rewrite the given arithmetic kernel such that the right-hand side is the given argument,
	 * and such that the given argument does not occur in the left-hand side.
	 * Returns an FOBDDTerm "term" such that the given arithmetic kernel is equivalent to
	 * term op rhs
	 * where op is kernel.symbol
	 * Returns a null-pointer in case this is impossible.
	 * Only guaranteed to work correctly on variables and indices with a FOBDDAtomKernel.
	 */
	const FOBDDTerm* solve(const FOBDDKernel* kernel, const FOBDDTerm* rhs); //TODO review, currently only works for  "="...

	bool containsPartialFunctions(const FOBDDTerm*); //!< Returns true iff the given term is partial

	int longestbranch(const FOBDDKernel*);
	int longestbranch(const FOBDD*);

	std::vector<Path> pathsToFalse(const FOBDD* bdd) const;

private:
	FOBDDManager(bool rewriteArithmetic = true);
	KernelOrder newOrder(KernelOrderCategory category);
	KernelOrder newOrder(const std::vector<const FOBDDTerm*>& args);
	KernelOrder newOrderForQuantifiedBDD(const FOBDD* bdd);
	KernelOrder newOrder(const FOBDDAggTerm* aggterm);

	FOBDD* addBDD(const FOBDDKernel* kernel, const FOBDD* falsebranch, const FOBDD* truebranch);
	FOBDDAtomKernel* addAtomKernel(PFSymbol* symbol, AtomKernelType akt, const std::vector<const FOBDDTerm*>& args);
	FOBDDQuantKernel* addQuantKernel(Sort* sort, const FOBDD* bdd);
	FOBDDAggKernel* addAggKernel(const FOBDDTerm* left, CompType comp, const FOBDDAggTerm* right);
	FOBDDVariable* addVariable(Variable* var);
	FOBDDDeBruijnIndex* addDeBruijnIndex(Sort* sort, unsigned int index);
	FOBDDFuncTerm* addFuncTerm(Function* func, const std::vector<const FOBDDTerm*>& args);
	FOBDDAggTerm* addAggTerm(AggFunction func, const FOBDDEnumSetExpr* set);
	FOBDDDomainTerm* addDomainTerm(Sort* sort, const DomainElement* value);
	FOBDDEnumSetExpr* addEnumSetExpr(const std::vector<const FOBDDQuantSetExpr*>& subsets, Sort* sort);
	FOBDDQuantSetExpr* addQuantSetExpr(const std::vector<Sort*>& varsorts, const FOBDD* formula, const FOBDDTerm* term, Sort* sort);

	void setTrueKernel(FOBDDKernel* k){
		_truekernel=k;
	}
	void setFalseKernel(FOBDDKernel* k){
		_falsekernel=k;
	}
	void setTrueBDD(FOBDD* bdd){
		_truebdd=bdd;
	}
	void setFalseBDD(FOBDD* bdd){
		_falsebdd=bdd;
	}
	void clearDynamicTables();

	const FOBDD* quantify(Sort* sort, const FOBDD* bdd);

	std::map<const FOBDDKernel*, tablesize> kernelUnivs(const FOBDD*, const Structure* structure);

	const FOBDD* makeMore(bool goal, const FOBDD*, const fobddvarset&, const fobddindexset&,
			const Structure*, double weight_per_ans); //Depending on goal, makes more pieces of the BDD true or false
	const FOBDD* makeMore(bool goal, const FOBDD* bdd, const std::set<PFSymbol*>& symbolsToRemove); //Depending on goal, makes more pieces of the BDD true or false such that result contains no forbidden symbols

	const FOBDDTerm* invert(const FOBDDTerm*);

	void moveDown(const FOBDDKernel*); //!< Swap the given kernel with its successor in the kernelorder
	void moveUp(const FOBDDKernel*); //!< Swap the given kernel with its predecessor in the kernelorder

	FOBDDKernel* kernelAbove(const FOBDDKernel*); //!< Returns the kernel of the same category, directly above the given one. Returns NULL if none such exists
	FOBDDKernel* kernelBelow(const FOBDDKernel*); //!< Returns the kernel of the same category, directly below the given one. Returns NULL if none such exists
};

fobddvarset variables(const FOBDDKernel*, std::shared_ptr<FOBDDManager> manager);
fobddvarset variables(const FOBDD*, std::shared_ptr<FOBDDManager> manager);
fobddindexset indices(const FOBDDKernel*, std::shared_ptr<FOBDDManager> manager);
fobddindexset indices(const FOBDD*, std::shared_ptr<FOBDDManager> manager);
std::set<const FOBDDKernel*> nonnestedkernels(const FOBDD* bdd, const std::shared_ptr<FOBDDManager> manager);
std::set<const FOBDDKernel*> allkernels(const FOBDD* bdd, const std::shared_ptr<FOBDDManager> manager);

/**
 * Returns the product of the sizes of the interpretations of the sorts of the given variables and indices in the given structure
 */
tablesize univNrAnswers(const fobddvarset& vars, const fobddindexset& indices, const Structure* structure);
