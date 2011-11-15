#ifndef FOBDD_HPP
#define FOBDD_HPP

#include <vector>
#include <map>
#include <set>
#include <string>

#include "fobdds/FoBddUtils.hpp"

class FOBDDArgument;
class FOBDDVariable;
class FOBDDFuncTerm;
class FOBDDDeBruijnIndex;
class FOBDDDomainTerm;

class FOBDDKernel;
class FOBDDAtomKernel;
class FOBDDQuantKernel;
class FOBDD;
class PFSymbol;
class Variable;
class Sort;
class Formula;
class Term;
class AbstractStructure;
class DomainElement;
class tablesize;
class Function;

typedef std::map<const FOBDD*, FOBDD*> MBDDBDD;
typedef std::map<const FOBDD*, MBDDBDD> MBDDMBDDBDD;
typedef std::map<const FOBDDKernel*, MBDDMBDDBDD> BDDTable;

typedef std::map<std::vector<const FOBDDArgument*>, FOBDDAtomKernel*> MVAGAK;
typedef std::map<AtomKernelType, MVAGAK> MAKTMVAGAK;
typedef std::map<PFSymbol*, MAKTMVAGAK> AtomKernelTable;
typedef std::map<const FOBDD*, FOBDDQuantKernel*> MBDDQK;
typedef std::map<Sort*, MBDDQK> QuantKernelTable;

typedef std::map<unsigned int, FOBDDKernel*> MIK;
typedef std::map<unsigned int, MIK> KernelTable;

typedef std::map<Variable*, FOBDDVariable*> VariableTable;
typedef std::map<unsigned int, FOBDDDeBruijnIndex*> MUIDB;
typedef std::map<Sort*, MUIDB> DeBruijnIndexTable;
typedef std::map<const DomainElement*, FOBDDDomainTerm*> MTEDT;
typedef std::map<Sort*, MTEDT> DomainTermTable;
typedef std::map<std::vector<const FOBDDArgument*>, FOBDDFuncTerm*> MVAFT;
typedef std::map<Function*, MVAFT> FuncTermTable;

/**
 * Class to create and manage first-order BDDs
 */
class FOBDDManager {
private:
	// Leaf nodes
	FOBDD* _truebdd; //!< the BDD 'true'
	FOBDD* _falsebdd; //!< the BDD 'false'
	FOBDDKernel* _truekernel; //!< the kernel 'true'
	FOBDDKernel* _falsekernel; //!< the kernel 'false'

	// Order
	std::map<unsigned int, unsigned int> _nextorder;

	// Global tables
	BDDTable _bddtable;
	AtomKernelTable _atomkerneltable;
	QuantKernelTable _quantkerneltable;
	VariableTable _variabletable;
	DeBruijnIndexTable _debruijntable;
	FuncTermTable _functermtable;
	DomainTermTable _domaintermtable;
	KernelTable _kernels;

	// Dynamic programming tables
	std::map<const FOBDD*, const FOBDD*> _negationtable;
	std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > _conjunctiontable;
	std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > _disjunctiontable;
	std::map<const FOBDDKernel*, std::map<const FOBDD*, std::map<const FOBDD*, const FOBDD*> > > _ifthenelsetable;
	std::map<Sort*, std::map<const FOBDD*, const FOBDD*> > _quanttable;

public:
	FOBDDManager();

	const FOBDD* truebdd() const {
		return _truebdd;
	}
	const FOBDD* falsebdd() const {
		return _falsebdd;
	}

	bool isTruebdd(const FOBDD* bdd) const {
		return _truebdd == bdd;
	}
	bool isFalsebdd(const FOBDD* bdd) const {
		return _falsebdd == bdd;
	}

	const FOBDD* getBDD(const FOBDDKernel* kernel, const FOBDD* truebranch, const FOBDD* falsebranch);
	const FOBDDKernel* getAtomKernel(PFSymbol*, AtomKernelType, const std::vector<const FOBDDArgument*>&);
	const FOBDDKernel* getQuantKernel(Sort* sort, const FOBDD* bdd);
	const FOBDDVariable* getVariable(Variable* var);
	const FOBDDDeBruijnIndex* getDeBruijnIndex(Sort* sort, unsigned int index);
	const FOBDDArgument* getFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args);
	const FOBDDDomainTerm* getDomainTerm(Sort* sort, const DomainElement* value);

	std::set<const FOBDDVariable*> getVariables(const std::set<Variable*>& vars);

	const FOBDD* negation(const FOBDD*);
	const FOBDD* conjunction(const FOBDD*, const FOBDD*);
	const FOBDD* disjunction(const FOBDD*, const FOBDD*);
	const FOBDD* univquantify(const FOBDDVariable*, const FOBDD*);
	const FOBDD* existsquantify(const FOBDDVariable*, const FOBDD*);
	const FOBDD* univquantify(const std::set<const FOBDDVariable*>&, const FOBDD*);
	const FOBDD* existsquantify(const std::set<const FOBDDVariable*>&, const FOBDD*);
	const FOBDD* ifthenelse(const FOBDDKernel*, const FOBDD* truebranch, const FOBDD* falsebranch);
	const FOBDD* substitute(const FOBDD*, const std::map<const FOBDDVariable*, const FOBDDVariable*>&);
	const FOBDD* substitute(const FOBDD*, const FOBDDDeBruijnIndex*, const FOBDDVariable*);
	const FOBDDKernel* substitute(const FOBDDKernel*, const FOBDDDomainTerm*, const FOBDDVariable*);
	const FOBDD* substitute(const FOBDD*, const std::map<const FOBDDVariable*, const FOBDDArgument*>&);

	std::ostream& put(std::ostream&, const FOBDD*, unsigned int spaces = 0) const;
	std::ostream& put(std::ostream&, const FOBDDKernel*, unsigned int spaces = 0) const;
	std::ostream& put(std::ostream&, const FOBDDArgument*) const;

	bool contains(const FOBDDKernel*, Variable*);
	bool contains(const FOBDDKernel*, const FOBDDVariable*);
	bool contains(const FOBDD*, const FOBDDVariable*);
	bool contains(const FOBDDArgument*, const FOBDDVariable*);
	bool contains(const FOBDDArgument*, const FOBDDArgument*);
	bool containsFuncTerms(const FOBDDKernel*);
	bool containsFuncTerms(const FOBDD*);

	Formula* toFormula(const FOBDD*);
	Formula* toFormula(const FOBDDKernel*);
	Term* toTerm(const FOBDDArgument*);

	double estimatedNrAnswers(const FOBDDKernel*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&,
			AbstractStructure*);
	double estimatedNrAnswers(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);
	double estimatedCostAll(bool, const FOBDDKernel*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&,
			AbstractStructure*);
	double estimatedCostAll(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);

	void optimizequery(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*);
	const FOBDD* make_more_false(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*,
			double weight_per_ans);
	const FOBDD* make_more_true(const FOBDD*, const std::set<const FOBDDVariable*>&, const std::set<const FOBDDDeBruijnIndex*>&, AbstractStructure*,
			double weight_per_ans);

	const FOBDD* simplify(const FOBDD*); //!< apply arithmetic simplifications to the given bdd

	const FOBDD* getBDD(const FOBDD* bdd, FOBDDManager*); //!< Given a bdd and the manager that created the bdd,
														  //!< this function returns the same bdd, but created
														  //!< by the manager 'this'

	/**
	 * Try to rewrite the given arithmetic kernel such that the right-hand side is the given argument,
	 * and such that the given argument does not occur in the left-hand side.
	 * Returns a null-pointer in case this is impossible.
	 * Only guaranteed to work correctly on variables and indices.
	 */
	const FOBDDArgument* solve(const FOBDDKernel*, const FOBDDArgument*);

	bool partial(const FOBDDArgument*); //!< Returns true iff the given term is partial

	int longestbranch(const FOBDDKernel*);
	int longestbranch(const FOBDD*);

private:
	KernelOrder newOrder(unsigned int category);
	KernelOrder newOrder(const std::vector<const FOBDDArgument*>& args);
	KernelOrder newOrder(const FOBDD* bdd);

	FOBDD* addBDD(const FOBDDKernel* kernel, const FOBDD* falsebranch, const FOBDD* truebranch);
	FOBDDAtomKernel* addAtomKernel(PFSymbol* symbol, AtomKernelType akt, const std::vector<const FOBDDArgument*>& args);
	FOBDDQuantKernel* addQuantKernel(Sort* sort, const FOBDD* bdd);
	FOBDDVariable* addVariable(Variable* var);
	FOBDDDeBruijnIndex* addDeBruijnIndex(Sort* sort, unsigned int index);
	FOBDDFuncTerm* addFuncTerm(Function* func, const std::vector<const FOBDDArgument*>& args);
	FOBDDDomainTerm* addDomainTerm(Sort* sort, const DomainElement* value);

	void clearDynamicTables();

	const FOBDD* quantify(Sort* sort, const FOBDD* bdd);

	std::set<const FOBDDVariable*> variables(const FOBDDKernel*);
	std::set<const FOBDDVariable*> variables(const FOBDD*);
	std::set<const FOBDDDeBruijnIndex*> indices(const FOBDDKernel*);
	std::set<const FOBDDDeBruijnIndex*> indices(const FOBDD*);
	std::map<const FOBDDKernel*, tablesize> kernelUnivs(const FOBDD*, AbstractStructure* structure);

	std::vector<std::vector<std::pair<bool, const FOBDDKernel*> > > pathsToFalse(const FOBDD* bdd);
	std::set<const FOBDDKernel*> nonnestedkernels(const FOBDD* bdd);
	std::set<const FOBDDKernel*> allkernels(const FOBDD* bdd);
	std::map<const FOBDDKernel*, double> kernelAnswers(const FOBDD*, AbstractStructure*);
	double estimatedChance(const FOBDDKernel*, AbstractStructure*);
	double estimatedChance(const FOBDD*, AbstractStructure*);

	const FOBDDArgument* invert(const FOBDDArgument*);

	void moveDown(const FOBDDKernel*); //!< Swap the given kernel with its successor in the kernelorder
	void moveUp(const FOBDDKernel*); //!< Swap the given kernel with its predecessor in the kernelorder
};

#endif
