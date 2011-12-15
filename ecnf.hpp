/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ECNF_HPP
#define ECNF_HPP

#include "common.hpp"
#include <ostream>

#include "theory.hpp"
#include "visitors/TheoryVisitor.hpp" // TODO calls should go into cpp
#include "visitors/TheoryMutatingVisitor.hpp" // TODO calls should go into cpp
#include "visitors/VisitorFriends.hpp"
class GroundTranslator;
class GroundTermTranslator;
class PCTsBody;
class AggTsBody;
class CPTsBody;
class SortTable;

class LazyQuantGrounder;
class DomElemContainer;
class DomainElement;
class InstGenerator;

typedef unsigned int VarId;

/*********************
 Ground clauses
 *********************/

typedef litlist GroundClause;

class GroundSet {
	ACCEPTNONMUTATING()
private:
	unsigned int _setnr;
	litlist _setlits; // All literals in the ground set
	std::vector<double> _litweights; // For each literal a corresponding weight

public:
	// Constructors
	GroundSet() {
	}
	GroundSet(int setnr, const litlist& s, const std::vector<double>& lw) :
			_setnr(setnr), _setlits(s), _litweights(lw) {
	}

	// Inspectors
	unsigned int setnr() const {
		return _setnr;
	}
	unsigned int size() const {
		return _setlits.size();
	}
	int literal(unsigned int n) const {
		return _setlits[n];
	}
	double weight(unsigned int n) const {
		return (not _litweights.empty()) ? _litweights[n] : 1;
	}
	bool weighted() const {
		return not _litweights.empty();
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
	int _head; // the head literal
	TsType _arrow; // the relation between head and aggregate expression
				   // INVARIANT: this should never be TsType::RULE
	double _bound; // the bound
	bool _lower; // true iff the bound is a lower bound for the aggregate
	AggFunction _type; // the aggregate function
	int _set; // the set id

public:
	// Constructors
	GroundAggregate(AggFunction t, bool l, TsType e, int h, int s, double b) :
			_head(h), _arrow(e), _bound(b), _lower(l), _type(t), _set(s) {
		Assert(e != TsType::RULE);
	}
	GroundAggregate(const GroundAggregate& a) :
			_head(a._head), _arrow(a._arrow), _bound(a._bound), _lower(a._lower), _type(a._type), _set(a._set) {
		Assert(a._arrow != TsType::RULE);
	}
	GroundAggregate() {
	}

	// Inspectors
	int head() const {
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
	unsigned int setnr() const {
		return _set;
	}
};

/*************************
 Ground definitions
 *************************/

// Enumeration type for rules
enum RuleType {
	RT_CONJ, RT_DISJ, RT_AGG
};

/**
 * class GroundRule
 *		This class represents a ground rule body, where literals are represented by integers.
 */
class GroundRule {
	ACCEPTDECLAREBOTH(GroundRule)
private:
	int _head;
	RuleType _type; // The rule type (disjunction, conjunction, or aggregate
	bool _recursive; // True iff the rule body contains defined literals

protected:
	GroundRule(int head, RuleType type, bool rec) :
			_head(head), _type(type), _recursive(rec) {
	}

public:
	virtual ~GroundRule() {
	}

	// Inspectors
	int head() const {
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

	void head(int head) {
		_head = head;
	}
	void type(RuleType type) {
		_type = type;
	}
	void recursive(bool recursive) {
		_recursive = recursive;
	}
};

typedef std::vector<Lit> litlist;

class PCGroundRule: public GroundRule {
	ACCEPTBOTH(GroundRule)
private:
	litlist _body; // The literals in the body

public:
	// Constructors
	PCGroundRule(int head, RuleType type, const litlist& body, bool rec) :
			GroundRule(head, type, rec), _body(body) {
	}
	PCGroundRule(int head, PCTsBody* body, bool rec);
	PCGroundRule(const PCGroundRule& grb) :
			GroundRule(grb.head(), grb.type(), grb.recursive()), _body(grb._body) {
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
	unsigned int size() const {
		return _body.size();
	}
	bool empty() const {
		return _body.empty();
	}
	int literal(unsigned int n) const {
		return _body[n];
	}
	bool isFalse() const {
		return (_body.empty() && type() == RT_DISJ);
	}
	bool isTrue() const {
		return (_body.empty() && type() == RT_CONJ);
	}
};

class AggGroundRule: public GroundRule {
	ACCEPTBOTH(GroundRule)
private:
	int _setnr; // The id of the set of the aggregate
	AggFunction _aggtype; // The aggregate type (cardinality, sum, product, min, or max)
	bool _lower; // True iff the bound is a lower bound
	double _bound; // The bound on the aggregate

public:
	// Constructors
	AggGroundRule(int head, int setnr, AggFunction at, bool lower, double bound, bool rec) :
			GroundRule(head, RT_AGG, rec), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) {
	}
	AggGroundRule(int head, AggTsBody* body, bool rec);
	AggGroundRule(const AggGroundRule& grb) :
			GroundRule(grb.head(), RT_AGG, grb.recursive()), _setnr(grb._setnr), _aggtype(grb._aggtype), _lower(grb._lower), _bound(grb._bound) {
	}

	~AggGroundRule() {
	}

	// Inspectors
	unsigned int setnr() const {
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
	unsigned int _id;
	GroundTranslator* _translator;
	std::map<int, GroundRule*> _rules;

public:
	// Constructors
	GroundDefinition(unsigned int id, GroundTranslator* tr) :
			_id(id), _translator(tr) {
	}
	GroundDefinition* clone() const;
	void recursiveDelete();

	// Mutators
	void addTrueRule(int head);
	void addFalseRule(int head);
	void addPCRule(int head, const litlist& body, bool conj, bool recursive);
	void addAggRule(int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive);

	unsigned int id() const {
		return _id;
	}

	unsigned int getNumberOfRules() const {
		return _rules.size();
	}

	typedef std::map<int, GroundRule*>::iterator ruleiterator;
	ruleiterator begin() {
		return _rules.begin();
	}
	ruleiterator end() {
		return _rules.end();
	}

	GroundTranslator* translator() const {
		return _translator;
	}

	typedef std::map<int, GroundRule*>::const_iterator const_ruleiterator;
	const_ruleiterator begin() const {
		return _rules.cbegin();
	}
	const_ruleiterator end() const {
		return _rules.cend();
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

/**********************
 CP reifications
 **********************/

/**
 * class CPReification
 * 		This class represents CP constraints.
 */
class CPReification { //TODO ?
	ACCEPTNONMUTATING()
public:
	int _head;
	CPTsBody* _body;
	CPReification(int head, CPTsBody* body) :
			_head(head), _body(body) {
	}
	std::string toString(unsigned int spaces = 0) const;
};

struct GroundTerm {
	bool isVariable;
	union {
		const DomainElement* _domelement;
		VarId _varid;
	};
	GroundTerm() {
	}
	GroundTerm(const DomainElement* domel) :
			isVariable(false), _domelement(domel) {
	}
	GroundTerm(const VarId& varid) :
			isVariable(true), _varid(varid) {
	}
	friend bool operator==(const GroundTerm&, const GroundTerm&);
	friend bool operator<(const GroundTerm&, const GroundTerm&);
};

/**
 * Set corresponding to a tseitin.
 */
class TsSet {
private:
	std::vector<int> _setlits; // All literals in the ground set
	std::vector<double> _litweights; // For each literal a corresponding weight
	std::vector<double> _trueweights; // The weights of the true literals in the set
public:
	// Modifiers
	void setWeight(unsigned int n, double w) {
		_litweights[n] = w;
	}
	void removeLit(unsigned int n){
		_setlits[n] = _setlits.back();
		_litweights[n] = _litweights.back();
		_setlits.pop_back();
		_litweights.pop_back();
	}
	// Inspectors
	std::vector<int> literals() const {
		return _setlits;
	}
	std::vector<double> weights() const {
		return _litweights;
	}
	std::vector<double> trueweights() const {
		return _trueweights;
	}
	unsigned int size() const {
		return _setlits.size();
	}
	bool empty() const {
		return _setlits.empty();
	}
	int literal(unsigned int n) const {
		return _setlits[n];
	}
	double weight(unsigned int n) const {
		return _litweights[n];
	}
	friend class GroundTranslator;
};

/**
 * A complete definition of a tseitin atom.
 */
class TsBody {
protected:
	const TsType _type; // the type of "tseitin definition"
	TsBody(TsType type) :
			_type(type) {
	}
public:
	virtual ~TsBody() {
	}
	TsType type() const {
		return _type;
	}

	virtual bool operator==(const TsBody& rhs) const;
	virtual bool operator<(const TsBody& rhs) const;
	bool operator>(const TsBody& rhs) const {
		return not (*this == rhs && *this < rhs);
	}
};

class PCTsBody: public TsBody {
private:
	std::vector<int> _body; // the literals in the subformula replaced by the tseitin
	bool _conj; // if true, the replaced subformula is the conjunction of the literals in _body,
				// if false, the replaced subformula is the disjunction of the literals in _body
public:
	PCTsBody(TsType type, const std::vector<int>& body, bool conj) :
			TsBody(type), _body(body), _conj(conj) {
	}
	std::vector<int> body() const {
		return _body;
	}
	unsigned int size() const {
		return _body.size();
	}
	int literal(unsigned int n) const {
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
	int _setnr;
	AggFunction _aggtype;
	bool _lower; //comptype == CompType::LT
	double _bound; //The other side of the equation.
	//If _lower is true this means CARD{_setnr}=<_bound
	//If _lower is false this means CARD{_setnr}>=_bound
public:
	AggTsBody(TsType type, double bound, bool lower, AggFunction at, int setnr) :
			TsBody(type), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) {
	}
	int setnr() const {
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
	CPBound(const int& bound) :
			_isvarid(false), _bound(bound) {
	}
	CPBound(const VarId& varid) :
			_isvarid(true), _varid(varid) {
	}
	bool operator==(const CPBound& rhs) const;
	bool operator<(const CPBound& rhs) const;
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
	CPTsBody(TsType type, CPTerm* left, CompType comp, const CPBound& right) :
			TsBody(type), _left(left), _comp(comp), _right(right) {
	}
	CPTerm* left() const {
		return _left;
	}
	CompType comp() const {
		return _comp;
	}
	CPBound right() const {
		return _right;
	}
	bool operator==(const TsBody& rhs) const;
	bool operator<(const TsBody& rhs) const;
};

typedef std::pair<const DomElemContainer*, const DomainElement*> dominst;
typedef std::vector<dominst> dominstlist;

struct ResidualAndFreeInst {
	InstGenerator* generator;
	Lit residual;
	dominstlist freevarinst;

	/*	bool operator==(const ResidualAndFreeInst& rhs) const {
	 return rhs.residual == residual && freevarinst == rhs.freevarinst;
	 }*/
};

class LazyTsBody: public TsBody {
private:
	unsigned int id_;
	LazyQuantGrounder const* const grounder_;
	ResidualAndFreeInst* inst;

public:
	LazyTsBody(int id, LazyQuantGrounder const* const grounder, ResidualAndFreeInst* inst, TsType type) :
			TsBody(type), id_(id), grounder_(grounder), inst(inst) {
	}
	//FIXME bool operator==(const TsBody& rhs) const;
	//FIXME bool operator<(const TsBody& rhs) const;

	unsigned int id() const {
		return id_;
	}

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
	bool operator>(const CPTerm& rhs) const {
		return not (*this == rhs && *this < rhs);
	}
};

/**
 * CP term consisting of one CP variable.
 */
class CPVarTerm: public CPTerm {
	ACCEPTNONMUTATING()
private:
	VarId _varid;
public:
	CPVarTerm(const VarId& varid) :
			_varid(varid) {
	}

	const VarId& varid() const {
		return _varid;
	}

	bool operator==(const CPTerm&) const;
	bool operator<(const CPTerm&) const;
};

/**
 * CP term consisting of a sum of CP variables.
 */
class CPSumTerm: public CPTerm {
	ACCEPTNONMUTATING()
private:
	std::vector<VarId> _varids;
public:
	CPSumTerm(const VarId& left, const VarId& right) :
			_varids(2) {
		_varids[0] = left;
		_varids[1] = right;
	}
	CPSumTerm(const std::vector<VarId>& varids) :
			_varids(varids) {
	}

	const std::vector<VarId>& varids() const {
		return _varids;
	}
	void varids(const std::vector<VarId>& newids) {
		_varids = newids;
	}

	bool operator==(const CPTerm&) const;
	bool operator<(const CPTerm&) const;
};

/**
 * CP term consisting of a weighted sum of CP variables.
 */
class CPWSumTerm: public CPTerm {
	ACCEPTNONMUTATING()
private:
	std::vector<VarId> _varids;
	std::vector<int> _weights;
public:
	CPWSumTerm(const std::vector<VarId>& varids, const std::vector<int>& weights) :
			_varids(varids), _weights(weights) {
	}

	const std::vector<VarId>& varids() const {
		return _varids;
	}
	const std::vector<int>& weights() const {
		return _weights;
	}

	bool operator==(const CPTerm&) const;
	bool operator<(const CPTerm&) const;
};

#endif
