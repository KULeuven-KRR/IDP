/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include "common.hpp"
#include <ostream>

#include "theory.hpp"
#include "visitors/VisitorFriends.hpp"

class TheoryVisitor;

class GroundTranslator;
class PCTsBody;
class AggTsBody;
class CPTsBody;
class SortTable;

class LazyExistsGrounder;
class DomElemContainer;
class DomainElement;
class InstGenerator;

typedef std::vector<int> intweightlist;

/******************
 * Ground clauses
 ******************/

typedef litlist GroundClause;

class GroundSet {
ACCEPTNONMUTATING()
private:
	SetId _setnr;
	litlist _setlits; // All literals in the ground set
	weightlist _litweights; // For each literal a corresponding weight

public:
	GroundSet(SetId setnr, const litlist& s, const weightlist& lw)
			: 	_setnr(setnr),
				_setlits(s),
				_litweights(lw) {
	}
	virtual ~GroundSet() {
	}

	// Inspectors
	SetId setnr() const {
		return _setnr;
	}
	size_t size() const {
		return _setlits.size();
	}
	Lit literal(size_t n) const {
		return _setlits[n];
	}
	Weight weight(size_t n) const {
		return (not _litweights.empty()) ? _litweights[n] : 1;
	}
	bool weighted() const {
		return not _litweights.empty();
	}
};

class GroundEquivalence{
public:
	const Lit head;
	const std::vector<Lit> body;
	const bool conj;
	
	GroundEquivalence(const Lit h, const std::vector<Lit>& b, const bool c):head(h),body(b),conj(c){}
	
	void getClauses(std::vector<GroundClause>& clauses) const{
		clauses.clear();
		int neg = (conj?1:-1);
		Lit hd = neg*head;
		clauses.push_back(GroundClause());
		clauses[0].resize(body.size() + 1, hd);
		for (size_t i = 0; i < body.size(); ++i) {
			Lit b = neg*body[i];
			clauses[0][i + 1] = -b;
			clauses.push_back({-hd , b});
		}
	}
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
ACCEPTNONMUTATING()
private:
	// Attributes
	Lit _head; // the head literal
	TsType _arrow; // the relation between head and aggregate expression
				   // INVARIANT: this should never be TsType::RULE
	double _bound; // the bound
	bool _lower; // true iff the bound is a lower bound for the aggregate
	AggFunction _type; // the aggregate function
	SetId _set; // the set id

public:
	// Constructors
	GroundAggregate(AggFunction t, bool l, TsType e, Lit h, SetId s, double b)
			: 	_head(h),
				_arrow(e),
				_bound(b),
				_lower(l),
				_type(t),
				_set(s) {
		Assert(e != TsType::RULE);
	}
	GroundAggregate(const GroundAggregate& a)
			: 	_head(a._head),
				_arrow(a._arrow),
				_bound(a._bound),
				_lower(a._lower),
				_type(a._type),
				_set(a._set) {
		Assert(a._arrow != TsType::RULE);
	}
	virtual ~GroundAggregate() {
	}

	// Inspectors
	Lit head() const {
		return _head;
	}
	TsType arrow() const {
		return _arrow;
	}
	double bound() const {
		return _bound;
	}
	bool lower() const {
		return _lower;
	}
	AggFunction type() const {
		return _type;
	}
	SetId setnr() const {
		return _set;
	}
};

/*************************
 Ground definitions
 *************************/

// Enumeration type for rules
enum class RuleType {
	CONJ,
	DISJ,
	AGG
};

/**
 * class GroundRule
 *		This class represents a ground rule body, where literals are represented by integers.
 */
class GroundRule {
ACCEPTDECLAREBOTH(GroundRule)
private:
	Lit _head;
	RuleType _type; // The rule type (disjunction, conjunction, or aggregate
	bool _recursive; // True iff the rule body contains defined literals

protected:
	GroundRule(Lit head, RuleType type, bool rec)
			: 	_head(head),
				_type(type),
				_recursive(rec) {
	}

public:
	virtual ~GroundRule() {
	}

	// Inspectors
	Lit head() const {
		return _head;
	}
	RuleType type() const {
		return _type;
	}
	bool recursive() const {
		return _recursive;
	}
	virtual bool isFalse() const = 0;
	virtual bool isTrue() const = 0;

	void head(Lit head) {
		_head = head;
	}
	void type(RuleType type) {
		_type = type;
	}
	void recursive(bool recursive) {
		_recursive = recursive;
	}
};

class PCGroundRule: public GroundRule {
ACCEPTBOTH(GroundRule)
private:
	litlist _body; // The literals in the body

public:
	// Constructors
	PCGroundRule(Lit head, RuleType type, const litlist& body, bool rec)
			: 	GroundRule(head, type, rec),
				_body(body) {
	}
	PCGroundRule(Lit head, PCTsBody* body, bool rec);
	PCGroundRule(const PCGroundRule& grb)
			: 	GroundRule(grb.head(), grb.type(), grb.recursive()),
				_body(grb._body) {
	}

	~PCGroundRule() {
	}

	// Inspectors
	const litlist& body() const {
		return _body;
	}
	litlist& body() {
		return _body;
	}
	void body(const litlist& body) {
		_body = body;
	}
	size_t size() const {
		return _body.size();
	}
	bool empty() const {
		return _body.empty();
	}
	Lit literal(size_t n) const {
		return _body[n];
	}
	bool isFalse() const {
		return (_body.empty() && type() == RuleType::DISJ);
	}
	bool isTrue() const {
		return (_body.empty() && type() == RuleType::CONJ);
	}
};

class AggGroundRule: public GroundRule {
ACCEPTBOTH(GroundRule)
private:
	double _bound; // The bound on the aggregate
	bool _lower; // If true, then bound=<agg(set), otherwise bound>=agg(set)
	SetId _setnr; // The id of the set of the aggregate
	AggFunction _aggtype; // The aggregate type (cardinality, sum, product, min, or max)

public:
	// Constructors
	AggGroundRule(Lit head, double bound, bool lower, SetId setnr, AggFunction at, bool rec)
			: 	GroundRule(head, RuleType::AGG, rec),
			  	_bound(bound),
			  	_lower(lower),
				_setnr(setnr),
				_aggtype(at)
				 {
	}
	AggGroundRule(Lit head, AggTsBody* body, bool rec);
	AggGroundRule(const AggGroundRule& grb)
			: 	GroundRule(grb.head(), RuleType::AGG, grb.recursive()),
			  	_bound(grb._bound),
			  	_lower(grb._lower),
				_setnr(grb._setnr),
				_aggtype(grb._aggtype)
				 {
	}

	~AggGroundRule() {
	}

	// Inspectors
	SetId setnr() const {
		return _setnr;
	}
	AggFunction aggtype() const {
		return _aggtype;
	}
	bool lower() const {
		return _lower;
	}
	double bound() const {
		return _bound;
	}
	bool isFalse() const {
		return false;
	}
	bool isTrue() const {
		return false;
	}
};

class GroundDefinition: public AbstractDefinition {
ACCEPTBOTH(AbstractDefinition)
private:
	DefId _id;
	GroundTranslator* _translator;
	std::map<int, GroundRule*> _rules;

public:
	GroundDefinition(DefId id, GroundTranslator* tr)
			: 	_id(id),
				_translator(tr) {
	}
	GroundDefinition* clone() const;
	void recursiveDelete();

	// Mutators
	void addPCRule(Lit head, const litlist& body, bool conj, bool recursive);
	void addAggRule(Lit head, SetId setnr, AggFunction aggtype, bool lower, double bound, bool recursive);

	DefId id() const {
		return _id;
	}

	size_t getNumberOfRules() const {
		return _rules.size();
	}

	std::map<int, GroundRule*>& rules() { return _rules; }
	const std::map<int, GroundRule*>& rules() const { return _rules; }
	bool hasRule(Atom head) const;

	GroundTranslator* translator() const {
		return _translator;
	}

	// Debugging
	std::ostream& put(std::ostream&) const;
};

/****************************************
 Ground nested fixpoint definitions
 ****************************************/

class GroundFixpDef: public AbstractDefinition {
private:
//		std::map<int,GroundRule*>	_rules;		// the direct subrules
//		std::vector<GroundFixpDef*>		_subdefs;	// the direct subdefinitions
public:
	//TODO
};

/*******************
 * CP reifications
 *******************/

/**
 * class CPReification
 * 		This class represents CP constraints.
 */
class CPReification {
ACCEPTNONMUTATING()
public:
	Lit _head;
	CPTsBody* _body;
	CPReification(Lit head, CPTsBody* body)
			: 	_head(head),
				_body(body) {
	}
	virtual ~CPReification();
	std::string print(unsigned int spaces = 0) const;
};

struct GroundTerm {
	bool isVariable;
	union {
		const DomainElement* _domelement;
		VarId _varid;
	};
	GroundTerm(const DomainElement* domel)
			: 	isVariable(false),
				_domelement(domel) {
		//Assert(domel!=NULL); //Correct?
	}
	GroundTerm(const VarId& varid)
			: 	isVariable(true),
				_varid(varid) {
	}
	friend bool operator==(const GroundTerm&, const GroundTerm&);
	friend bool operator<(const GroundTerm&, const GroundTerm&);
	std::ostream& put(std::ostream&) const;
};

typedef std::vector<GroundTerm> termlist;

/**
 * Set corresponding to a tseitin.
 */
class TsSet {
private:
	litlist _setlits; // All literals in the ground set
	weightlist _litweights; // For each literal a corresponding weight
	weightlist _trueweights; // The weights corresponding to true literals in the set
	termlist _cpvars; // CP variable ids corresponding to true literals in the set
public:
	// Modifiers
	void setWeight(size_t n, Weight w) {
		_litweights[n] = w;
	}
	void removeLit(size_t n) {
		_setlits[n] = _setlits.back();
		_litweights[n] = _litweights.back();
		_setlits.pop_back();
		_litweights.pop_back();
	}
	// Inspectors
	const litlist& literals() const {
		return _setlits;
	}
	const weightlist& weights() const {
		return _litweights;
	}
	const weightlist& trueweights() const {
		return _trueweights;
	}
	const termlist& cpvars() const {
		return _cpvars;
	}
	size_t size() const {
		return _setlits.size();
	}
	bool empty() const {
		return _setlits.empty();
	}
	Lit literal(size_t n) const {
		return _setlits[n];
	}
	Weight weight(size_t n) const {
		return _litweights[n];
	}
	friend class GroundTranslator;
};

/**
 * A complete definition of a tseitin atom.
 */
class TsBody {
protected:
	TsType _type; // the type of "tseitin definition"
	TsBody(TsType type)
			: _type(type) {
	}
public:
	virtual ~TsBody() {
	}

	void type(TsType type) {
		_type = type;
	}
	TsType type() const {
		return _type;
	}

	virtual bool operator==(const TsBody& rhs) const;
	virtual bool operator<(const TsBody& rhs) const;
	bool operator>(const TsBody& rhs) const;
	virtual void put(std::ostream& stream) const;
};

class PCTsBody: public TsBody {
private:
	litlist _body; // the literals in the subformula replaced by the tseitin
	bool _conj; // if true, the replaced subformula is the conjunction of the literals in _body,
				// if false, the replaced subformula is the disjunction of the literals in _body
public:
	PCTsBody(TsType type, const litlist& body, bool conj)
			: 	TsBody(type),
				_body(body),
				_conj(conj) {
	}
	litlist body() const {
		return _body;
	}
	size_t size() const {
		return _body.size();
	}
	Lit literal(size_t n) const {
		return _body[n];
	}
	bool conj() const {
		return _conj;
	}
	bool operator==(const TsBody& rhs) const;
	bool operator<(const TsBody& rhs) const;
};

class AggTsBody: public TsBody {
private:
	SetId _setnr;
	AggFunction _aggtype;
	bool _lower; //comptype == CompType::LT
	double _bound; //The other side of the equation.
	//If _lower is true this means bound=<agg{_setnr}
	//If _lower is false this means bound>=agg{_setnr}
public:
	AggTsBody(TsType type, double bound, bool lower, AggFunction at, SetId setnr)
			: 	TsBody(type),
				_setnr(setnr),
				_aggtype(at),
				_lower(lower),
				_bound(bound) {
	}
	SetId setnr() const {
		return _setnr;
	}
	AggFunction aggtype() const {
		return _aggtype;
	}
	bool lower() const {
		return _lower;
	}
	double bound() const {
		return _bound;
	}
	void setBound(double bound) {
		_bound = bound;
	}
	bool operator==(const TsBody& rhs) const;
	bool operator<(const TsBody& rhs) const;
};

class DenotingTsBody: public TsBody {
private:
	VarId varid;
public:
	DenotingTsBody(TsType type, VarId varid)
			: 	TsBody(type),
			  	varid(varid) {
	}
	VarId getVarId() const {
		return varid;
	}
	bool operator==(const TsBody& rhs) const;
	bool operator<(const TsBody& rhs) const;
};

class CPTerm;

/**
 * A bound in a CP constraint can be an integer or a CP variable.
 */
struct CPBound {
public:
	bool _isvarid;
	union {
		int _bound;
		VarId _varid;
	};
	CPBound(const int& bound)
			: 	_isvarid(false),
				_bound(bound) {
	}
	CPBound(const VarId& varid)
			: 	_isvarid(true),
				_varid(varid) {
	}
	virtual ~CPBound() {
	}
	bool operator==(const CPBound& rhs) const;
	bool operator<(const CPBound& rhs) const;
	virtual void put(std::ostream& stream) const;
};

/**
 * Tseitin body consisting of a CP constraint.
 */
class CPTsBody: public TsBody {
private:
	CPTerm* _left;
	CompType _comp;
	CPBound _right;
public:
	CPTsBody(TsType type, CPTerm* left, CompType comp, const CPBound& right)
			: 	TsBody(type),
				_left(left),
				_comp(comp),
				_right(right) {
		Assert(type!=TsType::RULE);
		// Constraints within recursive definitions cannot be handled atm.
	}
	~CPTsBody() {
	}
	CPTerm* left() const {
		return _left;
	}
	void left(CPTerm* newleft) {
		_left = newleft;
	}
	CompType comp() const {
		return _comp;
	}
	CPBound right() const {
		return _right;
	}
	bool operator==(const TsBody& rhs) const;
	bool operator<(const TsBody& rhs) const;
	virtual void put(std::ostream& stream) const;
};

struct LazyInstantiation;

class LazyTsBody: public TsBody {
public:
	LazyInstantiation* inst;

public:
	LazyTsBody(LazyInstantiation* inst, TsType type)
			: 	TsBody(type),
				inst(inst) {
	}
	//FIXME bool operator==(const TsBody& rhs) const;
	//FIXME bool operator<(const TsBody& rhs) const;

	void notifyTheoryOccurence();
};

/* Sets and terms that will be handled by a constraint solver */

class CPTerm {
	VISITORS()
protected:
	CPTerm() {
	}
public:
	virtual ~CPTerm() {
	}
	virtual void accept(TheoryVisitor*) const = 0;
	virtual bool operator==(const CPTerm& body) const;
	virtual bool operator<(const CPTerm& body) const;
	bool operator>(const CPTerm& rhs) const;
	virtual void put(std::ostream& stream) const = 0;
	virtual std::vector<VarId> getVarIds() const = 0;
};

/**
 * CP term consisting of one CP variable.
 */
class CPVarTerm: public CPTerm {
ACCEPTNONMUTATING()
private:
	VarId _varid;
public:
	CPVarTerm(const VarId& varid)
			: _varid(varid) {
	}
	const VarId& varid() const {
		return _varid;
	}
	bool operator==(const CPTerm&) const;
	bool operator<(const CPTerm&) const;
	virtual void put(std::ostream& stream) const;
	virtual std::vector<VarId> getVarIds() const {
		return {_varid};
	}
};

/**
 * CP term consisting of a weighted sum of CP variables and an operator type.
 * Represents sum (all weighted), prod (one weight) and min and max (no weights)
 */
class CPSetTerm: public CPTerm {
ACCEPTNONMUTATING()
private:
	AggFunction _type;
	litlist _conditions;
	varidlist _varids;
	intweightlist _weights;
public:
	CPSetTerm(AggFunction type, const litlist& conditions, const varidlist& varids, const intweightlist& weights)
			: 	_type(type),
			  	_conditions(conditions),
			  	_varids(varids),
				_weights(weights) {
		Assert(conditions.size()==varids.size());
		switch(type){
		case AggFunction::CARD:
			throw IdpException("Invalid code path");
		case AggFunction::SUM:
			Assert(weights.size()==varids.size());
			break;
		case AggFunction::PROD:
			Assert(weights.size()==1);
			break;
		case AggFunction::MIN:
			Assert(weights.size()==0);
			break;
		case AggFunction::MAX:
			Assert(weights.size()==0);
			break;
		}
	}
	AggFunction type() const {
		return _type;
	}
	const litlist& conditions() const {
		return _conditions;
	}
	const varidlist& varids() const {
		return _varids;
	}
	const intweightlist& weights() const {
		return _weights;
	}
	void conditions(const litlist& conditions) {
		_conditions = conditions;
	}
	void varids(const varidlist& newids) {
		_varids = newids;
	}
	void weights(const intweightlist& newweights) {
		_weights = newweights;
	}
	bool operator==(const CPTerm&) const;
	bool operator<(const CPTerm&) const;
	virtual void put(std::ostream&) const;
	virtual std::vector<VarId> getVarIds() const {
		return varids();
	}
};
