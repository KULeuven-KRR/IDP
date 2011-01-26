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

struct GroundFeatures {
	bool	_containsDefinitions;
	bool	_containsAggregates;
	bool	_containsFixpDefs;
	GroundFeatures() {
		_containsDefinitions = false;
		_containsAggregates = false;
		_containsFixpDefs = false;
	}
};

class GroundPrinter {

	protected:

		GroundPrinter() { }

	public:
		virtual ~GroundPrinter() { }

		/**
		 * Output the header of the ground file.
		 */
		virtual void outputinit(GroundFeatures*) = 0;	
		virtual void outputend() = 0;
		virtual void outputtseitin(int l) = 0;
		virtual void outputunitclause(int l) = 0;
		virtual void outputclause(const vector<int>& l) = 0;
		virtual void outputunitrule(int h, int b) = 0;
		virtual void outputrule(int h, const vector<int>& b, bool c) = 0;
		/*
		 * equiv if !defined, otherwise definition
		 * =< if lowerthan, otherwise >=
		 * e.g. !defined && lowerthan: h <=> max(setid) =< bound
		 */
		virtual void outputmax(int h, bool defined, int setid, bool lowerthan, int bound) = 0;
		virtual void outputmin(int h, bool defined, int setid, bool lowerthan, int bound) = 0;
		virtual void outputsum(int h, bool defined, int setid, bool lowerthan, int bound) = 0;
		virtual void outputprod(int h, bool defined, int setid, bool lowerthan, int bound) = 0;
		//TODO important: no longer supported by solver
		virtual void outputeu(const vector<int>&) = 0;
		//TODO important: no longer supported by solver
		virtual void outputamo(const vector<int>&) = 0;
		virtual void outputcard(int h, bool defined, int setid, bool lowerthan, int bound) = 0;
		virtual void outputset(int setid, const vector<int>& sets) = 0;
		virtual void outputunitfdrule(int d, int h, int b) = 0;
		virtual void outputfdrule(int d, int h, const vector<int>& b, bool c) = 0;
		virtual void outputfixpdef(int d, const vector<int>& sd, bool l) = 0;
		virtual void outputwset(int s, const vector<int>& sets, const vector<int>& weights) = 0;
		virtual void outputunsat() = 0;

		void outputgroundererror();

};

class outputECNF : public GroundPrinter {

	private:
		FILE*	_out;
	public:
		outputECNF(FILE* f);
		~outputECNF();
		void outputinit(GroundFeatures*);
		void outputend();
		void outputtseitin(int){};
		void outputunitclause(int l);
		void outputclause(const vector<int>& l);
		void outputunitrule(int h, int b);
		void outputrule(int h, const vector<int>& b, bool c);
		void outputcard(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmax(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmin(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputsum(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputprod(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputeu(const vector<int>&);
		void outputamo(const vector<int>&);
		void outputunitfdrule(int d, int h, int b);
		void outputfdrule(int d, int h, const vector<int>& b, bool c);
		void outputfixpdef(int d, const vector<int>& sd, bool l);
		void outputset(int setid, const vector<int>& sets);
		void outputwset(int s, const vector<int>& sets, const vector<int>& weights);
		void outputunsat();

};

class outputHR : public GroundPrinter {

	private:
		FILE* _out;
	public:
		outputHR(FILE*);
		~outputHR();
		void outputinit(GroundFeatures*);
		void outputend();
		void outputtseitin(int){};
		void outputunitclause(int l);
		void outputclause(const vector<int>& l);
		void outputunitrule(int h, int b);
		void outputrule(int h, const vector<int>& b, bool c);
		void outputcard(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmax(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmin(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputsum(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputprod(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputeu(const vector<int>&);
		void outputamo(const vector<int>&);
		void outputset(int setid, const vector<int>& sets);
		void outputwset(int s, const vector<int>& sets, const vector<int>& weights);
		void outputunitfdrule(int d, int h, int b);
		void outputfdrule(int d, int h, const vector<int>& b, bool c);
		void outputfixpdef(int d, const vector<int>& sd, bool l);
		void outputunsat();

	private:
		void outputaggregate(int h, bool defined, int setid, bool lowerthan, int bound, const char* type);

};

typedef MinisatID::WrappedPCSolver SATSolver;

class outputToSolver : public GroundPrinter {

	private:
		//Not owning pointer!
		SATSolver* _solver;
		SATSolver* solver() { return _solver; }
	public:
		//outputToSolver();
		outputToSolver(SATSolver* solver);
		~outputToSolver();
		void outputinit(GroundFeatures*);
		void outputend();
		void outputtseitin(int){};
		void outputunitclause(int l);
		void outputclause(const vector<int>& l);
		void outputunitrule(int h, int b);
		void outputrule(int h, const vector<int>& b, bool c);
		void outputcard(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmax(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputmin(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputsum(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputprod(int h, bool defined, int setid, bool lowerthan, int bound);
		void outputeu(const vector<int>&);
		void outputamo(const vector<int>&);
		void outputunitfdrule(int d, int h, int b);
		void outputfdrule(int d, int h, const vector<int>& b, bool c);
		void outputfixpdef(int d, const vector<int>& sd, bool l);
		void outputset(int setid, const vector<int>& sets);
		void outputwset(int setid, const vector<int>& sets, const vector<int>& weights);
		void outputunsat();

};

/*****************************
	Internal ecnf theories
*****************************/

/** Propositional clause **/
typedef vector<int> EcnfClause;	// vector of all literals in a clause

/** Propositional set **/
struct EcnfSet {
	vector<int>	_set;
	vector<double> _weights;
	EcnfSet(const vector<int>& s, const vector<double>& w) : _set(s), _weights(w) { }
};

/** Propositional expression of one of the following forms
		(a) head <- agg(set) =< bound
		(b) head <- bound =< agg(set)
		(c) head <=> agg(set) =< bound
		(d) head <=> bound =< agg(set)
		(e) head => agg(set) =< bound
		(f) head => bound =< agg(set)
**/
enum EcnfHeadAgg { EHA_DEFINED, EHA_EQUIV, EHA_IMPLIES }; // cases (a,b), (c,d), and (e,f), respectively.
struct EcnfAgg {
	AggType			_type;		// the aggregate
	bool			_lower;		// true in cases (b) and (d)
	EcnfHeadAgg		_eha;		// the relation between head and aggregate expression
	int				_head;		// head atom
	unsigned int	_set;		// the set id
	double			_bound;		// the bound
	EcnfAgg(AggType t, bool l, EcnfHeadAgg e, int h, unsigned int s, double b) :
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

class EcnfTheory : public AbstractTheory {
	
	private:
		GroundFeatures			_features;
		
		GroundTranslator*		_translator;

		vector<EcnfClause>		_clauses;	
		vector<EcnfDefinition>	_definitions;
		vector<EcnfAgg>			_aggregates;
		vector<EcnfFixpDef>		_fixpdefs;
		vector<EcnfSet>			_sets;

	public:

		// Constructor
		EcnfTheory() : AbstractTheory("",ParseInfo()), _translator(new NaiveTranslator()) { }

		// Destructor
		void recursiveDelete() { }

		// Mutators
		void addClause(const EcnfClause& vi)		{ _clauses.push_back(vi);											}
		void addDefinition(const EcnfDefinition& d)	{ _definitions.push_back(d); 
													  _features._containsDefinitions = true;	
													  _features._containsAggregates = 
														_features._containsAggregates || d.containsAgg();				}
		void addFixpDef(const EcnfFixpDef& d)		{ _fixpdefs.push_back(d); 
													  _features._containsFixpDefs = true;		
													  _features._containsAggregates = 
														_features._containsAggregates || d.containsAgg();				} 
		void addAgg(const EcnfAgg& a)				{ _aggregates.push_back(a); _features._containsAggregates = true;	}

		unsigned int addSet(const vector<int>& lits, const vector<double>& weights)
													{ _sets.push_back(EcnfSet(lits,weights)); return _sets.size() - 1;	}

		void add(Formula* f);	
		void add(Definition* d);
		void add(FixpDef* fd);	

		// Inspectors
		unsigned int		nrSentences()				const { return _clauses.size() + _aggregates.size();	}
		unsigned int		nrDefinitions()				const { return _definitions.size();						}
		unsigned int		nrFixpDefs()				const { return _fixpdefs.size();						}
		Formula*			sentence(unsigned int n)	const;
		Definition*			definition(unsigned int n)	const;
		FixpDef*			fixpdef(unsigned int n)		const;
		GroundTranslator*	translator()				const { return _translator;	}

		// Visitor
		void			accept(Visitor* v)			{ v->visit(this);			}
		AbstractTheory*	accept(MutatingVisitor* v)	{ return v->visit(this);	}

		// Debugging
		string to_string() const;

		// Output
		void print(GroundPrinter*);

};

#endif
