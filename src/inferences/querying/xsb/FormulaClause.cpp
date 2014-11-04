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

#include "FormulaClause.hpp"
#include "FormulaClauseVisitor.hpp"
#include "XSBToIDPTranslator.hpp"
#include "vocabulary/vocabulary.hpp"
#include "vocabulary/VarCompare.hpp"

using std::string;
using std::stringstream;

void FormulaClause::recursiveDelete() {
	for (auto arg : _arguments) {
		arg->recursiveDelete();
	}
	for (auto arg : _variables) {
		arg->recursiveDelete();
	}
	for (auto arg : _instantiatedVariables) {
		arg->recursiveDelete();
	}
	for (auto arg : _inputvars_to_check) {
		arg->recursiveDelete();
	}
	for (auto arg : _inputvars_to_check) {
		arg->recursiveDelete();
	}
	delete (this);
}

std::ostream& operator<<(std::ostream& output, const PrologVariable& pv) {
	output << pv._name;
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologConstant& pc) {
	output << pc._name;
	return output;
}

std::ostream& operator<<(std::ostream& output, const PrologTerm& pt) {
	if (pt._numeric) {
		if (!pt._sign) {
			output << "\\+ ";
		}
		if (pt._arguments.size() == 2) {
			// Important: always add a whitespace before and after each operator, arguments may be of the form -1
			// which will only be accepted by XSB if they are written as (X = -1), and not if written as (X=-1)
			if (pt._arguments.back()->numeric()) {
				output << toString(*pt._arguments.front()) << " is " << toString(*pt._arguments.back());
			} else {
				if (pt._name == "<" or pt._name == ">" or pt._name == "=<" or pt._name == ">=") {
					if (pt._numerical_operation) {
						output << toString(*pt._arguments.front()) << " " << pt._name <<  " " << toString(*pt._arguments.back());
					} else {
						output << toString(*pt._arguments.front()) << " @" << pt._name <<  " " << toString(*pt._arguments.back());
					}
				} else {
					output << toString(*pt._arguments.front()) <<  " " << pt._name <<  " " << toString(*pt._arguments.back());
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
			if(pt._name == "/") {
				output << XSBToIDPTranslator::get_division_term_name() << "(" << toString(**solution) << ", " <<
						toString(**first) << ", " << toString(**second) << ")";
			} else if(pt._name == "^") {
					output << XSBToIDPTranslator::get_exponential_term_name() << "(" << toString(**solution) << ", " <<
							toString(**first) << ", " << toString(**second) << ")";
				} else {
				output << toString(**solution) << " is " << toString(**first) << " " << name << " " << toString(**second);
			}

		}
	} else if (pt._infix) {
		if (!pt._sign) {
			output << "\\+ ";
		}
		output << toString(*pt._arguments.front()) << " " << pt._name << " " << toString(*pt._arguments.back());
	} else {
		if (!pt._sign) {
			// For a negated call, all variables have to be instantiated
			for (auto it = (pt._variables).begin(); it != (pt._variables).end(); ++it) {
				output << *(*it)->instantiation() << ", ";
			}
			if (pt._tabled) {
				output << "tnot ";
			} else {
				output << "\\+ ";
			}
		}
		output << pt._name;
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

std::ostream& PrologTerm::put(std::ostream& output) const {
	output << *this;
	return output;
}

std::ostream& PrologVariable::put(std::ostream& output) const {
	output << *this;
	return output;
}

std::ostream& PrologConstant::put(std::ostream& output) const {
	output << *this;
	return output;
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

SetExpression::SetExpression(string name, XSBToIDPTranslator* translator, bool twovaluedbody)
		: FormulaClause(name),
		  _twovaluedbody(twovaluedbody) {
	_var = translator->create("INTERNAL_VAR_NAME");
}

AggregateTerm::AggregateTerm(string name, XSBToIDPTranslator* translator)
		: FormulaClause(name) {
	_result = translator->create("RESULT_VAR");
}


PrologTerm* PrologVariable::instantiation() {
	auto sc = new PrologTerm(_type);
	sc->addArgument(this);
	return sc;
}

PrologTerm* FormulaClause::asTerm() {
	if (_name.length() == 0) {
		std::stringstream ss;
		ss <<"temp" <<getGlobal()->getNewID();
		_name = ss.str();
	}
	auto tmp = new PrologTerm(_name, _arguments);
	tmp->variables(variables());
	return tmp;
}

std::set<PrologVariable*> FormulaClause::variablesRequiringInstantiation() {
	return std::set<PrologVariable*>();
}

std::set<PrologVariable*> QuantifiedFormulaClause::variablesRequiringInstantiation() {
	auto leftovers = set<PrologVariable*>(_quantifiedVariables);
	for (auto var = _child->variables().begin(); var != _child->variables().end(); ++var) {
		leftovers.erase(*var);
	}
	return leftovers;
}

std::set<PrologVariable*> PrologTerm::variablesRequiringInstantiation() {
	return _inputvars_to_check;
}

//TODO: replace with operator< ?
bool PrologTerm::term_ordering(PrologTerm* first, PrologTerm* second) {
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

std::list<PrologTerm*> PrologTerm::vars2terms(std::list<PrologVariable*> vars) {
	std::list<PrologTerm*> terms;
	for (auto var = vars.begin(); var != vars.end(); ++var) {
		terms.push_back(*var);
	}
	return terms;
}
