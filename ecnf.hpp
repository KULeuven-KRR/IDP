/************************************
	ecnf.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ECNF_H
#define ECNF_H

#include "term.hpp"
#include "pcsolver/solvers/PCSolver.hpp"

struct GroundFeatures {
	bool	_containsDefinitions;
	bool	_containsAggregates;
	GroundFeatures() {
		_containsDefinitions = false;
		_containsAggregates = false;
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
		void outputtseitin(int l){};
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
		void outputtseitin(int l){};
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

class outputToSolver : public GroundPrinter {

	private:
		//Not owning pointer!
		PCSolver* _solver;
		PCSolver* solver() { return _solver; }
	public:
		//outputToSolver();
		outputToSolver(PCSolver* solver);
		~outputToSolver();
		void outputinit(GroundFeatures*);
		void outputend();
		void outputtseitin(int l){};
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

typedef vector<int> EcnfClause;
typedef vector<int> EcnfRule;

struct EcnfSet {
	vector<int>		_literals;
	vector<double>	_weights;	// empty for a non-weighted set
};

struct EcnfAgg {
	AggType			_type;
	bool			_lower;
	bool			_defined;
	int				_head;
	unsigned int	_set;
	int				_bound;
};

struct EcnfDefinition {
	vector<EcnfRule>	_disjrules;
	vector<EcnfRule>	_conjrules;
};

class EcnfTheory {
	
	private:
		GroundFeatures			_features;

		vector<EcnfClause>		_clauses;
		vector<EcnfDefinition>	_definitions;
		vector<EcnfSet>			_sets;
		vector<EcnfAgg>			_aggregates;

	public:

		// Mutators
		void addClause(const EcnfClause& vi)		{ _clauses.push_back(vi);											}
		void addDefinition(const EcnfDefinition& d)	{ _definitions.push_back(d); _features._containsDefinitions = true;	}
		void addAggregate(const EcnfAgg& a)			{ _aggregates.push_back(a); _features._containsAggregates = true;	}

		// Output
		void print(GroundPrinter*);

};

#endif
