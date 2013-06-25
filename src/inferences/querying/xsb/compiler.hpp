#pragma once

#include <list>
#include <stack>
#include <set>
#include <string>
#include <map>
#include <algorithm>

#include "theory/theory.hpp"
#include "theory/Sets.hpp"
#include "vocabulary/vocabulary.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "structure/Structure.hpp"

class PrologTerm;

std::string address(int);
std::string as_string(int);
std::string as_string(PrologTerm*);
std::string strip(std::string);
std::string var_name(std::string);
std::string term_name(std::string);
std::string transformIntoTermName(std::string);
std::string filter(std::string);
std::string domainelement_idp(std::string);
std::string domainelement_prolog(const DomainElement*);

class PrologVariable;
class SymbolClause;

class FormulaClauseVisitor;

class FormulaClause {
protected:
	FormulaClause* _parent;
	std::list<PrologTerm*> _arguments;
	std::list<PrologVariable*> _variables;
	std::set<PrologVariable*> _instantiatedVariables;
	std::string _name;
	Formula* _formula;
	bool _numeric;
	std::set<PrologVariable*> _inputvars_to_check;
	std::set<PrologVariable*> _outputvars_to_check;
public:
	FormulaClause(FormulaClause* parent)
			: 	_parent(parent),
				_name(NULL),
				_formula(NULL),
				_numeric(false) {
	}
	FormulaClause(const std::string& name = "")
			: 	_parent(NULL),
				_name(name),
				_formula(NULL),
				_numeric(false){
	}
	virtual ~FormulaClause(){
		// TODO what should be deleted?
	}

	virtual void accept(FormulaClauseVisitor*) = 0;
	virtual PrologTerm* asTerm();
	std::list<PrologVariable*>& variables() {
		return _variables;
	}
	void variables(std::list<PrologVariable*> variables) {
		_variables = variables;
	}
	std::set<PrologVariable*>& instantiatedVariables() {
		return _instantiatedVariables;
	}
	void instantiatedVariables(std::set<PrologVariable*> variables) {
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
	void arguments(std::list<PrologTerm*> arguments) {
		_arguments = arguments;
	}
	virtual void addChild(FormulaClause*) {
	}
	virtual void removeChild(FormulaClause*) {

	}
	virtual void addArgument(PrologTerm* pt) {
		_arguments.push_back(pt);
	}

	void addArguments(std::list<PrologTerm*> s) {
		for (auto i = s.begin(); i != s.end(); ++i) {
			addArgument(*i);
		}
	}

	void addVariable(PrologVariable* v) {
		if (find(_variables.begin(), _variables.end(), v) == _variables.end()) {
			_variables.push_back(v);
		}
	}

	void addVariables(std::list<PrologVariable*> l) {
		for (auto v = l.begin(); v != l.end(); ++v) {
			addVariable(*v);
		}
	}

	virtual std::set<PrologVariable*> variablesRequiringInstantiation();
	virtual const std::string name() const {
		return _name;
	}
	void name(std::string name) {
		_name = name;
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

	std::set<PrologVariable*>& inputvarsToCheck() {
		return _inputvars_to_check;
	}

	std::set<PrologVariable*>& outputvarsToCheck() {
		return _outputvars_to_check;
	}

	void addInputvarToCheck(PrologVariable* v) {
		_inputvars_to_check.insert(v);
	}

	void addInputvarsToCheck(std::set<PrologVariable*> vs) {
		for (auto v = vs.begin(); v != vs.end(); ++v) {
			addInputvarToCheck(*v);
		}
	}

	void addOutputvarToCheck(PrologVariable* v) {
		_outputvars_to_check.insert(v);
	}

	void addOutputvarsToCheck(std::set<PrologVariable*> vs) {
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
	bool _prefix;
	bool _fact;
	bool _numerical_operation;
public:
	PrologTerm(std::string functor, std::list<PrologTerm*> args)
			: 	FormulaClause(functor),
				_infix(false),
				_sign(true),
				_tabled(false),
				_prefix(true),
				_fact(false) ,
				_numerical_operation(false) {
		_arguments = args;
	}
	PrologTerm(std::string functor)
			: 	FormulaClause(functor),
				_infix(false),
				_sign(true),
				_tabled(false),
				_prefix(true),
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
	void numericalOperation(bool b) {
		_numerical_operation = b;
	}
	bool numericalOperation() {
		return _numerical_operation;
	}
	void prefix(bool b) {
		_prefix = b;
	}
	bool prefix() {
		return _prefix;
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
	std::string* namep() {
		return &_name;
	}
	const std::list<PrologTerm*>& arguments() const {
		return _arguments;
	}
	void accept(FormulaClauseVisitor*) {
	}

	PrologTerm* asTerm() {
		return this;
	}

	void addChild(FormulaClause* f) {
		addArgument((PrologTerm*) f);
	}

	std::set<PrologVariable*> variablesRequiringInstantiation();

	virtual std::ostream& put(std::ostream&) const;

};

class PrologVariable: public PrologTerm {
	friend std::ostream& operator<<(std::ostream& output, const PrologVariable& pv);
private:
	static std::map<std::string, PrologVariable*> vars;
	std::string _type;

public:
	PrologVariable(std::string name, std::string type = "")
			: 	PrologTerm(name),
				_type(type) {
		prefix(false);
	}
	std::string type() {
		return _type;
	}
	void type(std::string type) {
		_type = type;
	}
	static PrologVariable* create(std::string name = "", std::string type = "");
	PrologTerm* instantiation();
	virtual std::ostream& put(std::ostream&) const;
};

class PrologConstant: public PrologTerm {
	friend std::ostream& operator<<(std::ostream& output, const PrologConstant& pc);
public:
	PrologConstant(std::string name)
			: PrologTerm(name) {
		prefix(false);
	}
	virtual std::ostream& put(std::ostream&) const;
};

class PrologClause {
	friend std::ostream& operator<<(std::ostream& output, const PrologClause& pc);
private:
	PrologTerm* _head;
	std::list<PrologTerm*> _body;
	void reorder();
public:
	PrologClause(PrologTerm* head, std::list<PrologTerm*> body, bool reordering = true)
			: 	_head(head),
				_body(body) {
		if (reordering) {
			reorder();

		}
	}
	PrologClause(PrologTerm* head)
			: 	_head(head),
				_body() {

	}
	PrologClause(PrologTerm* head, PrologTerm* body)
			: 	_head(head),
				_body() {
		_body.push_back(body);
	}

	const PrologTerm* head() const {
		return _head;
	}

	const std::list<PrologTerm*>& body() const {
		return _body;
	}
};

class PrologProgram {
	friend std::ostream& operator<<(std::ostream& output, const PrologProgram& pp);
private:
	std::list<PrologClause*> _clauses;
	std::set<PFSymbol*> _tabled;
	std::set<Sort*> _sorts;
	Structure* _structure;
	Definition* _definition;
	std::set<std::string> _loaded;
	std::set<std::string> _all_predicates;
public:
	PrologProgram(Structure* structure)
			: 	_structure(structure),
				_definition(NULL) {
	}
	void table(PFSymbol*);
	void addClause(PrologClause* pc) {
		_clauses.push_back(pc);
	}
	std::string getCode();
	std::string getFacts();
	std::string getRanges();

	void addDefinition(Definition* d);
	bool isTabling(PFSymbol* p) {
		return _tabled.find(p) != _tabled.end();
	}
	void addClauses(std::list<PrologClause*> pcs) {
		for (std::list<PrologClause*>::iterator it = pcs.begin(); it != pcs.end(); ++it) {
			_clauses.push_back(*it);
		}
	}
	std::list<PrologClause*> clauses() {
		return _clauses;
	}
	Structure* structure() {
		return _structure;
	}
	const std::set<std::string>& allPredicates() const {
		return _all_predicates;
	}
	void addDomainDeclarationFor(Sort*);
};

class FormulaClauseVisitor;

class SymbolClause: public FormulaClause {
private:
	std::string _functor;
public:
	SymbolClause(std::string functor)
			: _functor(functor) {
	}
	void accept(FormulaClauseVisitor*);
	virtual PrologTerm* asTerm();
};

class CompositeFormulaClause: public FormulaClause {
protected:
	std::list<FormulaClause*> _children;
public:
	CompositeFormulaClause(std::list<FormulaClause*>& children)
			: 	FormulaClause(),
				_children(children) {
	}
	CompositeFormulaClause()
			: 	FormulaClause(),
				_children() {
	}
	std::list<FormulaClause*>& children() {
		return _children;
	}
	void children(std::list<FormulaClause*>& children) {
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
	std::set<PrologVariable*> _quantifiedVariables;
	FormulaClause* _child;
public:
	QuantifiedFormulaClause(std::set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: 	FormulaClause(),
				_quantifiedVariables(quantifiedVariables),
				_child(child) {
	}
	QuantifiedFormulaClause(std::set<PrologVariable*> quantifiedVariables)
			: 	FormulaClause(),
				_quantifiedVariables(quantifiedVariables),
				_child() {
	}
	QuantifiedFormulaClause()
			: 	FormulaClause(),
				_quantifiedVariables(),
				_child() {
	}
	std::set<PrologVariable*>& quantifiedVariables() {
		return _quantifiedVariables;
	}
	FormulaClause* child() {
		return _child;
	}
	void child(FormulaClause* child) {
		_child = child;
	}
	void quantifiedVariables(std::set<PrologVariable*> variables) {
		_quantifiedVariables = variables;
	}
	void addQuantifiedVar(PrologVariable* var) {
		_quantifiedVariables.insert(var);
	}
	void addChild(FormulaClause* f) {
		_child = f;
	}
	std::set<PrologVariable*> variablesRequiringInstantiation();
};

class ExistsClause: public QuantifiedFormulaClause {
public:
	ExistsClause(std::set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: QuantifiedFormulaClause(quantifiedVariables, child) {
	}
	ExistsClause(std::set<PrologVariable*> quantifiedVariables)
			: QuantifiedFormulaClause(quantifiedVariables) {
	}
	ExistsClause()
			: QuantifiedFormulaClause() {
	}
	void accept(FormulaClauseVisitor* v);

};

class ForallClause: public QuantifiedFormulaClause {
public:
	ForallClause(std::set<PrologVariable*> quantifiedVariables, FormulaClause* child)
			: QuantifiedFormulaClause(quantifiedVariables, child) {
	}
	ForallClause(std::set<PrologVariable*> quantifiedVariables)
			: QuantifiedFormulaClause(quantifiedVariables) {
	}
	ForallClause()
			: QuantifiedFormulaClause() {
	}
	void accept(FormulaClauseVisitor*);
};

class AndClause: public CompositeFormulaClause {
public:
	AndClause(std::list<FormulaClause*>& children)
			: CompositeFormulaClause(children) {
	}
	AndClause()
			: CompositeFormulaClause() {
	}
	void accept(FormulaClauseVisitor*);
};

class OrClause: public CompositeFormulaClause {
public:
	OrClause(std::list<FormulaClause*>& children)
			: CompositeFormulaClause(children) {
	}
	OrClause()
			: CompositeFormulaClause() {
	}
	void accept(FormulaClauseVisitor*);
};

class AggregateTerm;
class SetExpression;

class AggregateClause: public FormulaClause {
private:
	PrologVariable* _aggvar;
	std::string _type;
	AggregateTerm* _aggterm;
	PrologTerm* _term;
public:
	AggregateClause()
			: 	FormulaClause(),
				_aggvar(),
				_type(),
				_aggterm(),
				_term() {
	}
	PrologVariable* aggvar() {
		return _aggvar;
	}
	void aggvar(PrologVariable* v) {
		_aggvar = v;
	}
	void type(std::string t) {
		_type = t;
	}
	std::string type() {
		return _type;
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

class AggregateTerm: public FormulaClause {
private:
	SetExpression* _set;
	AggFunction _type;
	PrologVariable* _result;
public:
	AggregateTerm()
			: FormulaClause() {
		_result = PrologVariable::create("RESULT_VAR");
	}
	void accept(FormulaClauseVisitor*);
	void type(AggFunction type) {
		_type = type;
	}
	AggFunction type() {
		return _type;
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

class SetExpression: public FormulaClause {
private:
	PrologVariable* _var;
	std::set<PrologVariable*> _quantvars;
public:
	SetExpression()
			: FormulaClause() {
		_var = PrologVariable::create("INTERNAL_VAR_NAME");
	}
	PrologVariable* var() {
		return _var;
	}
	void close() {
		FormulaClause::addArgument(_var);
	}
	void quantifiedVariables(std::set<PrologVariable*> v) {
		_quantvars = v;
	}

	std::set<PrologVariable*>& quantifiedVariables() {
		return _quantvars;
	}
};

class EnumSetExpression: public SetExpression {
private:
	std::map<FormulaClause*, PrologTerm*> _set;
	FormulaClause* _last;
public:
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
	FormulaClause* _clause;
	PrologTerm* _term;
public:
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

class FormulaClauseVisitor: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
public:
	virtual void visit(PrologTerm*) = 0;
	virtual void visit(SymbolClause*) = 0;
	virtual void visit(ExistsClause*) = 0;
	virtual void visit(ForallClause*) = 0;
	virtual void visit(AndClause*) = 0;
	virtual void visit(OrClause*) = 0;
	virtual void visit(AggregateClause*) = 0;
	virtual void visit(AggregateTerm*) = 0;
	virtual void visit(EnumSetExpression*) = 0;
	virtual void visit(QuantSetExpression*) = 0;
};

class FormulaClauseToPrologClauseConverter: public FormulaClauseVisitor {
private:
	std::list<PrologClause*> _prologClauses;
	PrologProgram* _pp;
public:
	FormulaClauseToPrologClauseConverter(PrologProgram* p)
			: _pp(p) {
	}
	void visit(FormulaClause* f) {
		f->accept(this);
	}
	virtual void visit(PrologTerm*){
		// TODO?
	}
	virtual void visit(SymbolClause*){
		// TODO?
	}
	void visit(ExistsClause*);
	void visit(ForallClause*);
	void visit(AndClause*);
	void visit(OrClause*);
	void visit(AggregateClause*);
	void visit(AggregateTerm*);
	void visit(QuantSetExpression*);
	void visit(EnumSetExpression*);
};

class FormulaClauseBuilder: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::list<FormulaClause*> _ruleClauses;
	FormulaClause* _parent;
	PrologProgram* _pp;
	void enter(FormulaClause* f);
	void leave();
public:
	FormulaClauseBuilder(PrologProgram* p, Definition* d)
			: 	_ruleClauses(),
				_parent(NULL),
				_pp(p) {
		visit(d);
	}

	std::list<FormulaClause*>& clauses() {
		return _ruleClauses;
	}

	void visit(const Rule*);
	void visit(const Definition*);
	void visit(const Theory*);

	void visit(const VarTerm*);
	void visit(const DomainTerm*);
	void visit(const AggTerm*);
	void visit(const AggForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const PredForm*);
	void visit(const FuncTerm*);
	void visit(const EnumSetExpr*);
	void visit(const QuantSetExpr*);
};
