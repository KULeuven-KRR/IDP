/************************************
	ecnf.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ECNF_H
#define ECNF_H

#include "theory.hpp"
#include "ground.hpp"

namespace MinisatID{
 	 class WrappedPCSolver;
}
typedef MinisatID::WrappedPCSolver SATSolver;


/*********************
	Ground clauses
*********************/

typedef vector<int> GroundClause;

/****************** 
	Ground sets 
******************/

struct GroundSet {
	// Attributes
	int				_setnr;
	vector<int>		_setlits;		// All literals in the ground set
	vector<double>	_litweights;	// For each literal a corresponding weight

	// Constructors
	GroundSet() { }
	GroundSet(int setnr, const vector<int>& s, const vector<double>& lw) :
		_setnr(setnr), _setlits(s), _litweights(lw) { }
};


/********************************
	Ground aggregate formulas
********************************/

/*
 * struct GroundAggregate
 *		This class represents ground formulas of the form
 *			head ARROW bound COMP agg(set)
 *		where 
 *			head is a literal
 *			ARROW is <=, =>, or <=>
 *			bound is an integer or floating point number
 *			COMP is either =< or >=
 *			agg is an aggregate function
 *			set is a ground set
 */
struct GroundAggregate {
	// Attributes
	int			_head;		// the head literal
	TsType		_arrow;		// the relation between head and aggregate expression
							// INVARIANT: this should never be TS_RULE
	double		_bound;		// the bound
	bool		_lower;		// true iff the bound is a lower bound for the aggregate
	AggType		_type;		// the aggregate function
	int			_set;		// the set id
	// Constructors
	GroundAggregate(AggType t, bool l, TsType e, int h, int s, double b) :
		_head(h), _arrow(e), _bound(b), _lower(l), _type(t), _set(s) { }
	GroundAggregate(const GroundAggregate& a) : 
		_head(a._head), _arrow(a._arrow), _bound(a._bound), _lower(a._lower), _type(a._type), _set(a._set) { }
	GroundAggregate() { }
};


/*************************
	Ground definitions
*************************/

// Enumeration used in GroundTheory::transformForAdd 
enum VIType { VIT_DISJ, VIT_CONJ, VIT_SET };

// Enumeration type for rules
// RM enum RuleType { RT_TRUE, RT_FALSE, RT_UNARY, RT_CONJ, RT_DISJ, RT_AGG };
enum RuleType { RT_CONJ, RT_DISJ, RT_AGG };

/*
 * class GroundRuleBody
 *		This class represents a ground rule body, where literals are represented by integers.
 */ 
class GroundRuleBody {
	public:
		// Attributes
		RuleType	_type;					// The rule type (disjunction, conjunction, or aggregate
		bool		_recursive;				// True iff the rule body contains defined literals
		// Constructors
		GroundRuleBody() { }
		GroundRuleBody(RuleType type, bool rec): _type(type), _recursive(rec) { }
		// Destructor
		virtual		~GroundRuleBody() { }
		// Inspectors
		virtual	bool	isFalse()	const = 0;
		virtual	bool	isTrue()	const = 0;
};

/*
 * class PCGroundRuleBody
 *		This class represents ground rule bodies that are conjunctions or disjunctions of literals.
 */
class PCGroundRuleBody : public GroundRuleBody {
	public:
		// Attributes
		vector<int>	_body;	// The literals in the body
		// Constructors
		PCGroundRuleBody(RuleType type, const vector<int>& body, bool rec) : GroundRuleBody(type,rec), _body(body) { }
		PCGroundRuleBody(const PCGroundRuleBody& grb): GroundRuleBody(grb._type,grb._recursive), _body(grb._body) { }
		// Inspectors
		bool	isFalse()	const { return (_body.empty() && _type == RT_DISJ);	}
		bool	isTrue()	const { return (_body.empty() && _type == RT_CONJ);	}
};

/*
 * class AggGroundRuleBody
 *		This class represents ground rule bodies that are aggregates.
 */
class AggGroundRuleBody : public GroundRuleBody {

	public:

		// Attributes
		int		_setnr;		// The id of the set of the aggregate
		AggType _aggtype;	// The aggregate type (cardinality, sum, product, min, or max)
		bool	_lower;		// True iff the bound is a lower bound
		double	_bound;		// The bound on the aggregate

		// Constructors
		AggGroundRuleBody(int setnr, AggType at, bool lower, double bound, bool rec):
			GroundRuleBody(RT_AGG,rec), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		AggGroundRuleBody(const AggGroundRuleBody& grb):
			GroundRuleBody(RT_AGG,grb._recursive), _setnr(grb._setnr), _aggtype(grb._aggtype), _lower(grb._lower), _bound(grb._bound) { }
		// Inspectors
		bool	isFalse()	const { return false;	}
		bool	isTrue()	const { return false;	}
};

/*
 * struct GroundDefinition
 *		This class represents ground definitions.
 */
struct GroundDefinition {
	// Attributes
	GroundTranslator*			_translator;
	map<int,GroundRuleBody*>	_rules;			// Maps a head to its corresponding body

	// Constructors
	GroundDefinition(GroundTranslator* tr) : _translator(tr) { }

	// Mutators
	void addTrueRule(int head);
	void addFalseRule(int head);
	void addPCRule(int head, const vector<int>& body, bool conj, bool recursive);
	void addAggRule(int head, int setnr, AggType aggtype, bool lower, double bound, bool recursive);

	// Debugging
	string to_string() const;
};


/****************************************
	Ground nested fixpoint defintions 
****************************************/

struct GroundFixpDef {
	// Attributes
	GroundDefinition		_rules;		// the direct subrules
	vector<GroundFixpDef>	_subdefs;	// the direct subdefinitions
};


/**********************
	Ground theories
**********************/

/*
 * class GroundTheory
 *		Implements ground theories
 */
class AbstractGroundTheory : public AbstractTheory {

	protected:
		AbstractStructure*			_structure;		// The ground theory may be partially reduced with respect
													// to this structure. 
		GroundTranslator*			_translator;	// Link between ground atoms and SAT-solver literals

		set<int>					_printedtseitins;	// Tseitin atoms produced by the translator that occur 
														// in the theory.
		set<int>					_printedsets;		// Set numbers produced by the translator that occur in the theory

		const 	GroundTranslator& getTranslator() const	{ return *_translator; }
				GroundTranslator& getTranslator() 		{ return *_translator; }

	public:
		// Constructors 
		AbstractGroundTheory(AbstractStructure* str) : 
			AbstractTheory("",ParseInfo()), _structure(str), _translator(new GroundTranslator()) { }
		AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) : 
			AbstractTheory("",voc,ParseInfo()), _structure(str), _translator(new GroundTranslator()) { }

		// Destructor
		virtual void recursiveDelete()	{ delete(this);			}
		virtual	~AbstractGroundTheory()	{ delete(_translator);	}

		// Mutators
				void add(Formula* )		{ assert(false);	}
				void add(Definition* )	{ assert(false);	}
				void add(FixpDef* )		{ assert(false);	}

				void transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst = false);

		virtual void addClause(GroundClause& cl, bool skipfirst = false)	= 0;
		virtual void addDefinition(GroundDefinition&)						= 0;
		virtual void addFixpDef(GroundFixpDef&)								= 0;
		virtual void addSet(int setnr, int defnr, bool weighted)			= 0;

				void addEmptyClause()		{ GroundClause c(0); addClause(c);		}
				void addUnitClause(int l)	{ GroundClause c(1,l); addClause(c);	}

		virtual	void addAggregate(int tseitin, AggTsBody* body)			= 0; 
		virtual void addPCRule(int defnr, int tseitin, PCTsBody* body)		= 0; 
		virtual void addAggRule(int defnr, int tseitin, AggTsBody* body)	= 0; 

		// Inspectors
		GroundTranslator*		translator()	const { return _translator;			}
		AbstractStructure*		structure()		const { return _structure;			}
		AbstractGroundTheory*	clone()			const { assert(false); /* TODO */	}
};

/* 
 * class SolverTheory 
 *		A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
class SolverTheory : public AbstractGroundTheory {

	private:
		SATSolver*					_solver;	// The SAT solver
		map<PFSymbol*,set<int> >	_defined;	// Symbols that are defined in the theory. This set is used to
												// communicate to the solver which ground atoms should be considered defined.
		const 	SATSolver& getSolver() const	{ return *_solver; }
				SATSolver& getSolver() 			{ return *_solver; }

		void 	addAggregate(int definitionID, int head, bool lowerbound, int setnr, AggType aggtype, TsType sem, double bound);
		void 	addPCRule(int defnr, int head, vector<int> body, bool conjunctive);

	public:
		// Constructors 
		SolverTheory(SATSolver* solver,AbstractStructure* str);
		SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str);

		// Mutators
		void	addClause(GroundClause& cl, bool skipfirst = false);
		void	addDefinition(GroundDefinition&);
		void	addFixpDef(GroundFixpDef&);
		void	addSet(int setnr, int defnr, bool weighted);

		void	addAggregate(int tseitin, AggTsBody* body);
		void	addPCRule(int defnr, int tseitin, PCTsBody* body);
		void	addAggRule(int defnr, int tseitin, AggTsBody* body);
		void	addPCRule(int defnr, int head, PCGroundRuleBody* body);
		void	addAggRule(int defnr, int head, AggGroundRuleBody* body);

		void	addFuncConstraints();
		void	addFalseDefineds();

		// Inspectors
		unsigned int	nrSentences()				const { assert(false); /*TODO*/	}
		unsigned int	nrDefinitions()				const { assert(false); /*TODO*/	}
		unsigned int	nrFixpDefs()				const { assert(false); /*TODO*/	}
		Formula*		sentence(unsigned int )		const { assert(false); /*TODO*/	}
		Definition*		definition(unsigned int )	const { assert(false); /*TODO*/	}
		FixpDef*		fixpdef(unsigned int )		const { assert(false); /*TODO*/	}

		// Visitor
		void			accept(Visitor* v) const	{ v->visit(this);			}
		AbstractTheory*	accept(MutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		string to_string() const { assert(false); /*TODO*/	}

};

/*
 * class GroundTheory
 *		This class implements ground theories
 */
class GroundTheory : public AbstractGroundTheory {
	
	private:
		vector<GroundClause>		_clauses;	
		vector<GroundDefinition>	_definitions;
		vector<GroundAggregate>		_aggregates;
		vector<GroundFixpDef>		_fixpdefs;
		vector<GroundSet>			_sets;

	public:

		// Constructor
		GroundTheory(AbstractStructure* str) : AbstractGroundTheory(str) { }
		GroundTheory(Vocabulary* voc, AbstractStructure* str) : AbstractGroundTheory(voc,str)	{ }

		// Mutators
		void	addClause(GroundClause& cl, bool skipfirst = false);
		void	addDefinition(GroundDefinition&);
		void	addFixpDef(GroundFixpDef&);
		void	addSet(int setnr, int defnr, bool weighted);

		void	addAggregate(int head, AggTsBody* body);
		void	addPCRule(int defnr, int tseitin, PCTsBody* body);
		void	addAggRule(int defnr, int tseitin, AggTsBody* body);

		// Inspectors
		unsigned int		nrSentences()				const { return _clauses.size() + _aggregates.size();	}
		unsigned int		nrDefinitions()				const { return _definitions.size();						}
		unsigned int		nrFixpDefs()				const { return _fixpdefs.size();						}
		Formula*			sentence(unsigned int n)	const;
		Definition*			definition(unsigned int n)	const;
		FixpDef*			fixpdef(unsigned int n)		const;

		// Visitor
		void			accept(Visitor* v) const	{ v->visit(this);			}
		AbstractTheory*	accept(MutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		string to_string() const;

};

#endif
