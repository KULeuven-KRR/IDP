#include <set>
#include <list>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
#include "compiler.hpp"
#include "theory/theory.hpp"
#include "vocabulary/vocabulary.hpp"
#include "commontypes.hpp"
#include "utils/ListUtils.hpp"
#include "theory/term.hpp"
#include "theory/Sets.hpp"
#include "theory/TheoryUtils.hpp"
#include "structure/StructureComponents.hpp"

#ifndef IDPXSB_PREFIX
#define IDPXSB_PREFIX "idpxsb"
#endif

#ifndef IDPXSB_CAPS_PREFIX
#define IDPXSB_CAPS_PREFIX "IDPXSB"
#endif

using namespace std;

map<string, string> domainels;
map<string, string> termnames;
unsigned int identifier;

bool isoperator(int c) {
	return c == '*' ||
			c == '(' ||
			c == ')' ||
			c == '/' ||
			c == '-' ||
			c == '+' ||
			c == '=' ||
			c == '%' ||
			c == '<' ||
			c == '>' ||
			c == '.';		// Dot for floating point numbers
}

string var_name(string str) {
	stringstream s;
	s << IDPXSB_CAPS_PREFIX;
	for (auto i = str.begin(); i != str.end(); ++i) {
		if (isalnum(*i)) {
			s << *i;
		} else {
			unsigned int tmp = *i;
			s << "x" << tmp << "x";
		}
	}
	return s.str();
}

string term_name(string str) {
	for (auto it = termnames.cbegin(); it != termnames.cend(); ++it) {
		if ((*it).first == str) {
			return (*it).second;
		}
		Assert((*it).second != transformIntoTermName(str)); // Value that this str will map to may not already be mapped to!
	}
	auto ret = transformIntoTermName(str);
	termnames[str] = ret;
	return ret;
}

string transformIntoTermName(string str) {
	bool numeric = true;
	for (auto i = str.begin(); i != str.end() && numeric; ++i) {
		if (!isdigit(*i) && !isoperator(*i)) {
			numeric = false;
		}
	}
	stringstream ss;
	if (str == "card" || str == "prod" || str == "min" || str == "max" || str == "abs" || str=="sum") {
		ss << IDPXSB_PREFIX << "_";
	} else if (!numeric && str != "findall") {
		ss << IDPXSB_PREFIX << "_";
	}
	ss << str;
	return ss.str();
}

string domainelement_prolog(const DomainElement* domelem) {
	auto str = toString(domelem);
	string ret;
	if(domelem->type() == DomainElementType::DET_INT ||
			domelem->type() == DomainElementType::DET_DOUBLE) {
		domainels[str] = str;
		ret = str;
	} else {
		// filter the string
		stringstream s;
		s << IDPXSB_PREFIX << "_";
		for (auto i = str.begin(); i != str.end(); ++i) {
			if (isalnum(*i)) {
				s << *i;
			} else {
				// Replace the non-conventional character (such as "_", "/", "." etc)
				// with their unsigned int value, padded with the character "x"
				unsigned int tmp = (unsigned int) *i;
				s << "x" << tmp << "x";
			}
		}
		ret = s.str();
		domainels[ret] = str;
	}
	return ret;
}

string domainelement_idp(string str) {
	auto string = domainels[str];
	if (string == "") {
		return str;
	} else {
		return string;
	}
}

string sort_name(string str) {
	stringstream s;
	for (auto i = str.begin(); i != str.end(); ++i) {
		if (isalnum(*i)) {
			s << *i;
		} else {
			unsigned int tmp = *i;
			s << "x" << tmp << "x";
		}
	}
	return s.str();
}

std::string strip(string str) {
	// Filter the names, strip off the prefixes
	str = filter(str);
	size_t i = str.rfind('/');
	string ret = str;
	if (i != string::npos) {
		ret = ret.substr(0, i);
	}
	return ret;
}

std::string filter(string str) {
	stringstream s;
	for (auto i = str.begin(); i != str.end(); ++i) {
		if (isalnum(*i) || isoperator(*i)) {
			s << *i;
		} else {
			unsigned int tmp = *i;
			s << "x" << tmp << "x";
		}
	}
	return s.str();
}

std::string address(void* ptr) {
	std::stringstream ss;
	ss << "addr" << ptr;
	return ss.str();
}

std::string as_string(int value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::string as_string(PrologTerm* term) {
	std::stringstream ss;
	ss << *term;
	return ss.str();
}

std::set<PrologVariable*> prologVars(const varset& vars) {
	std::set<PrologVariable*> list;
	for (auto var = vars.begin(); var != vars.end(); ++var) {
		list.insert(PrologVariable::create((*var)->name(), (*var)->sort()->name()));
	}
	return list;
}

std::list<PrologTerm*> vars2terms(std::list<PrologVariable*> vars) {
	std::list<PrologTerm*> terms;
	for (auto var = vars.begin(); var != vars.end(); ++var) {
		terms.push_back(*var);
	}
	return terms;
}

std::string compType2string(CompType c) {
	std::string str;
	switch (c) {
	case CompType::EQ:
		str = "=";
		break;
	case CompType::NEQ:
		str = "!=";
		break;
	case CompType::LT:
		str = "<";
		break;
	case CompType::GT:
		str = ">";
		break;
	case CompType::LEQ:
		str = "<=";
		break;
	case CompType::GEQ:
		str = ">=";
		break;
	default:
		break;
	}
	return str;
}

std::string getStripped(const std::string& name){
	return term_name(sort_name(strip(name)));
}

std::string getStrippedAppendedName(const std::string& name, int arity){
	return getStripped(name).append("/").append(toString(arity));
}

void PrologProgram::addDefinition(Definition* d) {
	_definition = d;

//	Defined symbols should be tabled
	for (auto it = d->defsymbols().begin(); it != d->defsymbols().end(); ++it) {
		table(*it);
	}
	FormulaClauseBuilder builder(this, d);
	FormulaClauseToPrologClauseConverter converter(this);
	for (auto clause = builder.clauses().begin(); clause != builder.clauses().end(); ++clause) {
		converter.visit(*clause);
	}
}

string PrologProgram::getCode() {
	for (auto clause : _clauses) {
		auto plterm = clause->head();
		auto name = plterm->name();
		auto predname = term_name(name).append("/").append(toString(plterm->arguments().size()));
		_all_predicates.insert(predname);
	}
	stringstream s;
	s <<":- set_prolog_flag(unknown, fail).\n";
	s << *this;
	return s.str();
}

std::ostream& operator<<(std::ostream& output, const PrologVariable& pv) {
	output << var_name(pv._name);
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologConstant& pc) {
	output << term_name(pc._name);
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologTerm& pt) {
	if (pt._numeric) {
		if (!pt._sign) {
			output << "\\+ ";
		}
		if (pt.name() == "abs") {
			output << IDPXSB_PREFIX << "_abs(" << toString(*pt._arguments.front()) << ", " << toString(*pt._arguments.back()) << ")";
			return output;
		}
		if (pt._arguments.size() == 2) {
			if (pt._arguments.back()->numeric()) {
				output << toString(*pt._arguments.front()) << " is " << toString(*pt._arguments.back());
			} else {
				if (pt._name == "<" or pt._name == ">" or pt._name == "=<" or pt._name == ">=") {
					output << toString(*pt._arguments.front()) << "@" << pt._name << toString(*pt._arguments.back());
				} else {
					output << toString(*pt._arguments.front()) << pt._name << toString(*pt._arguments.back());
				}
			}
		} else {
			auto counter = pt._arguments.begin();
			auto first = counter++;
			auto second = counter++;
			auto solution = counter;
			auto name = pt._name;
			if(pt._name == "%") {
				name = "mod";
			}
			output << toString(**solution) << " is " << toString(**first) << " " << name << " " << toString(**second);

		}
	} else if (pt._infix) {
		if (!pt._sign) {
			output << "\\+ ";
		}
		output << toString(*pt._arguments.front()) << " " << pt._name << " " << toString(*pt._arguments.back());
	} else {
		if (!pt._sign) {
			for (auto it = (pt._variables).begin(); it != (pt._variables).end(); ++it) {
				output << *(*it)->instantiation() << ", ";
			}
			if (pt._tabled) {
				output << "tnot ";
			} else {
				output << "\\+ ";
			}
		}
		if (pt._prefix) {
			output << term_name(strip(pt._name));
		} else {
			output << pt._name;
		}
		if (!pt._arguments.empty()) {
			output << "(";
			for (auto it = (pt._arguments).begin(); it != (pt._arguments).end(); ++it) {
				if (it != (pt._arguments).begin()) {
					output << ", ";
				}
				Assert((*it)!=NULL);

				output << toString(**it);
			}
			output << ")";
		}
	}
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologClause& pc) {
	output << *(pc._head);
	if (!pc._body.empty()) {
		output << " :- ";
		auto instantiatedVars = new std::set<PrologVariable*>();
		for (std::list<PrologTerm*>::const_iterator it = (pc._body).begin(); it != (pc._body).end(); ++it) {
			if (it != (pc._body).begin()) {
				output << ", ";
			}
			for (auto var = (*it)->numericVars().begin(); var != (*it)->numericVars().end(); ++var) {
				if(instantiatedVars->find(*var) == instantiatedVars->end()) {
					output << *(*var)->instantiation() << ", ";
				}
			}
			output << (**it);
			for (auto var = (*it)->variables().begin(); var != (*it)->variables().end(); ++var) {
				instantiatedVars->insert(*var);
			}
		}
		for (auto it = (pc._head)->numericVars().begin(); it != (pc._head)->numericVars().end(); ++it) {
			if(instantiatedVars->find(*it) == instantiatedVars->end()) {
				output << ", " << *(*it)->instantiation();
			}
		}
	}
	output << ".";
	return output;
}

string PrologProgram::getRanges() {
	stringstream output;
	for (auto it = _sorts.begin(); it != _sorts.end(); ++it) {
		if (_loaded.find((*it)->name()) == _loaded.end()) {
			SortTable* st = _structure->inter((*it));
			if (st->isRange() && not st->size().isInfinite()) {
				_loaded.insert((*it)->name());
				_all_predicates.insert(getStrippedAppendedName((*it)->name(), (*it)->pred()->sorts().size()));
				output << term_name(sort_name(strip((*it)->name()))) << "(X) :- var(X), between(" << domainelement_prolog(st->first()) << ","
						<< domainelement_prolog(st->last()) << ",X).\n";
				output << term_name(sort_name(strip((*it)->name()))) << "(X) :- nonvar(X), X >= " << domainelement_prolog(st->first()) << ", X =< "
						<< domainelement_prolog(st->last()) << ".\n";
			}
		}
	}
	return output.str();
}

string PrologProgram::getFacts() {
	stringstream output;
	for (auto it = _sorts.begin(); it != _sorts.end(); ++it) {
		if (_loaded.find((*it)->name()) == _loaded.end()) {
			SortTable* st = _structure->inter((*it));
			if (not st->isRange() && st->finite()) {
				_loaded.insert((*it)->name());
				_all_predicates.insert(getStrippedAppendedName((*it)->name(), (*it)->pred()->sorts().size()));
				for (auto tuple = st->begin(); !tuple.isAtEnd(); ++tuple) {
					output << getStripped((*it)->name()) << "(" << domainelement_prolog((*tuple).front()) << ").\n";
				}
			}
		}
	}

	auto openSymbols = DefinitionUtils::opens(_definition);

	for (auto it = openSymbols.begin(); it != openSymbols.end(); ++it) {
		if ((*it)->builtin() || contains(_loaded,(*it)->nameNoArity())) {
			continue;
		}
		_loaded.insert((*it)->nameNoArity());
		_all_predicates.insert(getStrippedAppendedName((*it)->name(), (*it)->sorts().size()));
		auto st = _structure->inter(*it)->ct();
		for (auto tuple = st->begin(); !tuple.isAtEnd(); ++tuple) {
			output << term_name(strip((*it)->name()));
			const auto& tmp = *tuple;
			if(tmp.size()>0){
				output << "(";
				printList(output, tmp, ",", [](std::ostream& output, const DomainElement* domelem){output <<domainelement_prolog(domelem); }, true);
				output <<")";
			}
			output << ".\n";
		}
	}
	return output.str();
}

ostream& PrologTerm::put(ostream& output) const {
	output << *this;
	return output;
}

ostream& PrologVariable::put(ostream& output) const {
	output << *this;
	return output;
}

ostream& PrologConstant::put(ostream& output) const {
	output << *this;
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologProgram& p) {
//	Generating table statements
	for (auto it = p._tabled.begin(); it != p._tabled.end(); ++it) {
		output << ":- table " << term_name(strip((*it)->name())).append("/").append(toString((*it)->nrSorts())) << ".\n";
	}
//	Generating clauses
	for (auto it = p._clauses.begin(); it != p._clauses.end(); ++it) {
		output << **it << "\n";
	}
	return output;
}

void SymbolClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}

void AndClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}
void OrClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}
void ExistsClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}

void AggregateClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}
void ForallClause::accept(FormulaClauseVisitor* fcv) {
	fcv->visit(this);
}
void AggregateTerm::accept(FormulaClauseVisitor* f) {
	f->visit(this);
}

void QuantSetExpression::accept(FormulaClauseVisitor* f) {
	f->visit(this);
}

void EnumSetExpression::accept(FormulaClauseVisitor* f) {
	f->visit(this);
}

bool term_ordering(PrologTerm* first, PrologTerm* second) {
	auto sign1 = first->sign();
	auto sign2 = second->sign();
	if (sign1 && !sign2) {
		return true;
	} else if (sign2 && !sign1) {
		return false;
	}
	// else they have the same sign, procede with intrasign order
	auto numeric1 = first->numeric();
	auto numeric2 = second->numeric();
	if (numeric1 && !numeric2) {
		return false;
	} else if (numeric2 && !numeric1) {
		return true;
	}
	auto fact1 = first->fact();
	auto fact2 = second->fact();
	if (fact1 && !fact2) {
		return true;
	} else if (fact2 && !fact1) {
		return false;
	}
	// conintue with arity
	if (numeric1) {
		// both are numeric
		if (first->arguments().front() == second->arguments().back() || (*(++(first->arguments().begin()))) == second->arguments().back()) {
			return false;
		} else {
			return true;
		}
	} else {
		auto arity1 = first->arguments().size();
		auto arity2 = second->arguments().size();
		if (arity1 <= arity2) {
			return true;
		} else {
			return false;
		}
	}
}

void PrologClause::reorder() {
	_body.sort(term_ordering);
}

void FormulaClauseToPrologClauseConverter::visit(ExistsClause* ec) {
	std::list<PrologTerm*> body;
	body.push_back(ec->child()->asTerm());

	for (auto it = (ec->instantiatedVariables()).begin(); it != (ec->instantiatedVariables()).end(); ++it) {
		body.push_back((*it)->instantiation()->asTerm());
	}
	auto head = ec->asTerm();
	head->addNumericVars(ec->quantifiedVariables());
	auto clause = new PrologClause(head, body);
	_pp->addClause(clause);

	ec->child()->accept(this);
}

void FormulaClauseToPrologClauseConverter::visit(ForallClause* fc) {
	PrologTerm* forall = new PrologTerm("idpxsb_forall");
	forall->prefix(false);
	std::list<PrologTerm*> list;
//	Create generator clause
	AndClause tmp;
	for (auto it = (fc->quantifiedVariables()).begin(); it != (fc->quantifiedVariables()).end(); ++it) {
		tmp.addChild((*it)->instantiation());
		list.push_back(*it);
	}
	tmp.arguments(list);
	this->visit(&tmp);
	forall->addArgument(tmp.asTerm());
	auto tmp2 = new PrologTerm("");
	auto term = fc->child()->asTerm();
	std::list<PrologVariable*> emptylist;
	term->variables(emptylist);
	tmp2->addArgument(fc->child()->asTerm());
	tmp2->prefix(false);
	forall->addArgument(tmp2);
	forall->addNumericVars(set<PrologVariable*>(fc->variables().begin(), fc->variables().end()));
	_pp->addClause(new PrologClause(fc->asTerm(), forall));
	fc->child()->accept(this);

}

void FormulaClauseToPrologClauseConverter::visit(AndClause* ac) {
	std::list<PrologTerm*> body;
	for (std::list<FormulaClause*>::iterator it = (ac->children()).begin(); it != (ac->children()).end(); ++it) {
		body.push_back((*it)->asTerm());
		(*it)->accept(this);
	}
	_pp->addClause(new PrologClause(ac->asTerm(), body));
}

void FormulaClauseToPrologClauseConverter::visit(OrClause* oc) {
	for (std::list<FormulaClause*>::iterator it = (oc->children()).begin(); it != (oc->children()).end(); ++it) {
		std::list<PrologTerm*> body;
		body.push_back((*it)->asTerm());
		auto head = oc->asTerm();
		auto headvars = list<PrologVariable*>(oc->variables());
		auto childvars = list<PrologVariable*>((*it)->variables());
		for (auto var = childvars.begin(); var != childvars.end(); ++var) {
			headvars.remove(*var);
		}
		head->addNumericVars(set<PrologVariable*>(headvars.begin(), headvars.end()));
		_pp->addClause(new PrologClause(oc->asTerm(), body));
		(*it)->accept(this);
	}
}

void FormulaClauseToPrologClauseConverter::visit(AggregateClause* ac) {
	list<PrologTerm*> body;
	body.push_back(ac->aggterm()->asTerm());
	auto term = new PrologTerm(ac->type());
	term->addArgument(ac->aggterm()->result());
	term->addArgument(ac->term());
	body.push_back(term);
	ac->aggterm()->accept(this);
	_pp->addClause(new PrologClause(ac->asTerm(), body, false));
}

void FormulaClauseToPrologClauseConverter::visit(AggregateTerm* at) {
	list<PrologTerm*> body;
	for(auto it = at->instantiatedVariables().cbegin(); it != at->instantiatedVariables().cend(); it++) {
		PrologTerm* boundSort = new PrologTerm((*it)->type());
		auto sortArg = PrologVariable::create((*it)->name());
		boundSort->addArgument(sortArg);
		body.push_back(boundSort);
	}

	PrologTerm* findall = new PrologTerm("findall");
	findall->addArgument(at->set()->var());
	findall->addArgument(at->set()->asTerm());
	auto listvar = PrologVariable::create("INTERNAL_LIST");
	findall->addArgument(listvar);
	body.push_back(findall);
	at->set()->accept(this);
	PrologTerm* calculation = new PrologTerm(toString(at->type()));
	calculation->addArgument(listvar);
	calculation->addArgument(at->result());
	body.push_back(calculation);
	_pp->addClause(new PrologClause(at->asTerm(), body, false));
}

void FormulaClauseToPrologClauseConverter::visit(QuantSetExpression* set) {
	list<PrologTerm*> body;
	auto f = set->clause();
	auto t = set->term();
	body.push_back(f->asTerm());
	auto unification = new PrologTerm("=");
	unification->addArgument(t);
	unification->addArgument(set->var());
	body.push_back(unification);
	set->clause()->accept(this);
	_pp->addClause(new PrologClause(set->asTerm(), body));
}

void FormulaClauseToPrologClauseConverter::visit(EnumSetExpression* set) {
	list<PrologTerm*> body;
	for (auto it = set->set().begin(); it != set->set().end(); ++it) {
		(*it).first->accept(this);
		(*it).second->accept(this);
		auto f = (*it).first;
		auto t = (*it).second;
		body.push_back(f->asTerm());
		auto unification = new PrologTerm("=");
		unification->addArgument(t);
		unification->addArgument(set->var());
		body.push_back(unification);
		_pp->addClause(new PrologClause(set->asTerm(), body));
	}
}

void FormulaClauseBuilder::leave() {
	_parent->close();
	_parent = _parent->parent();
}

void FormulaClauseBuilder::enter(FormulaClause* f) {
	f->parent(_parent);
	_parent = f;
}

void FormulaClauseBuilder::visit(const Theory* t) {
	auto defs = t->definitions();
	for (auto def = defs.begin(); def != defs.end(); ++def) {
		(*def)->accept(this);
	}
}

void FormulaClauseBuilder::visit(const Definition* d) {
	auto rules = d->rules();
	for (auto rule = rules.begin(); rule != rules.end(); ++rule) {
		(*rule)->accept(this);
	}
}

void FormulaClauseBuilder::visit(const Rule* r) {
	ExistsClause* ec = new ExistsClause();
	ec->quantifiedVariables(prologVars(r->head()->freeVars()));
	ec->name(r->head()->symbol()->nameNoArity());
	enter(ec);
	r->body()->accept(this);
	leave();
	for (auto it1 = r->head()->subterms().begin(); it1 != r->head()->subterms().end(); ++it1) {
		if ( (*it1)->type() == TermType::DOM) {
			DomainTerm* domterm = (DomainTerm*) (*it1);
			ec->addArgument(new PrologConstant(toString(domterm->value())));
		} else {
			auto varterm = (VarTerm*) (*it1);
			auto prologvar = PrologVariable::create(varterm->var()->name(), varterm->var()->sort()->name());
			ec->addArgument(prologvar);
			ec->addVariable(prologvar);
			_pp->addDomainDeclarationFor(varterm->var()->sort());
		}
	}
	_ruleClauses.push_back(ec);
}

void FormulaClauseBuilder::visit(const VarTerm* v) {
	_pp->addDomainDeclarationFor(v->var()->sort());
	auto var = PrologVariable::create(v->var()->name(), v->var()->sort()->name());
	_parent->addArgument(var);
	_parent->addVariable(var);
}

void FormulaClauseBuilder::visit(const DomainTerm* d) {
	_parent->addArgument(new PrologConstant(toString(d->value())));
}

void FormulaClauseBuilder::visit(const EnumSetExpr* e) {
	auto term = new EnumSetExpression();
	enter(term);
	for (uint i = 0; i < e->getSubSets().size(); ++i) {
		auto subf = e->getSets()[i]->getCondition();
		auto subt = e->getSets()[i]->getTerm();
		subf->accept(this);
		subt->accept(this);
	}
	term->arguments(vars2terms(term->variables()));
	leave();
	_parent->addVariables(term->variables());
}

void FormulaClauseBuilder::visit(const QuantSetExpr* q) {
	auto term = new QuantSetExpression();
	term->quantifiedVariables(prologVars(q->quantVars()));
	enter(term);
	q->getSubFormula()->accept(this);
	q->getSubTerm()->accept(this);
	term->arguments(vars2terms(term->variables()));
	leave();
	_parent->addVariables(term->variables());
}

void FormulaClauseBuilder::visit(const AggForm* a) {
	auto term = new AggregateClause();
	term->type(compType2string(a->comp()));
	enter(term);
	a->getBound()->accept(this);
	a->getAggTerm()->accept(this);
	leave();
	term->arguments(vars2terms(term->variables()));
	_parent->addVariables(term->variables());

}

void FormulaClauseBuilder::visit(const AggTerm* a) {
	auto term = new AggregateTerm();
	term->type(a->function());
	enter(term);
	a->set()->accept(this);
	auto vars = list<PrologVariable*>(term->variables());

	for (auto it = a->freeVars().begin(); it != a->freeVars().end(); ++it) {
		auto var = PrologVariable::create((*it)->name(), (*it)->sort()->name());
		term->instantiatedVariables().insert(var);
	}
	term->variables(vars);
	term->arguments(vars2terms(vars));

	leave();
	_parent->addVariables(vars);
}

void FormulaClauseBuilder::visit(const FuncTerm* f) {
	_parent->numeric(true);
	if (f->function()->arity() == 0) {
		// The "function" is a constant and needs to be made into a PrologConstant instead of a PrologTerm
		_parent->addArgument(new PrologConstant(toString(*(f->function()->interpretation(_pp->structure())->funcTable()->begin()))));
	} else {
	auto term = new PrologTerm(strip(f->function()->name()));
	term->infix(true);
	term->numeric(true);
	enter(term);
	for (auto it = f->subterms().begin(); it != f->subterms().end(); ++it) {
		(*it)->accept(this);
	}
	leave();
	_parent->addVariables(term->variables());
	auto tmp = set<PrologVariable*>(term->variables().begin(), term->variables().end());
	_parent->addNumericVars(tmp);
	}
}

void FormulaClauseBuilder::visit(const PredForm* p) {
	auto term = new PrologTerm(strip(p->symbol()->name()));
	term->sign(p->sign() == SIGN::POS);
	term->tabled(_pp->isTabling(p->symbol()));
	term->fact(true);
	enter(term);
	for (auto it = p->subterms().begin(); it != p->subterms().end(); ++it) {
		(*it)->accept(this);
	}
	if (p->symbol()->nameNoArity() == "abs" && p->args().size() == 2) {
		term->numeric(true);
	} else if (isoperator(p->symbol()->name().at(0))) {
		if (p->args().size() == 3) {
			term->numeric(true);
			term->infix(true);
			auto nvars = set<PrologVariable*>(term->variables().begin(), term->variables().end());
			try {
				nvars.erase((PrologVariable*) term->arguments().back());
			} catch (char *str) {
				cout << str << endl;
			}
			term->addNumericVars(nvars);
		} else {
			term->numeric(true);
			term->addNumericVars(set<PrologVariable*>(term->variables().begin(), term->variables().end()));
		}
	} else {
		term->numeric(false);
	}
	leave();
	_parent->addVariables(term->variables());
}

void FormulaClauseBuilder::visit(const BoolForm* p) {
	if (p->trueFormula()) {
		PrologTerm* term = new PrologConstant("true");
		term->parent(_parent);
	} else if (p->falseFormula()) {
		PrologTerm* term = new PrologConstant("false");
		term->parent(_parent);
	} else {
		CompositeFormulaClause* term;
		if (p->conj()) {
			term = new AndClause();
		} else {
			term = new OrClause();
		}
		enter(term);
		auto subs = p->subformulas();
		for (auto sub = subs.begin(); sub != subs.end(); ++sub) {
			(*sub)->accept(this);
		}
		leave();
		term->arguments(vars2terms(term->variables()));
		_parent->addVariables(term->variables());
	}

}

void FormulaClauseBuilder::visit(const QuantForm* p) {
	QuantifiedFormulaClause* term;
	if (p->isUniv()) {
		term = new ForallClause();
	} else {
		term = new ExistsClause();
	}
	auto quantvars = p->quantVars();
	for (auto quantvar = quantvars.begin(); quantvar != quantvars.end(); ++quantvar) {
		auto var = PrologVariable::create((*quantvar)->name(), (*quantvar)->sort()->name());
		term->addQuantifiedVar(var);
	}
	enter(term);
	p->subformula()->accept(this);
	leave();

	auto vars = list<PrologVariable*>(term->variables().begin(), term->variables().end());

	for (auto it = term->quantifiedVariables().begin(); it != term->quantifiedVariables().end(); ++it) {
		vars.remove(*it);
	}
	term->variables(vars);
	term->arguments(vars2terms(vars));
	_parent->addVariables(vars);
}

PrologTerm* PrologVariable::instantiation() {
	auto sc = new PrologTerm(sort_name(_type));
	sc->addArgument(this);
	return sc;
}

void PrologProgram::table(PFSymbol* pt) {
	_tabled.insert(pt);
}

void PrologProgram::addDomainDeclarationFor(Sort* s) {
	_sorts.insert(s);
}

PrologTerm* SymbolClause::asTerm() {
	return new PrologTerm(_functor, _arguments);
}

PrologTerm* FormulaClause::asTerm() {
	if (_name.length() == 0) {
		stringstream ss;
		ss <<"temp" <<getGlobal()->getNewID();
		_name = ss.str();
	}
	auto tmp = new PrologTerm(_name, _arguments);
	tmp->variables(variables());
	return tmp;
}

std::set<PrologVariable*> FormulaClause::variablesRequiringInstantiation() {
	return set<PrologVariable*>();
}

std::set<PrologVariable*> QuantifiedFormulaClause::variablesRequiringInstantiation() {
	auto leftovers = set<PrologVariable*>(_quantifiedVariables);
	for (auto var = _child->variables().begin(); var != _child->variables().end(); ++var) {
		leftovers.erase(*var);
	}
	return leftovers;
}

std::set<PrologVariable*> PrologTerm::variablesRequiringInstantiation() {
	return _numericVars;
}

PrologVariable* PrologVariable::create(std::string name, std::string type) {
	stringstream ss;
	ss << name << type;
	string str = ss.str();

	auto v = PrologVariable::vars[str];
	if (v == NULL) {
		v = new PrologVariable(name, type);

		PrologVariable::vars[str] = v;
	}
	return v;

}

map<string, PrologVariable*> PrologVariable::vars;
