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
 * class GroundRule
 *		This class represents a ground rule body, where literals are represented by integers.
 */ 
class GroundRule {
	private:
		int			_head;
		RuleType	_type;					// The rule type (disjunction, conjunction, or aggregate
		bool		_recursive;				// True iff the rule body contains defined literals

	public:
		GroundRule(int head, RuleType type, bool rec): _head(head), _type(type), _recursive(rec) { }

		// Destructor
		virtual		~GroundRule() { }

		// Inspectors
				int			head()		const { return _head; 		}
				RuleType	type()		const { return _type; 		}
				bool		recursive()	const { return _recursive;	}
		virtual	bool		isFalse()	const = 0;
		virtual	bool		isTrue()	const = 0;

		// Visitor
		virtual void accept(TheoryVisitor*) const = 0;
		virtual GroundRule* accept(TheoryMutatingVisitor* v) = 0;
};

/**
 * class PCGroundRule
 *		This class represents ground rule bodies that are conjunctions or disjunctions of literals.
 */
class PCGroundRule : public GroundRule {
	private:
		std::vector<int>	_body;	// The literals in the body

	public:
		// Constructors
		PCGroundRule(int head, RuleType type, const std::vector<int>& body, bool rec) : GroundRule(head, type,rec), _body(body) { }
		PCGroundRule(const PCGroundRule& grb): GroundRule(grb.head(), grb.type(),grb.recursive()), _body(grb._body) { }

		~PCGroundRule() { }

		// Inspectors
		std::vector<int>	body()				const { return _body;								}
		unsigned int	size()					const { return _body.size();						}
		bool			empty()					const { return _body.empty();						}
		int				literal(unsigned int n)	const { return _body[n];							}
		bool			isFalse()				const { return (_body.empty() && type() == RT_DISJ);	}
		bool			isTrue()				const { return (_body.empty() && type() == RT_CONJ);	}

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
		GroundRule* accept(TheoryMutatingVisitor* v) { return v->visit(this);	}
};

/**
 * class AggGroundRule
 *		This class represents ground rule bodies that are aggregates.
 */
class AggGroundRule : public GroundRule {
	private:
		int		_setnr;		// The id of the set of the aggregate
		AggFunction _aggtype;	// The aggregate type (cardinality, sum, product, min, or max)
		bool	_lower;		// True iff the bound is a lower bound
		double	_bound;		// The bound on the aggregate

	public:
		// Constructors
		AggGroundRule(int head, int setnr, AggFunction at, bool lower, double bound, bool rec):
			GroundRule(head, RT_AGG,rec), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		AggGroundRule(const AggGroundRule& grb):
			GroundRule(grb.head(), RT_AGG,grb.recursive()), _setnr(grb._setnr), _aggtype(grb._aggtype), _lower(grb._lower), _bound(grb._bound) { }

		~AggGroundRule() { }

		// Inspectors
		unsigned int	setnr()		const { return _setnr;		}
		AggFunction		aggtype()	const { return _aggtype;	}
		bool			lower()		const { return _lower;		}
		double			bound()		const { return _bound;		}
		bool			isFalse()	const { return false;		}
		bool			isTrue()	const { return false;		}

		// Visitor
		void accept(TheoryVisitor* v) const { v->visit(this);	}
		GroundRule* accept(TheoryMutatingVisitor* v) { return v->visit(this);	}
};

/**
 * class GroundDefinition
 *		This class represents ground definitions.
 */
class GroundDefinition : public AbstractDefinition {
	private:
		unsigned int				_id;
		GroundTranslator*			_translator;
		std::vector<GroundRule*>	_rules;

	public:
		// Constructors
		GroundDefinition(unsigned int id, GroundTranslator* tr) : _id(id), _translator(tr) { }
		GroundDefinition* clone() const;
		void recursiveDelete();

		void add(GroundRule* rule);

		unsigned int id() const { return _id; }

		typedef std::vector<GroundRule*>::iterator	ruleiterator;
		ruleiterator	begin()			{ return _rules.begin();	}
		ruleiterator	end()			{ return _rules.end();		}

		GroundTranslator*	translator()	const { return _translator;			}

		typedef std::vector<GroundRule*>::const_iterator	const_ruleiterator;
		const_ruleiterator	begin()			const { return _rules.begin();		}
		const_ruleiterator	end()			const { return _rules.end();		}

		// Visitor
		void 				accept(TheoryVisitor* v) const		{ v->visit(this);			}
		AbstractDefinition*	accept(TheoryMutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		std::ostream&	put(std::ostream&,unsigned int spaces = 0) const;
		std::string to_string(unsigned int spaces = 0) const;
};


/****************************************
	Ground nested fixpoint definitions 
****************************************/

class GroundFixpDef : public AbstractDefinition {
	private:
//		std::map<int,GroundRule*>	_rules;		// the direct subrules
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
		std::string to_string(unsigned int spaces = 0) const;
		void accept(TheoryVisitor* v) const { v->visit(this);	}
};

#endif
