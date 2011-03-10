/************************************
	ecnf.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ECNF_H
#define ECNF_H

#include "theory.hpp"
#include "ground.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

typedef MinisatID::WrappedPCSolver SATSolver;
typedef vector<int> GroundClause;

/***********************
	Ground theories
***********************/

/* Enumeration used in GroundTheory::transformForAdd */
enum VIType { VIT_DISJ, VIT_CONJ, VIT_SET };

/** Ground definitions **/
class GroundRuleBody {
	public:
		RuleType	_type;
		bool		_recursive;
};

class PCGroundRuleBody : public GroundRuleBody {
	public:
		vector<int>	_body;
};

class AggGroundRuleBody : public GroundRuleBody {
	public:
		int		_setnr;
		AggType _aggtype;
		bool	_lower;
		double	_bound;
};

struct GroundDefinition {
	GroundTranslator*		_translator;
	map<int,GroundRuleBody>	_rules;			// maps a head to its corresponding body

	GroundDefinition() { }
	GroundDefinition(GroundTranslator* tr) : _translator(tr) { }
	void addTrueRule(int head);
	void addRule(int head, const vector<int>& body, bool conj, bool recursive);
	void addAgg(int head, int setnr, AggType aggtype, bool lower, double bound, bool recursive) { /* TODO */ }
	string to_string() const;
};

/** Ground Aggregates
 * Propositional expression of one of the following forms
		(a) head <- agg(set) =< bound
		(b) head <- bound =< agg(set)
		(c) head <=> agg(set) =< bound
		(d) head <=> bound =< agg(set)
		(e) head => agg(set) =< bound
		(f) head => bound =< agg(set)
**/
struct GroundAggregate {
	AggType			_type;		// the aggregate
	bool			_lower;		// true in cases (b) and (d)
	TsType			_eha;		// the relation between head and aggregate expression
	int				_head;		// head atom
	unsigned int	_set;		// the set id
	double			_bound;		// the bound
	GroundAggregate(AggType t, bool l, TsType e, int h, unsigned int s, double b) :
		_type(t), _lower(l), _eha(e), _head(h), _set(s), _bound(b) { }
	GroundAggregate(const EcnfAgg& efa) : 
		_type(efa._type), _lower(efa._lower), _eha(efa._eha), _head(efa._head), _set(efa._set), _bound(efa._bound) { }
	GroundAggregate() { }
};

/** Ground set **/
struct GroundSet {
	int	_setnr;
	vector<int>	_set;
	vector<double> _weights;
	GroundSet(int setnr, const vector<int>& s, const vector<double>& w) : _setnr(setnr), _set(s), _weights(w) { }
};

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

		map<PFSymbol*,set<int> >	_defined;	// Symbols that are defined in the theory. This set is used to
												// communicate to the solver which ground atoms should be considered defined.

	public:
		// Constructors 
		AbstractGroundTheory(AbstractStructure* str) : 
			AbstractTheory("",ParseInfo()), _translator(new GroundTranslator()), _structure(str) { }
		AbstractGroundTheory(Vocabulary* voc, AbstractStructure* str) : 
			AbstractTheory("",voc,ParseInfo()), _translator(new GroundTranslator()), _structure(str) { }

		// Destructor
		virtual void recursiveDelete()	{ delete(this);			}
		virtual	~GroundTheory()			{ delete(_translator);	}

		// Mutators
				void add(Formula* )		{ assert(false);	}
				void add(Definition* )	{ assert(false);	}
				void add(FixpDef* )		{ assert(false);	}

				void transformForAdd(vector<int>& vi, VIType vit, int defnr, bool skipfirst = false);

				void addEmptyClause()		{ GroundClause c(0); addClause(c);		}
				void addUnitClause(int l)	{ GroundClause c(1,l); addClause(c);	}
		virtual void addClause(GroundClause& cl, bool skipfirst = false)	= 0;
		virtual void addDefinition(GroundDefinition&)						= 0;
		virtual	void addAggregate(int head, AggTsBody& body)				= 0;
		virtual void addSet(int setnr, int defnr, bool weighted)			= 0;

//		virtual void addPCRule(int defnr, int head, PCTsBody& body)			= 0;
//		virtual void addAggRule(int defnr, int head, AggTsBody& body)		= 0;

		// Inspectors
		GroundTranslator*		translator()	const { return _translator;	}
		AbstractGroundTheory*	clone()			const { assert(false); /* TODO */	}
};

/* 
 * SolverTheory acts as an interface to the theory of a SAT solver
 */
class SolverTheory : public AbstractGroundTheory {

	private:
		SATSolver*		_solver;		// SAT solver

	public:
		// Constructors 
		SolverTheory(SATSolver* solver,AbstractStructure* str) : 
			AbstractGroundTheory(str), _solver(solver) { }
		SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str) : 
			AbstractGroundTheory(voc,str), _solver(solver) { }

		// Mutators
		void	addClause(GroundClause& cl, bool skipfirst = false);
		void	addDefinition(GroundDefinition&);
		void	addAggregate(int head, AggTsBody& body);
		void	addSet(int setnr, int defnr, bool weighted);

//		void	addPCRule(int defnr, int head, PCTsBody& body);
//		void	addAggRule(int defnr, int head, AggTsBody& body);

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
		void	addAggregate(int head, AggTsBody& body);
		void	addSet(int setnr, int defnr, bool weighted);

//		void	addPCRule(int defnr, int head, PCTsBody& body);
//		void	addAggRule(int defnr, int head, AggTsBody& body);

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


#ifdef ROMMEL
/** Propositional clause **/
typedef vector<int> EcnfClause;	// vector of all literals in a clause

/** Propositional set **/
struct EcnfSet {
	int	_setnr;
	vector<int>	_set;
	vector<double> _weights;
	EcnfSet(int setnr, const vector<int>& s, const vector<double>& w) : _setnr(setnr), _set(s), _weights(w) { }
};

/** Propositional expression of one of the following forms
		(a) head <- agg(set) =< bound
		(b) head <- bound =< agg(set)
		(c) head <=> agg(set) =< bound
		(d) head <=> bound =< agg(set)
		(e) head => agg(set) =< bound
		(f) head => bound =< agg(set)
**/
struct EcnfAgg {
	AggType			_type;		// the aggregate
	bool			_lower;		// true in cases (b) and (d)
	TsType			_eha;		// the relation between head and aggregate expression
	int				_head;		// head atom
	unsigned int	_set;		// the set id
	double			_bound;		// the bound
	EcnfAgg(AggType t, bool l, TsType e, int h, unsigned int s, double b) :
		_type(t), _lower(l), _eha(e), _head(h), _set(s), _bound(b) { }
	EcnfAgg(const EcnfAgg& efa) : 
		_type(efa._type), _lower(efa._lower), _eha(efa._eha), _head(efa._head), _set(efa._set), _bound(efa._bound) { }
	EcnfAgg() { }
};

/** Propositional definition **/
enum RuleType { RT_TRUE, RT_FALSE, RT_UNARY, RT_CONJ, RT_DISJ, RT_AGG };
struct EcnfDefinition {
	map<int,RuleType>		_ruletypes;	// map a head to its current ruletype
	map<int,vector<int> >	_bodies;	// map a head to its body (conjunctive and disjunctive rules)
	map<int,EcnfAgg>		_aggs;		// map a head to its aggregate body
	void addAgg(const EcnfAgg& a, GroundTranslator* t);
	void addRule(int head, const vector<int>& body, bool conj, GroundTranslator* t);
	bool containsAgg()	const { return !_aggs.empty();	}
};
/** Propositional fixpoint definition **/
struct EcnfFixpDef {
	EcnfDefinition		_rules;		// the direct subrules
	vector<EcnfFixpDef>	_subdefs;	// the direct subdefinitions
	void addAgg(const EcnfAgg& a, GroundTranslator* t) { _rules.addAgg(a,t);	}
	void addRule(int head, const vector<int>& body, bool conj, GroundTranslator* t) { _rules.addRule(head,body,conj,t);	}
	bool containsAgg()	const;
};


				void transformForAdd(GroundDefinition& d);
				void transformForAdd(PCGroundRuleBody& grb, vector<int>& heads, vector<GroundRuleBody>& bodies);
				void transformForAdd(AggGroundRuleBody& grb, vector<int>& heads, vector<GroundRuleBody>& bodies);

class EcnfTheory : public GroundTheory {
	
	private:
		GroundFeatures				_features;
		
		vector<EcnfClause>			_clauses;	
		vector<GroundDefinition>	_definitions;
		vector<EcnfAgg>				_aggregates;
		vector<EcnfFixpDef>			_fixpdefs;
		vector<EcnfSet>				_sets;

	public:

		// Constructor
		EcnfTheory(AbstractStructure* str) : GroundTheory(str) { }
		EcnfTheory(Vocabulary* voc, AbstractStructure* str) : GroundTheory(voc,str)	{ }

		// Mutators
		void addClause(EcnfClause& cl, bool firstIsPrinted = false);
/*		void addDefinition(const EcnfDefinition& d)	{ _definitions.push_back(d);
													  _features._containsDefinitions = true;	
													  _features._containsAggregates = 
														_features._containsAggregates || d.containsAgg();				}*/ //TODO: remove this method?
		void addDefinition(GroundDefinition& d)		{ transformForAdd(d);
													  _definitions.push_back(d); }
/*		void addFixpDef(const EcnfFixpDef& d)		{ _fixpdefs.push_back(d);
													  _features._containsFixpDefs = true;		
													  _features._containsAggregates = 
														_features._containsAggregates || d.containsAgg();				} */ //TODO: remove this method?
/*		void addAggregate(const EcnfAgg& a)			{ _aggregates.push_back(a); _features._containsAggregates = true;	} */ //TODO: remove this method?
		void addAggregate(int head, AggTsBody& body);

		void addSet(int setnr, bool weighted);
		void addFuncConstraints()					{	/* TODO??  */	}
		void addFalseDefineds()						{	/* TODO??	*/	}

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

		// Output
		void print(GroundPrinter*);

};


#endif
