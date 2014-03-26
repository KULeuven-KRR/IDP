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

#include <list>
#include <algorithm>
#include "common.hpp"
#include "vocabulary/VarCompare.hpp"

using std::string;
using std::stringstream;
using std::set;
using std::list;
using namespace std;

class Formula;
class PrologVariable;
class PrologTerm;
class FormulaClauseVisitor;

class FormulaClause {
protected:
	FormulaClause* _parent;
	list<PrologTerm*> _arguments;
	list<PrologVariable*> _variables;
	set<PrologVariable*> _instantiatedVariables;
	string _name;
	Formula* _formula;
	bool _numeric;
	set<PrologVariable*> _inputvars_to_check;
	set<PrologVariable*> _outputvars_to_check;
public:
	FormulaClause(const string& name, FormulaClause* parent = NULL)
	: 	_parent(parent),
				_name(name),
				_formula(NULL),
				_numeric(false){
	}
	virtual ~FormulaClause(){
		// TODO what should be deleted?
	}

	virtual void accept(FormulaClauseVisitor*) = 0;
	virtual PrologTerm* asTerm();
	list<PrologVariable*>& variables() {
		return _variables;
	}
	void variables(list<PrologVariable*> variables) {
		_variables = variables;
	}
	set<PrologVariable*>& instantiatedVariables() {
		return _instantiatedVariables;
	}
	void instantiatedVariables(set<PrologVariable*> variables) {
		_instantiatedVariables = variables;
	}
	FormulaClause* parent() {
		return _parent;
	}
	void parent(FormulaClause* parent) {
		_parent = parent;
		if (parent != NULL) {
			parent->addChild(this);
		}
	}
	void arguments(list<PrologTerm*> arguments) {
		_arguments = arguments;
	}
	virtual void addChild(FormulaClause*) {
	}
	virtual void removeChild(FormulaClause*) {

	}
	virtual void addArgument(PrologTerm* pt) {
		_arguments.push_back(pt);
	}

	void addArguments(list<PrologTerm*> s) {
		for (auto i = s.begin(); i != s.end(); ++i) {
			addArgument(*i);
		}
	}

	void addVariable(PrologVariable* v) {
		if (std::find(_variables.begin(), _variables.end(), v) == _variables.end()) {
			_variables.push_back(v);
		}
	}

	void addVariables(list<PrologVariable*> l) {
		for (auto v = l.begin(); v != l.end(); ++v) {
			addVariable(*v);
		}
	}

	virtual set<PrologVariable*> variablesRequiringInstantiation();
	virtual const string name() const {
		return _name;
	}
	void name(string name) {
		_name = name;
	}
	string name() {
		return _name;
	}

	void formula(Formula* f) {
		_formula = f;
	}
	Formula* formula() {
		return _formula;
	}
	bool numeric() {
		return _numeric;
	}
	void numeric(bool b) {
		_numeric = b;
	}

	set<PrologVariable*>& inputvarsToCheck() {
		return _inputvars_to_check;
	}

	set<PrologVariable*>& outputvarsToCheck() {
		return _outputvars_to_check;
	}

	void addInputvarToCheck(PrologVariable* v) {
		_inputvars_to_check.insert(v);
	}

	void addInputvarsToCheck(set<PrologVariable*> vs) {
		// TODO: also allow interface with list?
		for (auto v = vs.begin(); v != vs.end(); ++v) {
			addInputvarToCheck(*v);
		}
	}

	void addOutputvarToCheck(PrologVariable* v) {
		_outputvars_to_check.insert(v);
	}

	void addOutputvarsToCheck(set<PrologVariable*> vs) {
		for (auto v = vs.begin(); v != vs.end(); ++v) {
			addOutputvarToCheck(*v);
		}
	}
	virtual void close() {

	}
};

class PrologTerm: public FormulaClause {
	friend std::ostream& operator<<(std::ostream& output, const PrologTerm& pt);
protected:
	bool _infix;
	bool _sign;
	bool _tabled;
	bool _fact;
	bool _numerical_operation;
public:
	PrologTerm(string functor, list<PrologTerm*> args)
			: 	FormulaClause(functor),
				_infix(false),
				_sign(true),
				_tabled(false),
				_fact(false) ,
				_numerical_operation(false) {
		_arguments = args;
	}
	PrologTerm(string functor)
			: 	FormulaClause(functor),
				_infix(false),
				_sign(true),
				_tabled(false),
				_fact(false),
				_numerical_operation(false) {
	}
	void infix(bool b) {
		_infix = b;
	}
	bool infix() {
		return _infix;
	}
	void fact(bool b) {
		_fact = b;
	}
	bool fact() {
		return _fact;
	}
	void sign(bool b) {
		_sign = b;
	}
	bool sign() {
		return _sign;
	}
	void tabled(bool b) {
		_tabled = b;
	}
	bool tabled() {
		return _tabled;
	}
	int arity() {
		return _arguments.size();
	}
	string* namep() {
		return &_name;
	}
	const list<PrologTerm*>& arguments() const {
		return _arguments;
	}
	void accept(FormulaClauseVisitor*) {
	}

	PrologTerm* asTerm() {
		return this;
	}

	string asString() {
		stringstream ss;
		ss << *this;
		return ss.str();
	}

	void addChild(FormulaClause* f) {
		addArgument((PrologTerm*) f);
	}

	set<PrologVariable*> variablesRequiringInstantiation();
	static list<PrologTerm*> vars2terms(list<PrologVariable*>);
	static bool term_ordering(PrologTerm* first, PrologTerm* second);

	virtual std::ostream& put(std::ostream&) const;

};

class PrologVariable: public PrologTerm {
	friend std::ostream& operator<<(std::ostream& output, const PrologVariable& pv);
private:

	string _type;

public:
	PrologVariable(string name, string type = "")
			: 	PrologTerm(name),
				_type(type) {
	}

	string type() {
		return _type;
	}
	void type(string type) {
		_type = type;
	}

	PrologTerm* instantiation();
	virtual std::ostream& put(std::ostream&) const;
};

class PrologConstant: public PrologTerm {
	friend std::ostream& operator<<(std::ostream& output, const PrologConstant& pc);
public:
	PrologConstant(string name)
			: PrologTerm(name) {
	}
	virtual std::ostream& put(std::ostream&) const;
};

class CompositeFormulaClause: public FormulaClause {
protected:
	list<FormulaClause*> _children;
public:
	CompositeFormulaClause(string name, list<FormulaClause*>& children)
			: 	FormulaClause(name),
				_children(children) {
	}
	CompositeFormulaClause(string name)
			: 	FormulaClause(name),
				_children() {
	}
	list<FormulaClause*>& children() {
		return _children;
	}
	void children(list<FormulaClause*>& children) {
		_children = children;
	}
	void addChild(FormulaClause* child) {
		_children.push_back(child);
	}
	void removeChild(FormulaClause* child) {
		_children.remove(child);
	}
};

class QuantifiedFormulaClause: public FormulaClause {
protected:
	set<PrologVariable*> _quantifiedVariables;
	FormulaClause* _child;
public:
	QuantifiedFormulaClause(string name, set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: 	FormulaClause(name),
				_quantifiedVariables(quantifiedVariables),
				_child(child) {
	}
	QuantifiedFormulaClause(string name, set<PrologVariable*> quantifiedVariables)
			: 	FormulaClause(name),
				_quantifiedVariables(quantifiedVariables),
				_child() {
	}
	QuantifiedFormulaClause(string name)
			: 	FormulaClause(name),
				_quantifiedVariables(),
				_child() {
	}
	set<PrologVariable*>& quantifiedVariables() {
		return _quantifiedVariables;
	}
	FormulaClause* child() {
		return _child;
	}
	void child(FormulaClause* child) {
		_child = child;
	}
	void quantifiedVariables(set<PrologVariable*> variables) {
		_quantifiedVariables = variables;
	}
	void addQuantifiedVar(PrologVariable* var) {
		_quantifiedVariables.insert(var);
	}
	void addChild(FormulaClause* f) {
		_child = f;
	}
	set<PrologVariable*> variablesRequiringInstantiation();
};

class ExistsClause: public QuantifiedFormulaClause {
public:
	ExistsClause(string name, set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: QuantifiedFormulaClause(name, quantifiedVariables, child) {
	}
	ExistsClause(string name, set<PrologVariable*> quantifiedVariables)
			: QuantifiedFormulaClause(name, quantifiedVariables) {
	}
	ExistsClause(string name)
			: QuantifiedFormulaClause(name) {
	}
	void accept(FormulaClauseVisitor* v);

};

class ForallClause: public QuantifiedFormulaClause {
public:
	ForallClause(string name, set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: QuantifiedFormulaClause(name, quantifiedVariables, child) {
	}
	ForallClause(string name, set<PrologVariable*> quantifiedVariables)
			: QuantifiedFormulaClause(name, quantifiedVariables) {
	}
	ForallClause(string name)
			: QuantifiedFormulaClause(name) {
	}
	void accept(FormulaClauseVisitor*);
};

class AndClause: public CompositeFormulaClause {
public:
	AndClause(string name, list<FormulaClause*>& children)
			: CompositeFormulaClause(name, children) {
	}
	AndClause(string name)
			: CompositeFormulaClause(name) {
	}
	void accept(FormulaClauseVisitor*);
};

class OrClause: public CompositeFormulaClause {
public:
	OrClause(string name, list<FormulaClause*>& children)
			: CompositeFormulaClause(name, children) {
	}
	OrClause(string name)
			: CompositeFormulaClause(name) {
	}
	void accept(FormulaClauseVisitor*);
};

class XSBToIDPTranslator;

class SetExpression: public FormulaClause {
private:
	bool _twovaluedbody;
	PrologVariable* _var;
	set<PrologVariable*> _quantvars;
public:
	SetExpression(string name, XSBToIDPTranslator* translator, bool twovaluedbody);
	PrologVariable* var() {
		return _var;
	}
	void close() {
		FormulaClause::addArgument(_var);
	}
	void quantifiedVariables(set<PrologVariable*> v) {
		_quantvars = v;
	}

	set<PrologVariable*>& quantifiedVariables() {
		return _quantvars;
	}
	bool hasTwoValuedBody() {
		return _twovaluedbody;
	}
};

class EnumSetExpression: public SetExpression {
private:
	FormulaClause* _last;
	std::map<FormulaClause*, PrologTerm*> _set;
public:
	EnumSetExpression(string name, XSBToIDPTranslator* translator, bool twovaluedbody)
			: SetExpression(name, translator, twovaluedbody),
			  _last(NULL),
			  _set() {}
	void addChild(FormulaClause* f) {
		_last = f;
	}
	void addArgument(PrologTerm* t) {
		_set.insert( { _last, t });
	}
	std::map<FormulaClause*, PrologTerm*>& set() {
		return _set;
	}
	void accept(FormulaClauseVisitor*);
};

class QuantSetExpression: public SetExpression {
private:
	PrologTerm* _term;
	FormulaClause* _clause;
public:
	QuantSetExpression(string name, XSBToIDPTranslator* translator, bool twovaluedbody)
			: SetExpression(name, translator, twovaluedbody),
			  _term(NULL),
			  _clause(NULL) {}
	void addChild(FormulaClause* f) {
		_clause = f;
	}
	void addArgument(PrologTerm* t) {
		FormulaClause::addArgument(t);
		_term = t;
	}
	FormulaClause* clause() {
		return _clause;
	}
	PrologTerm* term() {
		return _term;
	}
	void accept(FormulaClauseVisitor*);

};

class AggregateTerm: public FormulaClause {
private:
	SetExpression* _set;
	string _agg_type;
	PrologVariable* _result;
public:
	AggregateTerm(string name, XSBToIDPTranslator* translator);
	void accept(FormulaClauseVisitor*);
	void agg_type(string type) {
		_agg_type = type;
	}
	string agg_type() {
		return _agg_type;
	}
	SetExpression* set() {
		return _set;
	}
	void addChild(FormulaClause* f) {
		_set = (SetExpression*) f;
	}
	void close() {
		FormulaClause::addArgument(_result);
	}
	PrologVariable* result() {
		return _result;
	}
};

class AggregateClause: public FormulaClause {
private:
	PrologVariable* _aggvar;
	string _comparison_type;
	AggregateTerm* _aggterm;
	PrologTerm* _term;
public:
	AggregateClause(string name)
			: 	FormulaClause(name),
				_aggvar(),
				_comparison_type(),
				_aggterm(),
				_term() {
	}
	PrologVariable* aggvar() {
		return _aggvar;
	}
	void aggvar(PrologVariable* v) {
		_aggvar = v;
	}
	void comparison_type(string t) {
		_comparison_type = t;
	}
	string comparison_type() {
		return _comparison_type;
	}
	void addChild(FormulaClause* f) {
		_aggterm = (AggregateTerm *) f;
	}
	void addArgument(PrologTerm* t) {
		FormulaClause::addArgument(t);
		_term = t;
	}
	PrologTerm* term() {
		return _term;
	}
	AggregateTerm* aggterm() {
		return _aggterm;
	}
	void accept(FormulaClauseVisitor*);
	//PrologTerm* asTerm();
};
