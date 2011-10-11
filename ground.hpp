/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <cstdlib>
#include "theory.hpp"
#include "structure.hpp"
#include "commontypes.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

class PFSymbol;
class Variable;
class FuncTable;
class AbstractStructure;
class AbstractGroundTheory;
class InstGenerator;
class InstanceChecker;
class SortTable;
class DomainElement;
class Options;

typedef std::vector<const DomainElement*> ElementTuple;
typedef int Literal;
typedef std::vector<Literal> Clause;

typedef unsigned int VarId;

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

/**
 * Set corresponding to a tseitin.
 */ 
class TsSet {
	private:
		std::vector<int>	_setlits;		// All literals in the ground set
		std::vector<double>	_litweights;	// For each literal a corresponding weight
		std::vector<double>	_trueweights;	// The weights of the true literals in the set
	public:
		// Modifiers
		void	setWeight(unsigned int n, double w)	{ _litweights[n] = w;	}
		// Inspectors
		std::vector<int>	literals()				const { return _setlits; 			}
		std::vector<double>	weights()				const { return _litweights;			}
		std::vector<double>	trueweights()			const { return _trueweights;		}
		unsigned int		size() 					const { return _setlits.size();		}
		bool				empty()					const { return _setlits.empty();	}
		int					literal(unsigned int n)	const { return _setlits[n];			}
		double				weight(unsigned int n)	const { return _litweights[n];		}
	friend class GroundTranslator;
};

/**
 * A complete definition of a tseitin atom.
 */
class TsBody {
	protected:
		const TsType _type;	// the type of "tseitin definition"
		TsBody(TsType type): _type(type) { }
		virtual bool equal(const TsBody&) const;
		virtual bool compare(const TsBody&) const;
	public:
		virtual ~TsBody() { }
		TsType type() const { return _type; }
		friend bool operator==(const TsBody&, const TsBody&);
		friend bool operator<(const TsBody&, const TsBody&);
	friend class GroundTranslator;
};

class PCTsBody : public TsBody {
	private:
		std::vector<int> 	_body;	// the literals in the subformula replaced by the tseitin
		bool				_conj;	// if true, the replaced subformula is the conjunction of the literals in _body,
									// if false, the replaced subformula is the disjunction of the literals in _body
		//bool equal(const TsBody&) const;
		//bool compare(const TsBody&) const;
	public:
		PCTsBody(TsType type, const std::vector<int>& body, bool conj):
			TsBody(type), _body(body), _conj(conj) { }
		std::vector<int>	body() 					const { return _body; 			}
		unsigned int		size()					const { return _body.size();	}
		int					literal(unsigned int n)	const { return _body[n];		}
		bool				conj()					const { return _conj; 			}
	friend class GroundTranslator;
};

class AggTsBody : public TsBody {
	private:
		int			_setnr;
		AggFunction	_aggtype;
		bool		_lower;
		double		_bound;
		//bool equal(const TsBody&) const;
		//bool compare(const TsBody&) const;
	public:
		AggTsBody(TsType type, double bound, bool lower, AggFunction at, int setnr):
			TsBody(type), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		int			setnr()		const { return _setnr; 		}
		AggFunction	aggtype()	const { return _aggtype;	}
		bool		lower()		const { return _lower;		}
		double		bound()		const { return _bound;		}
	friend class GroundTranslator;
};

/* Sets and terms that will be handled by a constraint solver */

/**
 * Set of CP variable identifiers.
 */
class CPSet {
	public:
		std::vector<VarId> 		_varids;
};

/**
 * Abstract CP term class.
 */
class CPTerm {
	protected:
		virtual ~CPTerm() {	}
	private:
		virtual bool equal(const CPTerm&) const = 0;
		virtual bool compare(const CPTerm&) const = 0;
	public:
		virtual void accept(TheoryVisitor*) const = 0;
		friend bool operator==(const CPTerm&, const CPTerm&);
		friend bool operator<(const CPTerm&, const CPTerm&);
};

/**
 * Ordering class for CP terms.
 */
struct StrictWeakCPTermOrdering {
	bool operator()(const CPTerm* a, const CPTerm* b) const { return *a < *b; }
};

/**
 * CP term consisting of one CP variable.
 */
class CPVarTerm : public CPTerm {
	private:
		bool equal(const CPTerm&) const;
		bool compare(const CPTerm&) const;
	public:
		VarId 		_varid;
		CPVarTerm(const VarId& varid) : _varid(varid) { }
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

/**
 * CP term consisting of a sum of CP variables.
 */
class CPSumTerm : public CPTerm {
	private:
		bool equal(const CPTerm&) const;
		bool compare(const CPTerm&) const;
	public:
		std::vector<VarId> 		_varids;
		CPSumTerm(const VarId& left, const VarId& right) : _varids(2) { _varids[0] = left; _varids[1] = right; }
		CPSumTerm(const std::vector<VarId>& varids) : _varids(varids) { }
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

/**
 * CP term consisting of a weighted sum of CP variables.
 */
class CPWSumTerm : public CPTerm {
	private:
		bool equal(const CPTerm&) const;
		bool compare(const CPTerm&) const;
	public:
		std::vector<VarId> 		_varids; 
		std::vector<int>		_weights;
		CPWSumTerm(const std::vector<VarId>& varids, const std::vector<int>& weights) : _varids(varids), _weights(weights) { }
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

/**
 * A bound in a CP constraint can be an integer or a CP variable.
 */
struct CPBound {
	bool 		_isvarid;
	union { 
		int 	_bound;
		VarId 	_varid;
	};
	CPBound(const int& bound): _isvarid(false), _bound(bound) { }
	CPBound(const VarId& varid): _isvarid(true), _varid(varid) { }
	friend bool operator==(const CPBound&, const CPBound&);
	friend bool operator<(const CPBound&, const CPBound&);
};

/**
 * Tseitin body consisting of a CP constraint.
 */
class CPTsBody : public TsBody {
	private:
		CPTerm*		_left;
		CompType	_comp;
		CPBound		_right;
		bool equal(const TsBody&) const;
		bool compare(const TsBody&) const;
	public:
		CPTsBody(TsType type, CPTerm* left, CompType comp, const CPBound& right) :
			TsBody(type), _left(left), _comp(comp), _right(right) { }
		CPTerm*		left()	const { return _left;	}
		CompType	comp()	const { return _comp;	}
		CPBound		right()	const { return _right;	}
	friend class GroundTranslator;
};

/**
 * Ground translator 
 */
typedef std::map<ElementTuple,int,Compare<ElementTuple> > Tuple2Atom;
typedef std::map<TsBody*,int,Compare<TsBody> > Ts2Atom;
typedef std::pair<uint, TsBody*> tspair;

/**
 * Translator stores:
 * 		for a tseitin atom, what its interpretation is
 * 		for an input atom, what symbol it refers to and what elementtuple
 * 		for an atom which is neither, should not store anything, except that it is not stored.
 */

struct SymbolAndAtomMap{
	PFSymbol* symbol;
	Tuple2Atom tuple2atom;

	SymbolAndAtomMap(PFSymbol* symbol): symbol(symbol){}
};

enum class AtomType{
	INPUT, TSEITINWITHSUBFORMULA, LONETSEITIN
};

struct SymbolAndTuple{
	PFSymbol* symbol;
	ElementTuple tuple;

	SymbolAndTuple(){}
	SymbolAndTuple(PFSymbol* symbol, const ElementTuple& tuple): symbol(symbol), tuple(tuple){}
};

class GroundTranslator {
	private:
		std::vector<SymbolAndAtomMap>	symbols; // Each symbol added to the translated is associated a unique number, the index into this vector, at which the symbol is also stored

		std::vector<AtomType>			atomtype;
		std::vector<SymbolAndTuple*>	atom2Tuple;	// Pointers manager by the translator!
		std::vector<tspair>				atom2TsBody; // Pointers manager by the translator!

		std::queue<int>		_freenumbers;		// keeps atom numbers that were freed and can be used again
		std::queue<int>		_freesetnumbers;	// keeps set numbers that were freed and can be used again

		std::vector<TsSet>	_sets;	// keeps mapping between Set numbers and sets

		int	nextNumber(AtomType type);

	public:
		GroundTranslator() : atomtype(1, AtomType::LONETSEITIN),atom2Tuple(1, NULL), atom2TsBody(1, tspair(0,(TsBody*)NULL)), _sets(1) { }
		~GroundTranslator();

		int	translate(unsigned int,const ElementTuple&);
		int	translate(const std::vector<int>& cl, bool conj, TsType tp);
		int	translate(double bound, char comp, bool strict, AggFunction aggtype, int setnr, TsType tstype);
		int	translate(PFSymbol*,const ElementTuple&);
		int	translate(CPTerm*, CompType, const CPBound&, TsType);
		int	translateSet(const std::vector<int>&,const std::vector<double>&,const std::vector<double>&);

		unsigned int	addSymbol(PFSymbol* pfs);

		bool 		isStored(int atom)	const { return atom>0 && atomtype.size()>atom; }
		AtomType 	getType	(int atom)	const { return atomtype[atom]; }

		bool		isInputAtom	(int atom) 	const	{ return isStored(atom) && getType(atom)==AtomType::INPUT; }
		PFSymbol*	getSymbol	(int atom) 	const	{ assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL); return atom2Tuple[atom]->symbol; }
		const ElementTuple& getArgs(int atom) const { assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL); return atom2Tuple[atom]->tuple; }

		bool		isTseitinWithSubformula	(int atom)	const	{ return isStored(atom) && getType(atom)==AtomType::TSEITINWITHSUBFORMULA; }
		TsBody*		getTsBody				(int atom)	const	{ assert(isTseitinWithSubformula(atom)); return atom2TsBody[atom].second;	}

		int			createNewUninterpretedNumber(){
			return nextNumber(AtomType::LONETSEITIN);
		}

		bool		isSet		(int setID) const	{ return _sets.size()>setID; }
		TsSet&		groundset	(int setID)			{ assert(isSet(setID)); return _sets[setID];	} //FIXME check why cannot be const?

		bool				isManagingSymbol(uint n) 	const	{ return symbols.size()>n; }
		unsigned int		nbManagedSymbols()			const	{ return symbols.size(); }
		PFSymbol*			getManagedSymbol(uint n)	const	{ assert(isManagingSymbol(n)); return symbols[n].symbol;	}
		const Tuple2Atom&	getTuples(uint n)			const	{ assert(isManagingSymbol(n)); return symbols[n].tuple2atom;	}

		std::string	printAtom(int nr, bool longnames = false)	const;
};

/**
 * Ground terms
 */
struct GroundTerm {
	bool _isvarid;
	union {
		const DomainElement*	_domelement;
		VarId					_varid;
	};
	GroundTerm() { }
	GroundTerm(const DomainElement* domel): _isvarid(false), _domelement(domel) { }
	GroundTerm(const VarId& varid): _isvarid(true), _varid(varid) { }
	friend bool operator==(const GroundTerm&, const GroundTerm&);
	friend bool operator<(const GroundTerm&, const GroundTerm&);
};

/**
 * Ground term translator.
 */
class GroundTermTranslator {
	private:
		AbstractStructure*	_structure;

		std::vector<std::map<std::vector<GroundTerm>,VarId> >	_functerm2varid_table;	//!< map function term to CP variable identifier
		std::vector<Function*>									_varid2function;		//!< map CP varid to the symbol of its corresponding term
		std::vector<std::vector<GroundTerm> >					_varid2args;			//!< map CP varid to the terms of its corresponding term
		
		std::vector<Function*>		_offset2function;
		std::map<Function*,size_t>	_function2offset;

		std::map<VarId,CPTsBody*>	_varid2cprelation;

		std::vector<SortTable*>		_varid2domain;

	public:
		GroundTermTranslator(AbstractStructure* str) : _structure(str), _varid2function(1), _varid2args(1), _varid2domain(1) { }

		// Methods for translating terms to variable identifiers
		VarId	translate(size_t offset, const std::vector<GroundTerm>&);
		VarId	translate(Function*, const std::vector<GroundTerm>&);
		VarId	translate(CPTerm*, SortTable*);
		VarId	translate(const DomainElement*);
		
		// Adding variable identifiers and functions
		size_t	nextNumber();
		size_t	addFunction(Function*);

		// Containment checking
		//bool	isInternalVarId(const VarId& varid)					const { return _function(varid) == 0; }

		// Methods for translating variable identifiers to terms
		Function*						function(const VarId& varid)	const { return _varid2function[varid];					}
		const std::vector<GroundTerm>&	args(const VarId& varid)		const { return _varid2args[varid];						}
		CPTsBody*						cprelation(const VarId& varid)	const { return _varid2cprelation.find(varid)->second;	}
		SortTable*						domain(const VarId& varid)		const { return _varid2domain[varid];					}

		size_t			nrOffsets()					const { return _offset2function.size();		}
		size_t			getOffset(Function* func)	const { return _function2offset.at(func);	}
		const Function*	getFunction(size_t offset)	const { return _offset2function[offset];	}

		// Debugging
		std::string		printTerm(const VarId&, bool longnames)		const;
};

/************************************
	Optimized grounding algorithm
************************************/

class TermGrounder;
class FormulaGrounder;
class SetGrounder;
class RuleGrounder;
class DefinitionGrounder;
struct GroundDefinition;

/** Grounding context **/
enum CompContext { CC_SENTENCE, CC_HEAD, CC_FORMULA };

struct GroundingContext {
	bool				_truegen;		// Indicates whether the variables are instantiated in order to obtain
										// a ground formula that is possibly true.
	PosContext			_funccontext;	
	PosContext			_monotone;
	CompContext			_component;		// Indicates the context of the visited formula
	TsType				_tseitin;		// Indicates the type of tseitin definition that needs to be used.
	std::set<PFSymbol*>	_defined;		// Indicates whether the visited rule is recursive.
};


/*** Top level grounders ***/

class TopLevelGrounder {
	protected:
		AbstractGroundTheory*	_grounding;
		int						_verbosity;
	public:
		TopLevelGrounder(AbstractGroundTheory* gt, int verb) : _grounding(gt), _verbosity(verb) { }
		virtual ~TopLevelGrounder() { }

		virtual bool					run()		const = 0;
				AbstractGroundTheory*	grounding()	const { return _grounding;	}
};

class CopyGrounder : public TopLevelGrounder {
	private:
		const AbstractGroundTheory*		_original;
	public:
		CopyGrounder(AbstractGroundTheory* gt, const AbstractGroundTheory* orig, int verb) : TopLevelGrounder(gt,verb), _original(orig) { }
		bool run() const;
};

class TheoryGrounder : public TopLevelGrounder {
	private:
		std::vector<TopLevelGrounder*>	_grounders;
	public:
		TheoryGrounder(AbstractGroundTheory* gt, const std::vector<TopLevelGrounder*>& fgs, int verb) :
			TopLevelGrounder(gt,verb), _grounders(fgs) { }
		bool run() const;
};

class SentenceGrounder : public TopLevelGrounder {
	private:
		bool				_conj;	
		FormulaGrounder*	_subgrounder;
	public:
		SentenceGrounder(AbstractGroundTheory* gt, FormulaGrounder* sub, bool conj, int verb) : 
			TopLevelGrounder(gt,verb), _conj(conj), _subgrounder(sub) { }
		bool run() const;
};

class UnivSentGrounder : public TopLevelGrounder {
	private:
		TopLevelGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		UnivSentGrounder(AbstractGroundTheory* gt, TopLevelGrounder* sub, InstGenerator* gen, int verb) : 
			TopLevelGrounder(gt,verb), _subgrounder(sub), _generator(gen) { }
		bool run() const;
};


/*** Term grounders ***/

class TermGrounder {
	protected:
		AbstractGroundTheory*						_grounding;
		mutable SortTable*							_domain;
		const Term*									_origterm;
		std::map<Variable*,const DomainElement**>	_varmap;
		int											_verbosity;
		void printOrig() const;
	public:
		TermGrounder() { }
		TermGrounder(AbstractGroundTheory* g, SortTable* dom) : _grounding(g), _domain(dom) { }
		virtual ~TermGrounder() { }
		virtual GroundTerm run() const = 0;
		void setOrig(const Term* t, const std::map<Variable*, const DomainElement**>& mvd, int); 
		SortTable* 	getDomain() const 			{ return _domain; }
		void		setDomain(SortTable* dom) 	{ _domain = dom; }
};

class DomTermGrounder : public TermGrounder {
	private:
		const DomainElement*	_value;
	public:
		DomTermGrounder(const DomainElement* val) : _value(val) { }
		GroundTerm run() const;
};

class VarTermGrounder : public TermGrounder {
	private:
		const DomainElement**	_value;
	public:
		VarTermGrounder(const DomainElement** a) : _value(a) { }
		GroundTerm run() const; 
};

class FuncTermGrounder : public TermGrounder {
	protected:
		GroundTermTranslator* 			_termtranslator;
		Function*						_function;
		FuncTable*						_functable;
		std::vector<TermGrounder*>		_subtermgrounders;
	public:
		FuncTermGrounder(GroundTermTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<TermGrounder*>& sub)
			: _termtranslator(tt), _function(func), _functable(ftable), _subtermgrounders(sub) { _domain = dom; }
		GroundTerm run() const;

		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to run().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

enum SumType { ST_PLUS, ST_MINUS };

class SumTermGrounder : public TermGrounder {
	protected:
		GroundTermTranslator* 	_termtranslator;
		FuncTable*				_functable;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		SumType					_type;
	public:
		SumTermGrounder(AbstractGroundTheory* g, GroundTermTranslator* tt, FuncTable* ftable,
			SortTable* dom, TermGrounder* ltg, TermGrounder* rtg, SumType type = ST_PLUS)
			: TermGrounder(g,dom), _termtranslator(tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg), _type(type) { }
		GroundTerm run() const;
};

class AggTermGrounder : public TermGrounder {
	private:
		GroundTranslator*	_translator;
		AggFunction			_type;
		SetGrounder*		_setgrounder;
	public:
		AggTermGrounder(GroundTranslator* gt, AggFunction tp, SetGrounder* gr)
			: _translator(gt), _type(tp), _setgrounder(gr) { }
		GroundTerm run() const;
};


/*** Formula grounders ***/

class FormulaGrounder {
	protected:
		GroundTranslator*							_translator;
		GroundingContext							_context;
		const Formula*								_origform;
		std::map<Variable*,const DomainElement**>	_varmap;
		int											_verbosity;
		void printOrig() const;
	public:
		FormulaGrounder(GroundTranslator* gt, const GroundingContext& ct): _translator(gt), _context(ct) { }
		virtual ~FormulaGrounder() { }
		virtual int		run()					const = 0;
		virtual void	run(std::vector<int>&)	const = 0;
		virtual bool	conjunctive()			const = 0;
		void setOrig(const Formula* f, const std::map<Variable*, const DomainElement**>& mvd, int);
};

class AtomGrounder : public FormulaGrounder {
	protected:
		std::vector<TermGrounder*>			_subtermgrounders;
		InstGenerator*						_pchecker;
		InstGenerator*						_cchecker;
		size_t								_symbol; // symbol's offset in translator's table.
		std::vector<SortTable*>				_tables;
		bool								_sign;
		int									_certainvalue;
		std::vector<const DomainElement**>	_checkargs;
		PredInter*							_inter;
	public:
		AtomGrounder(GroundTranslator*, bool sign, PFSymbol*,
					const std::vector<TermGrounder*>, const std::vector<const DomainElement**>&,
					InstGenerator*, InstGenerator*, PredInter*, const std::vector<SortTable*>&, const GroundingContext&);
		int		run() const;
		void	run(std::vector<int>&) const;
		bool	conjunctive() const { return true;	}
};

class ComparisonGrounder : public FormulaGrounder {
	private:
		GroundTermTranslator* 	_termtranslator;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		CompType				_comparator;
	public:
		ComparisonGrounder(GroundTranslator* gt, GroundTermTranslator* tt, TermGrounder* ltg, CompType comp, TermGrounder* rtg, const GroundingContext& gc)
			: FormulaGrounder(gt,gc), _termtranslator(tt), _lefttermgrounder(ltg), _righttermgrounder(rtg), _comparator(comp) { } 
		int		run() const;
		void	run(std::vector<int>&) const;
		bool	conjunctive() const { return true;	}
};

class AggGrounder : public FormulaGrounder {
	private:
		SetGrounder*	_setgrounder;
		TermGrounder*	_boundgrounder;
		AggFunction		_type;
		char			_comp;
		bool			_sign;
		bool			_doublenegtseitin;
		int	handleDoubleNegation(double boundvalue,int setnr) const;
		int	finishCard(double truevalue,double boundvalue,int setnr) 	const;
		int	finishSum(double truevalue,double boundvalue,int setnr)		const;
		int	finishProduct(double truevalue,double boundvalue,int setnr)	const;
		int	finishMaximum(double truevalue,double boundvalue,int setnr)	const;
		int	finishMinimum(double truevalue,double boundvalue,int setnr)	const;
		int finish(double boundvalue,double newboundvalue,double maxpossvalue,double minpossvalue,int setnr) const;
	public:
		AggGrounder(GroundTranslator* tr, GroundingContext gc, AggFunction tp, SetGrounder* sg, TermGrounder* bg, char c,bool s)
			: FormulaGrounder(tr,gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(c), _sign(s) { 
				_doublenegtseitin = (gc._tseitin == TS_RULE) && ((gc._monotone == PC_POSITIVE && !s) || (gc._monotone == PC_NEGATIVE && s));	
			}
		int		run()								const;
		void	run(std::vector<int>&)				const;
		bool	conjunctive() 						const { return true;	}
};

class ClauseGrounder : public FormulaGrounder {
	protected:
		bool	_sign;
		bool	_conj;
		bool	_doublenegtseitin;
	public:
		ClauseGrounder(GroundTranslator* gt, bool sign, bool conj, const GroundingContext& ct) : 
			FormulaGrounder(gt,ct), _sign(sign), _conj(conj) { 
				_doublenegtseitin = ct._tseitin == TS_RULE && ((ct._monotone == PC_POSITIVE && !sign) || (ct._monotone == PC_NEGATIVE && sign));	
			}
		int		finish(std::vector<int>&) const;
		bool	check1(int l) const;
		bool	check2(int l) const;
		int		result1() const;
		int		result2() const;
		bool	conjunctive() const { return _conj == _sign;	}
};

class BoolGrounder : public ClauseGrounder {
	private:
		std::vector<FormulaGrounder*>	_subgrounders;
	public:
		BoolGrounder(GroundTranslator* gt, const std::vector<FormulaGrounder*> sub, bool sign, bool conj, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounders(sub) { }
		int	run() const;
		void	run(std::vector<int>&) const;
};

class QuantGrounder : public ClauseGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
		InstGenerator*		_checker;
	public:
		QuantGrounder(GroundTranslator* gt, FormulaGrounder* sub, bool sign, bool conj, InstGenerator* gen, InstGenerator* check, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounder(sub), _generator(gen), _checker(check) { }
		int		run() const;
		void	run(std::vector<int>&) const;
};

class EquivGrounder : public FormulaGrounder {
	private:
		FormulaGrounder*	_leftgrounder;
		FormulaGrounder*	_rightgrounder;
		bool				_sign;
	public:
		EquivGrounder(GroundTranslator* gt, FormulaGrounder* lg, FormulaGrounder* rg, bool sign, const GroundingContext& ct):
			FormulaGrounder(gt,ct), _leftgrounder(lg), _rightgrounder(rg), _sign(sign) { }
		int 	run() const;
		void	run(std::vector<int>&) const;
		bool	conjunctive() const { return true;	}
};


/*** Set grounders ***/

class SetGrounder {
	protected:
		GroundTranslator*	_translator;
	public:
		SetGrounder(GroundTranslator* gt) : _translator(gt) { }
		virtual ~SetGrounder() { }
		virtual int run() const = 0;
};

class QuantSetGrounder : public SetGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
		InstGenerator*		_checker;	
		TermGrounder*		_weightgrounder;
	public:
		QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, InstGenerator* check, TermGrounder* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _checker(check), _weightgrounder(w) { }
		int run() const;
};

class EnumSetGrounder : public SetGrounder {
	private:
		std::vector<FormulaGrounder*>	_subgrounders;
		std::vector<TermGrounder*>		_subtermgrounders;
	public:
		EnumSetGrounder(GroundTranslator* gt, const std::vector<FormulaGrounder*>& subgr, const std::vector<TermGrounder*>& subtgr) :
			SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) { }
		int run() const;
};


/*** Definition grounders ***/

/** Grounder for a head of a rule **/
class HeadGrounder {
	private:
		AbstractGroundTheory*			_grounding;
		std::vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*				_truechecker;
		InstanceChecker*				_falsechecker;
		unsigned int					_symbol;
		std::vector<SortTable*>			_tables;
	public:
		HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
					const std::vector<TermGrounder*>&, const std::vector<SortTable*>&);
		int	run() const;

};

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;	
		InstGenerator*		_bodygenerator;	
		GroundingContext	_context;
	public:
		RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
			: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) { }
		bool run(unsigned int defid, GroundDefinition* grounddefinition) const;

		// Mutators
		void addTrueRule(GroundDefinition* grounddefinition, int head) const;
		void addFalseRule(GroundDefinition* grounddefinition, int head) const;
		void addPCRule(GroundDefinition* grounddefinition, int head, const std::vector<int>& body, bool conj, bool recursive) const;
		void addAggRule(GroundDefinition* grounddefinition, int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) const;
};

/** Grounder for a definition **/
// NOTE: definition printing code is based on the INVARIANT that a defintion is ALWAYS grounded as contiguous component: never ground def A a bit, then ground B, then return to A again (code should error on this)
// directly printing in idp language will be incorrect then.
// TODO optimize grounding of definitions by grouping rules with the same head and grounding them atom by atom
//	(using approximation to derive given a head query which bodies might be true). This would allow to write out rules without
//	first constructing and storing the full ground definition to remove duplicate heads
class DefinitionGrounder : public TopLevelGrounder {
	private:
		static unsigned int _currentdefnb;
		unsigned int _defnb;
		std::vector<RuleGrounder*>	_subgrounders;	//!< Grounders for the rules of the definition.
		GroundDefinition* _grounddefinition;
	public:
		DefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<RuleGrounder*> subgr,int verb);
		bool run() const;
		unsigned int id() const { return _defnb; }
};


/***********************
	Grounder Factory
***********************/

/*
 * Class to produce grounders 
 */

class InteractivePrintMonitor;
class SymbolicStructure;

class GrounderFactory : public TheoryVisitor {
	private:
		// Data
		AbstractStructure*		_structure;		//!< The structure that will be used to reduce the grounding
		SymbolicStructure*		_symstructure;	//!< Used approximation
		AbstractGroundTheory*	_grounding;		//!< The ground theory that will be produced

		// Options
		Options*	_options;
		int			_verbosity;
		bool		_cpsupport;
		bool		_longnames;

		// Context
		GroundingContext				_context;
		std::stack<GroundingContext>	_contextstack;

		void	InitContext();		// Initialize the context 
		void	AggContext();	
		void	SaveContext();		// Push the current context onto the stack
		void	RestoreContext();	// Set _context to the top of the stack and pop the stack
		void	DeeperContext(bool);

		// Descend in the parse tree while taking care of the context
		void	descend(Formula* f); 
		void	descend(Term* t);
		void	descend(Rule* r);
		void	descend(SetExpr* s);
		
		// Symbols passed to CP solver
		std::set<const PFSymbol*>	_cpsymbols;

		// Variable mapping
		std::map<Variable*,const DomainElement**>	_varmapping;	// Maps variables to their counterpart during grounding.
														// That is, the corresponding const DomainElement** acts as a variable+value.

		// Return values
		FormulaGrounder*		_formgrounder;
		TermGrounder*			_termgrounder;
		SetGrounder*			_setgrounder;
		TopLevelGrounder*		_toplevelgrounder;
		HeadGrounder*			_headgrounder;
		RuleGrounder*			_rulegrounder;

		const FOBDD*	improve_generator(const FOBDD*, const std::vector<Variable*>&, double);
		const FOBDD*	improve_checker(const FOBDD*, double);

	public:
		// Constructor
		GrounderFactory(AbstractStructure* structure, Options* opts, SymbolicStructure* s = 0);

		// Factory method
		TopLevelGrounder* create(const AbstractTheory*);
		TopLevelGrounder* create(const AbstractTheory*, MinisatID::WrappedPCSolver*);
		TopLevelGrounder* create(const AbstractTheory* theory, InteractivePrintMonitor* monitor, Options* opts);

		// Determine what should be passed to CP solver
		std::set<const PFSymbol*> 	findCPSymbols(const AbstractTheory*);
		bool 						isCPSymbol(const PFSymbol*) const;

		// Recursive check
		bool recursive(const Formula*);

		// Visitors
		void visit(const AbstractGroundTheory*);
		void visit(const Theory*);

		void visit(const PredForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const EquivForm*);
		void visit(const EqChainForm*);
		void visit(const AggForm*);

		void visit(const VarTerm*);
		void visit(const DomainTerm*);
		void visit(const FuncTerm*);
		void visit(const AggTerm*);

		void visit(const EnumSetExpr*);
		void visit(const QuantSetExpr*);

		void visit(const Definition*);
		void visit(const Rule*);
};

#endif
