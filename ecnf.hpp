/************************************
	ecnf.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ECNF_HPP
#define ECNF_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <ostream>

#include "theory.hpp"
#include "commontypes.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

class GroundTranslator;
class GroundTermTranslator;
class PCTsBody;
class AggTsBody;
class CPTsBody;
class SortTable;

typedef unsigned int VarId;

namespace MinisatID{
 	 class WrappedPCSolver;
}
typedef MinisatID::WrappedPCSolver SATSolver;

// Enumeration used in GroundTheory::transformForAdd 
enum VIType { VIT_DISJ, VIT_CONJ, VIT_SET };


/*********************
	Ground clauses
*********************/

typedef std::vector<int> GroundClause;

/****************** 
	Ground sets 
******************/

/**
 * class GroundSet
 */
class GroundSet {
	private:
		unsigned int		_setnr;
		std::vector<int>	_setlits;		// All literals in the ground set
		std::vector<double>	_litweights;	// For each literal a corresponding weight

	public:
		// Constructors
		GroundSet() { }
		GroundSet(int setnr, const std::vector<int>& s, const std::vector<double>& lw) :
			_setnr(setnr), _setlits(s), _litweights(lw) { }

		// Inspectors
		unsigned int	setnr() 				const { return _setnr; 			}
		unsigned int	size()					const { return _setlits.size();	}
		int				literal(unsigned int n)	const { return _setlits[n];		}
		double			weight(unsigned int n)	const { return (not _litweights.empty()) ? _litweights[n] : 1;	}
		bool			weighted()				const { return not _litweights.empty(); }

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

/********************************
	Ground aggregate formulas
********************************/

/**
 * class GroundAggregate
 *		This class represents ground formulas of the form
 *			head ARROW bound COMP agg(set)
 *		where 
 *			head is a literal
 *			ARROW is <=, =>, or <=>
 *			bound is an integer or floating point number
 *			COMP is either =< or >=
 *			type is an aggregate function
 *			set is a ground set
 */
class GroundAggregate {
	private:
		// Attributes
		int			_head;		// the head literal
		TsType		_arrow;		// the relation between head and aggregate expression
								// INVARIANT: this should never be TS_RULE
		double		_bound;		// the bound
		bool		_lower;		// true iff the bound is a lower bound for the aggregate
		AggFunction	_type;		// the aggregate function
		int			_set;		// the set id

	public:
		// Constructors
		GroundAggregate(AggFunction t, bool l, TsType e, int h, int s, double b) :
			_head(h), _arrow(e), _bound(b), _lower(l), _type(t), _set(s) 
			{ assert(e != TS_RULE); }
		GroundAggregate(const GroundAggregate& a) : 
			_head(a._head), _arrow(a._arrow), _bound(a._bound), _lower(a._lower), _type(a._type), _set(a._set)
			{ assert(a._arrow != TS_RULE); }
		GroundAggregate() { }

		// Inspectors
		int				head()	const { return _head;	}
		TsType			arrow()	const { return _arrow;	}
		double			bound()	const { return _bound;	}
		bool			lower()	const { return _lower;	}
		AggFunction		type()	const { return _type;	}
		unsigned int	setnr()	const { return _set; 	}

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};


/*************************
	Ground definitions
*************************/

// Enumeration type for rules
enum RuleType { RT_CONJ, RT_DISJ, RT_AGG };

/**
 * class GroundRuleBody
 *		This class represents a ground rule body, where literals are represented by integers.
 */ 
class GroundRuleBody {
	private:
		RuleType	_type;					// The rule type (disjunction, conjunction, or aggregate
		bool		_recursive;				// True iff the rule body contains defined literals

	public:
		// Constructors
		GroundRuleBody() { }
		GroundRuleBody(RuleType type, bool rec): _type(type), _recursive(rec) { }

		// Destructor
		virtual		~GroundRuleBody() { }

		// Inspectors
				RuleType	type()		const { return _type; 		}
				bool		recursive()	const { return _recursive;	}
		virtual	bool		isFalse()	const = 0;
		virtual	bool		isTrue()	const = 0;

		// Visitor
		virtual void accept(TheoryVisitor*) const = 0;
		virtual GroundRuleBody* accept(TheoryMutatingVisitor* v) = 0;

		// Friends
		friend class GroundDefinition;
};

/**
 * class PCGroundRuleBody
 *		This class represents ground rule bodies that are conjunctions or disjunctions of literals.
 */
class PCGroundRuleBody : public GroundRuleBody {
	private:
		std::vector<int>	_body;	// The literals in the body

	public:
		// Constructors
		PCGroundRuleBody(RuleType type, const std::vector<int>& body, bool rec) : GroundRuleBody(type,rec), _body(body) { }
		PCGroundRuleBody(const PCGroundRuleBody& grb): GroundRuleBody(grb.type(),grb.recursive()), _body(grb._body) { }

		~PCGroundRuleBody() { }

		// Inspectors
		std::vector<int>		body()					const { return _body;								}
		unsigned int	size()					const { return _body.size();						}
		bool			empty()					const { return _body.empty();						}
		int				literal(unsigned int n)	const { return _body[n];							}
		bool			isFalse()				const { return (_body.empty() && type() == RT_DISJ);	}
		bool			isTrue()				const { return (_body.empty() && type() == RT_CONJ);	}

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
		GroundRuleBody* accept(TheoryMutatingVisitor* v) { return v->visit(this);	}

		// Friends
		friend class GroundDefinition;
};

/**
 * class AggGroundRuleBody
 *		This class represents ground rule bodies that are aggregates.
 */
class AggGroundRuleBody : public GroundRuleBody {
	private:
		int		_setnr;		// The id of the set of the aggregate
		AggFunction _aggtype;	// The aggregate type (cardinality, sum, product, min, or max)
		bool	_lower;		// True iff the bound is a lower bound
		double	_bound;		// The bound on the aggregate

	public:
		// Constructors
		AggGroundRuleBody(int setnr, AggFunction at, bool lower, double bound, bool rec):
			GroundRuleBody(RT_AGG,rec), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		AggGroundRuleBody(const AggGroundRuleBody& grb):
			GroundRuleBody(RT_AGG,grb.recursive()), _setnr(grb._setnr), _aggtype(grb._aggtype), _lower(grb._lower), _bound(grb._bound) { }

		~AggGroundRuleBody() { }

		// Inspectors
		unsigned int	setnr()		const { return _setnr;		}
		AggFunction		aggtype()	const { return _aggtype;	}
		bool			lower()		const { return _lower;		}
		double			bound()		const { return _bound;		}
		bool			isFalse()	const { return false;		}
		bool			isTrue()	const { return false;		}

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
		GroundRuleBody* accept(TheoryMutatingVisitor* v) { return v->visit(this);	}

		// Friends
		friend class GroundDefinition;
};

/**
 * class GroundDefinition
 *		This class represents ground definitions.
 */
class GroundDefinition : public AbstractDefinition {
	private:
		GroundTranslator*				_translator;
		std::map<int,GroundRuleBody*>	_rules;			// Maps a head to its corresponding body

	public:
		// Constructors
		GroundDefinition(GroundTranslator* tr) : _translator(tr) { }
		GroundDefinition* clone() const;
		void recursiveDelete();

		// Mutators
		void addTrueRule(int head);
		void addFalseRule(int head);
		void addPCRule(int head, const std::vector<int>& body, bool conj, bool recursive);
		void addAggRule(int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive);

		typedef std::map<int,GroundRuleBody*>::iterator	ruleiterator;
		ruleiterator	rule(int head)	{ return _rules.find(head);	}
		ruleiterator	begin()			{ return _rules.begin();	}
		ruleiterator	end()			{ return _rules.end();		}

		// Inspectors
		GroundTranslator*	translator()	const { return _translator;			}
		unsigned int		nrRules() 		const { return _rules.size();		}

		typedef std::map<int,GroundRuleBody*>::const_iterator	const_ruleiterator;
		const_ruleiterator	rule(int head)	const { return _rules.find(head);	}
		const_ruleiterator	begin()			const { return _rules.begin();		}
		const_ruleiterator	end()			const { return _rules.end();		}

		// Visitor
		void 				accept(TheoryVisitor* v) const		{ v->visit(this);			}
		AbstractDefinition*	accept(TheoryMutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		std::ostream&	put(std::ostream&,unsigned int spaces = 0) const;
		std::string toString(unsigned int spaces = 0) const;
};


/****************************************
	Ground nested fixpoint definitions 
****************************************/

class GroundFixpDef : public AbstractDefinition {
	private:
//		std::map<int,GroundRuleBody*>	_rules;		// the direct subrules
//		std::vector<GroundFixpDef*>		_subdefs;	// the direct subdefinitions
	public:
		//TODO
};


/**********************
	CP reifications
**********************/

/**
 * class CPReification
 * 		This class represents CP constraints.
 */
class CPReification { //TODO ?
	public:
		int 		_head;
		CPTsBody* 	_body;
		CPReification(int head, CPTsBody* body): _head(head), _body(body) { }
		std::string toString(unsigned int spaces = 0) const;
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

/**********************
	Ground theories
**********************/

/**
 * Implements base class for ground theories
 */
class AbstractGroundTheory : public AbstractTheory {
	protected:
		AbstractStructure*		_structure;			//!< The ground theory may be partially reduced with respect to this structure.
		GroundTranslator*		_translator;		//!< Link between ground atoms and SAT-solver literals.
		GroundTermTranslator*	_termtranslator;	//!< Link between ground terms and CP-solver variables.

		std::set<int>			_printedtseitins;		//!< Tseitin atoms produced by the translator that occur in the theory.
		std::set<int>			_printedsets;			//!< Set numbers produced by the translator that occur in the theory.
		std::set<int>			_printedconstraints;	//!< Atoms for which a connection to CP constraints are added.
		std::set<CPTerm*>		_foldedterms;

		const 	GroundTranslator& getTranslator() const	{ return *_translator; }
				GroundTranslator& getTranslator() 		{ return *_translator; }

	public:
		// Constructors
		AbstractGroundTheory(AbstractStructure* str);
		AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str);

		// Destructor
		virtual void recursiveDelete() = 0;
		virtual	~AbstractGroundTheory();

		// Mutators
				void 	add(Formula* )		{ assert(false);	}
				void 	add(Definition* )	{ assert(false);	}
				void 	add(FixpDef* )		{ assert(false);	}

				void 	transformForAdd(const std::vector<int>& vi, VIType vit, int defnr, bool skipfirst = false);

				CPTerm*	foldCPTerm(CPTerm*);

		virtual void 	addClause(GroundClause& cl, bool skipfirst = false)		= 0;
		virtual void 	addDefinition(GroundDefinition*)						= 0;
		virtual void 	addFixpDef(GroundFixpDef*)								= 0;
		virtual void 	addSet(int setnr, int defnr, bool weighted)				= 0;
		virtual	void 	addAggregate(int tseitin, AggTsBody* body)				= 0;
		virtual void 	addCPReification(int tseitin, CPTsBody* body)			= 0;

				void 	addEmptyClause()		{ GroundClause c(0); addClause(c);		}
				void 	addUnitClause(int l)	{ GroundClause c(1,l); addClause(c);	}

		virtual void 	addPCRule(int defnr, int tseitin, PCTsBody* body, bool recursive)= 0;
		virtual void 	addAggRule(int defnr, int tseitin, AggTsBody* body, bool recursive)		= 0;

				void	addFuncConstraints();

		// Inspectors
		GroundTranslator*		translator()		const { return _translator;			}
		GroundTermTranslator*	termtranslator()	const { return _termtranslator; 	}
		AbstractStructure*		structure()			const { return _structure;			}
		AbstractGroundTheory*	clone()				const { assert(false); return NULL;/* TODO */	}
};

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
class SolverTheory : public AbstractGroundTheory {
	private:
		SATSolver*							_solver;		// The SAT solver
		std::map<PFSymbol*,std::set<int> >	_defined;		// Symbols that are defined in the theory. This set is used to
															// communicate to the solver which ground atoms should be considered defined.
		std::set<unsigned int> 				_addedvarids;	// Variable ids that have already been added, together with their domain.

		int	_verbosity;

		const 	SATSolver& getSolver() const	{ return *_solver; }
				SATSolver& getSolver() 			{ return *_solver; }

	public:
		// Constructors
		SolverTheory(SATSolver* solver, AbstractStructure* str, int verbosity);
		SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str, int verbosity);

		// Destructors
		void recursiveDelete() { delete(this);	}

		// Mutators
		virtual void	addClause(GroundClause& cl, bool skipfirst = false);
		virtual void	addDefinition(GroundDefinition*);
		virtual void	addFixpDef(GroundFixpDef*);
		virtual void	addSet(int setnr, int defnr, bool weighted);
		virtual void	addAggregate(int tseitin, AggTsBody* body);
		virtual void 	addCPReification(int tseitin, CPTsBody* body);

		virtual void	addPCRule(int defnr, int tseitin, PCTsBody* body, bool recursive);
		virtual void	addAggRule(int defnr, int tseitin, AggTsBody* body, bool recursive);

				void	addFalseDefineds();

		// Visitor
		virtual void			accept(TheoryVisitor* v) const		{ v->visit(this);			}
		virtual AbstractTheory*	accept(TheoryMutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		virtual std::ostream&	put(std::ostream& output, unsigned int)	const { assert(false); /* TODO */ return output;	   }
		virtual std::string		to_string()								const { assert(false); return ""; /*TODO might be implemented in the future (if solver supports it)*/	}

	private:
		void 	addAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound);
		void 	addPCRule(int definitionID, int head, std::vector<int> body, bool conjunctive, bool recursive);
		void	addPCRule(int defnr, int head, PCGroundRuleBody* body, bool recursive);
		void	addAggRule(int defnr, int head, AggGroundRuleBody* body, bool recursive);
		void	addCPVariable(const VarId&);
		void	addCPVariables(const std::vector<VarId>&);
};

class GroundTheory : public AbstractGroundTheory {
	private:
		std::vector<GroundClause>		_clauses;
		std::vector<GroundDefinition*>	_definitions;
		std::vector<GroundFixpDef*>		_fixpdefs;
		std::vector<GroundSet*>			_sets;
		std::vector<GroundAggregate*>	_aggregates;
		std::vector<CPReification*>		_cpreifications;

	public:
		// Constructors
		GroundTheory(AbstractStructure* str) : AbstractGroundTheory(str) { }
		GroundTheory(Vocabulary* voc, AbstractStructure* str) : AbstractGroundTheory(voc,str)	{ }
		void recursiveDelete();

		// Mutators
		void	addClause(GroundClause& cl, bool skipfirst = false);
		void	addDefinition(GroundDefinition*);
		void	addFixpDef(GroundFixpDef*);
		void	addSet(int setnr, int defnr, bool weighted);
		void	addAggregate(int tseitin, AggTsBody* body);
		void 	addCPReification(int tseitin, CPTsBody* body);

		void	addPCRule(int defnr, int tseitin, PCTsBody* body, bool recursive);
		void	addAggRule(int defnr, int tseitin, AggTsBody* body, bool recursive);

		// Inspectors
		unsigned int		nrClauses()						const { return _clauses.size();							}
		unsigned int		nrDefinitions()					const { return _definitions.size();						}
		unsigned int		nrFixpDefs()					const { return _fixpdefs.size();						}
		unsigned int		nrSets()						const { return _sets.size();							}
		unsigned int		nrAggregates()					const { return _aggregates.size();						}
		unsigned int 		nrCPReifications()				const { return _cpreifications.size();					}
		GroundClause		clause(unsigned int n)			const { return _clauses[n];								}
		GroundDefinition*	definition(unsigned int n)		const { return _definitions[n];							}
		GroundFixpDef*		fixpdef(unsigned int n)			const { return _fixpdefs[n];							}
		GroundSet*			set(unsigned int n)				const { return _sets[n];								}
		GroundAggregate*	aggregate(unsigned int n)		const { return _aggregates[n];							}
		CPReification*		cpreification(unsigned int n)	const { return _cpreifications[n];						}

		// Visitor
		void			accept(TheoryVisitor* v) const		{ v->visit(this);			}
		AbstractTheory*	accept(TheoryMutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		std::ostream&	put(std::ostream&, unsigned int spaces = 0)	const;
		std::string		to_string()									const;
};

#endif
