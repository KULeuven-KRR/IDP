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

/***********************
	Ground theories
***********************/

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

/** Ground definitions **/
struct GroundRuleBody {
	RuleType	_type;
	vector<int>	_body;
	bool		_recursive;
};

struct GroundDefinition {
	GroundTranslator*		_translator;
	map<int,GroundRuleBody>	_rules;			// maps a head to its corresponding body

	GroundDefinition() { }
	GroundDefinition(GroundTranslator* tr) : _translator(tr) { }
	void addTrueRule(int head);
	void addRule(int head, const vector<int>& body, bool conj, bool recursive);
	void addAgg(const EcnfAgg& , GroundTranslator* ) { /* TODO */ }
	string to_string() const;
};

/** Propositional fixpoint definition **/
struct EcnfFixpDef {
	EcnfDefinition		_rules;		// the direct subrules
	vector<EcnfFixpDef>	_subdefs;	// the direct subdefinitions
	void addAgg(const EcnfAgg& a, GroundTranslator* t) { _rules.addAgg(a,t);	}
	void addRule(int head, const vector<int>& body, bool conj, GroundTranslator* t) { _rules.addRule(head,body,conj,t);	}
	bool containsAgg()	const;
};

class GroundTheory : public AbstractTheory {

	protected:
		GroundTranslator*	_translator;		// Link between ground atoms and SAT-solver literals
		set<int>			_printedtseitins;	// Tseitin atoms produced by the translator that occur 
												// in the theory.
		set<int>			_printedsets;		
		AbstractStructure*	_structure;			// The ground theory may be partially reduced with respect
												// to this structure. 

	public:
		// Constructors 
		GroundTheory(AbstractStructure* str) : 
			AbstractTheory("",ParseInfo()), _translator(new GroundTranslator()), _structure(str) { }
		GroundTheory(Vocabulary* voc, AbstractStructure* str) : 
			AbstractTheory("",voc,ParseInfo()), _translator(new GroundTranslator()), _structure(str) { }

		// Destructor
		virtual void recursiveDelete()	{ delete(this);			}
		virtual	~GroundTheory()			{ delete(_translator);	}

		// Mutators
				void add(Formula* )		{ assert(false);	}
				void add(Definition* )	{ assert(false);	}
				void add(FixpDef* )		{ assert(false);	}

				void transformForAdd(EcnfClause& cl, bool firstIsPrinted = false);
				void transformForAdd(GroundDefinition& d);
				void transformForAdd(GroundRuleBody& grb, vector<int>& heads, vector<GroundRuleBody>& bodies);

		virtual void addClause(EcnfClause& cl, bool firstIsPrinted = false) = 0;
				void addEmptyClause()		{ EcnfClause c(0); addClause(c);	}
				void addUnitClause(int l)	{ EcnfClause c(1,l); addClause(c);	}
		virtual void addDefinition(GroundDefinition&) = 0;
		virtual	void addAgg(int head, AggTsBody& body) = 0;
		virtual void addSet(int setnr, bool weighted) = 0;
		virtual void addFuncConstraints() = 0;



		// Inspectors
				GroundTranslator*	translator()	const { return _translator;	}
				GroundTheory*		clone()			const { assert(false); /* TODO */	}
};

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
														_features._containsAggregates || d.containsAgg();				}*/
		void addDefinition(GroundDefinition& d)		{ transformForAdd(d);
													  _definitions.push_back(d); }
		void addFixpDef(const EcnfFixpDef& d)		{ _fixpdefs.push_back(d); 
													  _features._containsFixpDefs = true;		
													  _features._containsAggregates = 
														_features._containsAggregates || d.containsAgg();				} 
		void addAgg(const EcnfAgg& a)				{ _aggregates.push_back(a); _features._containsAggregates = true;	}
		void addAgg(int head, AggTsBody& body);

		void addSet(int setnr, const vector<int>& lits, const vector<double>& weights)
													{ _sets.push_back(EcnfSet(setnr,lits,weights));	}
		void addSet(int setnr, bool weighted);
		void	addFuncConstraints()				{	/* TODO??  */}

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

/* SolverTheory acts as an interface to the theory of a SAT solver
*/
class SolverTheory : public GroundTheory {

	private:
		SATSolver*			_solver;		// SAT solver

	public:
		// Constructors 
		SolverTheory(SATSolver* solver,AbstractStructure* str) : 
			GroundTheory(str), _solver(solver) { }
		SolverTheory(Vocabulary* voc, SATSolver* solver, AbstractStructure* str) : 
			GroundTheory(voc,str), _solver(solver) { }

		// Mutators
		void	addClause(EcnfClause& cl, bool firstIsPrinted = false);
		void	addDefinition(GroundDefinition&);
		void	addAgg(int head, AggTsBody& body);
		void	addSet(int setnr, bool weighted);
		void	addFuncConstraints();

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

#endif
