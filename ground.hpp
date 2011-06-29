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
class StrictWeakTupleOrdering;

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

/*
 * A complete definition of a tseitin atom
*/
class TsBody {
	protected:
		TsType _type;	// the type of "tseitin definition"
		TsBody(TsType type): _type(type) { }
	public:
		virtual ~TsBody() { }
		TsType type() const { return _type; }
	friend class GroundTranslator;
};

class PCTsBody : public TsBody {
	private:
		std::vector<int> 	_body;	// the literals in the subformula replaced by the tseitin
		bool				_conj;	// if true, the replaced subformula is the conjunction of the literals in _body,
									// if false, the replaced subformula is the disjunction of the literals in _body
	public:
		PCTsBody(TsType type, const std::vector<int>& body, bool conj):
			TsBody(type), _body(body), _conj(conj) { }
		std::vector<int> 	body() 					const { return _body; 			}
		unsigned int		size()					const { return _body.size();	}
		int					literal(unsigned int n)	const { return _body[n];		}
		bool				conj()					const { return _conj; 			}
	friend class GroundTranslator;
};

class AggTsBody : public TsBody {
	private:
		int			_setnr;
		AggFunction		_aggtype;
		bool		_lower;
		double		_bound;
	public:
		AggTsBody(TsType type, double bound, bool lower, AggFunction at, int setnr):
			TsBody(type), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		int			setnr()		const { return _setnr; 		}
		AggFunction		aggtype()	const { return _aggtype;	}
		bool		lower()		const { return _lower;		}
		double		bound()		const { return _bound;		}
	friend class GroundTranslator;
};

class CPTerm {
	protected:
		virtual ~CPTerm() {	}
	public:
		virtual void accept(TheoryVisitor*) const = 0;
};

class CPVarTerm : public CPTerm {
	public:
		unsigned int _varid;
		CPVarTerm(unsigned int varid) : _varid(varid) { }
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

class CPSumTerm : public CPTerm {
	public:
		std::vector<unsigned int> _varids; 
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

class CPWSumTerm : public CPTerm {
	public:
		std::vector<unsigned int> 	_varids; 
		std::vector<int>			_weights;
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

struct CPBound {
	bool _isvarid;
	union { 
		int _bound;
		unsigned int _varid;
	} _value;
	CPBound(bool isvarid, int bound): _isvarid(isvarid) { _value._bound = bound; }
	CPBound(bool isvarid, unsigned int varid): _isvarid(isvarid) { _value._varid = varid; }
};

class CPTsBody : public TsBody {
	private:
		CPTerm*		_left;
		CompType	_comp;
		CPBound		_right;
	public:
		CPTsBody(TsType type, CPTerm* left, CompType comp, const CPBound& right) :
			TsBody(type), _left(left), _comp(comp), _right(right) { }
		CPTerm*		left()	const { return _left;	}
		CompType	comp()	const { return _comp;	}
		CPBound		right()	const { return _right;	}
	friend class GroundTranslator;
};

/*
 * Set corresponding to a tseitin
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

/*
 * Ground translator 
 */
class GroundTranslator {
	private:
		std::vector<std::map<std::vector<const DomainElement*>,int,StrictWeakTupleOrdering> >	_table;			// map atoms to integers
		std::vector<PFSymbol*>								_symboffsets;	// map integer to symbol
		std::vector<PFSymbol*>								_backsymbtable;	// map integer to the symbol of its corresponding atom
		std::vector<std::vector<const DomainElement*> >				_backargstable;	// map integer to the terms of its corresponding atom

		std::queue<int>			_freenumbers;		// keeps atom numbers that were freed 
													// and can be used again
		std::queue<int>			_freesetnumbers;	// keeps set numbers that were freed
													// and can be used again

		std::map<int,TsBody*>	_tsbodies;	// keeps mapping between Tseitin numbers and bodies

		std::vector<TsSet>		_sets;		// keeps mapping between Set numbers and sets

	public:
		GroundTranslator() : _backsymbtable(1), _backargstable(1), _sets(1) { }

		int				translate(unsigned int,const std::vector<const DomainElement*>&);
		int				translate(const std::vector<int>& cl, bool conj, TsType tp);
		int				translate(double bound, char comp, bool strict, AggFunction aggtype, int setnr, TsType tstype);
		int				translate(PFSymbol*,const std::vector<const DomainElement*>&);
		int				translate(CPTerm*, CompType, const CPBound&, TsType);
		int				nextNumber();
		unsigned int	addSymbol(PFSymbol* pfs);
		int				translateSet(const std::vector<int>&,const std::vector<double>&,const std::vector<double>&);

		bool										isSymbol(int nr)			const	{ return 0<nr && nr<_backsymbtable.size(); }
		PFSymbol*									symbol(int nr)				const	{ return _backsymbtable[abs(nr)];		}
		const std::vector<const DomainElement*>&	args(int nr)				const	{ return _backargstable[abs(nr)];		}
		bool										isTseitin(int l)			const	{ return symbol(l) == 0;				}
		TsBody*										tsbody(int l)				const	{ return _tsbodies.find(abs(l))->second;}
		const TsSet&								groundset(int nr)			const	{ return _sets[nr];						}
		TsSet&										groundset(int nr)					{ return _sets[nr];						}
		unsigned int								nrOffsets()					const	{ return _symboffsets.size();			}
		PFSymbol*									getSymbol(unsigned int n)	const	{ return _symboffsets[n];				}
		const std::map<std::vector<const DomainElement*>,int,StrictWeakTupleOrdering>&	getTuples(unsigned int n)	const	{ return _table[n];						}

		std::string	printAtom(int nr)	const;
};

/*
 * Ground term translator
 */
class GroundTermTranslator {
	private:
		std::vector<std::map<std::vector<const DomainElement*>,unsigned int,StrictWeakTupleOrdering> >	_table;			// map terms to integers
		std::vector<Function*>											_backfunctable;	// map integer to the symbol of its corresponding term
		std::vector<std::vector<const DomainElement*> >							_backargstable;	// map integer to the terms of its corresponding term
		
		std::vector<Function*>				_offset2function;
		std::map<Function*,unsigned int>	_function2offset;

	public:
		GroundTermTranslator() : _backfunctable(1), _backargstable(1) { }

		unsigned int	translate(unsigned int offset,const std::vector<const DomainElement*>& args);
		unsigned int	translate(Function*,const std::vector<const DomainElement*>& args);
		unsigned int	nextNumber();
		unsigned int	addFunction(Function*);

		Function*						function(unsigned int nr)		const { return _backfunctable[nr];		}
		const std::vector<const DomainElement*>&	args(unsigned int nr)			const { return _backargstable[nr];		}
		unsigned int					nrOffsets()						const { return _offset2function.size();	}
		Function*						getFunction(unsigned int nr)	const { return _offset2function[nr]; 	}
		std::string						printTerm(unsigned int nr)		const;
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
		const GroundTheory*		_original;
	public:
		CopyGrounder(AbstractGroundTheory* gt, const GroundTheory* orig, int verb) : TopLevelGrounder(gt,verb), _original(orig) { }
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
		const Term*									_origterm;
		std::map<Variable*,const DomainElement**>	_varmap;
		int											_verbosity;
		void printorig() const;
	public:
		TermGrounder() { }
		virtual ~TermGrounder() { }
		virtual const DomainElement* run() const = 0;
		virtual bool canReturnCPVar() const = 0;
		void setorig(const Term* t, const std::map<Variable*,const DomainElement**>& mvd,int); 
};

class DomTermGrounder : public TermGrounder {
	private:
		const DomainElement*	_value;
	public:
		DomTermGrounder(const DomainElement* val) : _value(val) { }
		const DomainElement* run() const { return _value;	}
		bool canReturnCPVar() const { return false; }
};

class VarTermGrounder : public TermGrounder {
	private:
		const DomainElement**	_value;
	public:
		VarTermGrounder(const DomainElement** a) : _value(a) { }
		const DomainElement* run() const; 
		bool canReturnCPVar() const { return false; }
};

class FuncTermGrounder : public TermGrounder {
	private:
		FuncTable*						_function;
		std::vector<TermGrounder*>		_subtermgrounders;
		mutable std::vector<const DomainElement*>	_args;
	public:
		FuncTermGrounder(const std::vector<TermGrounder*>& sub, FuncTable* f) :
			_function(f), _subtermgrounders(sub), _args(sub.size()) { }
		const DomainElement* run() const;
		bool canReturnCPVar() const { return false; }

		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to calc().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

class AggTermGrounder : public TermGrounder {
	private:
		AggFunction				_type;
		SetGrounder*		_setgrounder;
		GroundTranslator*	_translator;
	public:
		AggTermGrounder(GroundTranslator* gt, AggFunction tp, SetGrounder* gr):
			_type(tp), _setgrounder(gr), _translator(gt) { }
		const DomainElement* run() const;
		bool canReturnCPVar() const { return false; }
};

/*** Three-valued term grounders ***/

class ThreeValuedFuncTermGrounder : public TermGrounder {
	private:
		std::vector<TermGrounder*>		_subtermgrounders;
		Function*						_function;
		FuncTable*						_functable;
		mutable std::vector<const DomainElement*>	_args;
		std::vector<SortTable*>			_tables;
	public:
		ThreeValuedFuncTermGrounder(const std::vector<TermGrounder*>& sub, Function* f, FuncTable* ft, const std::vector<SortTable*>& vst):
			_subtermgrounders(sub), _function(f), _functable(ft), _args(sub.size()), _tables(vst) { }
		const DomainElement* run() const;
		bool canReturnCPVar() const { return true; }
};

class ThreeValuedAggTermGrounder : public TermGrounder {
	private:
		AggFunction				_type;
		SetGrounder*		_setgrounder;
		GroundTranslator*	_translator;
	public:
		ThreeValuedAggTermGrounder(GroundTranslator* gt, AggFunction tp, SetGrounder* gr):
			_type(tp), _setgrounder(gr), _translator(gt) { } 
		const DomainElement* run() const;
		bool canReturnCPVar() const { return true; }
};

/*** Formula grounders ***/

class FormulaGrounder {
	protected:
		const Formula*								_origform;
		std::map<Variable*,const DomainElement**>	_varmap;
		int											_verbosity;
		void printorig() const;
		GroundTranslator*	_translator;
		GroundingContext	_context;
	public:
		FormulaGrounder(GroundTranslator* gt, const GroundingContext& ct): _translator(gt), _context(ct) { }
		virtual ~FormulaGrounder() { }
		virtual int		run()					const = 0;
		virtual void	run(std::vector<int>&)	const = 0;
		virtual bool	conjunctive()			const = 0;
		void setorig(const Formula* f, const std::map<Variable*,const DomainElement**>& mvd,int);
};

class AtomGrounder : public FormulaGrounder {
	private:
		std::vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*				_pchecker;
		InstanceChecker*				_cchecker;
		unsigned int					_symbol;
		mutable std::vector<const DomainElement*>	_args;
		std::vector<SortTable*>			_tables;
		bool							_sign;
		int								_certainvalue;
	public:
		AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
					const std::vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
					const std::vector<SortTable*>& vst, const GroundingContext&);
		int		run() const;
		void	run(std::vector<int>&) const;
		bool	conjunctive() const { return true;	}
};

class CPGrounder : public FormulaGrounder {
	private:
		GroundTermTranslator* 	_termtranslator;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		CompType				_comparator;
	public:
		CPGrounder(GroundTranslator* gt, GroundTermTranslator* tt, TermGrounder* left, CompType comp, TermGrounder* right, const GroundingContext& gc):
			FormulaGrounder(gt,gc), _termtranslator(tt), _lefttermgrounder(left), _righttermgrounder(right), _comparator(comp) { } 
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
		AggGrounder(GroundTranslator* tr, GroundingContext gc, AggFunction tp, SetGrounder* sg, TermGrounder* bg, char c,bool s) :
			FormulaGrounder(tr,gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(c), _sign(s) { 
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
	public:
		QuantGrounder(GroundTranslator* gt, FormulaGrounder* sub, bool sign, bool conj, InstGenerator* gen, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounder(sub), _generator(gen) { }
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
		TermGrounder*		_weightgrounder;
	public:
		QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, TermGrounder* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _weightgrounder(w) { }
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
		mutable std::vector<const DomainElement*>	_args;
		std::vector<SortTable*>			_tables;
	public:
		HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
					const std::vector<TermGrounder*>&, const std::vector<SortTable*>&);
		int	run() const;

};

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		GroundDefinition*	_definition;
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;	
		InstGenerator*		_bodygenerator;	
		GroundingContext	_context;
	public:
		RuleGrounder(GroundDefinition* def, HeadGrounder* hgr, FormulaGrounder* bgr,
					InstGenerator* hig, InstGenerator* big, GroundingContext& ct) :
			_definition(def), _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig),
			_bodygenerator(big), _context(ct) { }
		bool run() const;
};

/** Grounder for a definition **/
class DefinitionGrounder : public TopLevelGrounder {
	private:
		GroundDefinition*			_definition;	// The ground definition that will be produced by running the grounder.
		std::vector<RuleGrounder*>	_subgrounders;	// Grounders for the rules of the definition.
	public:
		DefinitionGrounder(AbstractGroundTheory* gt, GroundDefinition* def, std::vector<RuleGrounder*> subgr,int verb) :
			TopLevelGrounder(gt,verb), _definition(def), _subgrounders(subgr) { }
		bool run() const;
};


/***********************
	Grounder Factory
***********************/

/*
 * Class to produce grounders 
 */
class GrounderFactory : public TheoryVisitor {
	private:
		// Data
		Options*				_options;
		AbstractStructure*		_structure;		// The structure that will be used to reduce the grounding
		AbstractGroundTheory*	_grounding;		// The ground theory that will be produced

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
		
		// Grounding to CP
		std::set<const Function*>	_cpfunctions;

		// Variable mapping
		std::map<Variable*,const DomainElement**>	_varmapping;	// Maps variables to their counterpart during grounding.
														// That is, the corresponding const DomainElement** acts as a variable+value.

		// Current ground definition
		GroundDefinition*		_definition;	// The ground definition that will be produced by the 
												// currently constructed definition grounder.

		// Return values
		FormulaGrounder*		_formgrounder;
		TermGrounder*			_termgrounder;
		SetGrounder*			_setgrounder;
		TopLevelGrounder*		_toplevelgrounder;
		HeadGrounder*			_headgrounder;
		RuleGrounder*			_rulegrounder;

	public:
		// Constructor
		GrounderFactory(AbstractStructure* structure, Options* opts): _options(opts), _structure(structure) { }

		// Factory method
		TopLevelGrounder* create(const AbstractTheory*);
		TopLevelGrounder* create(const AbstractTheory*, MinisatID::WrappedPCSolver*);

		// Determine what should be grounded to CP
		std::set<const Function*> findCPFunctions(const AbstractTheory*);

		// Recursive check
		bool recursive(const Formula*);

		// Visitors
		void visit(const GroundTheory*);
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
