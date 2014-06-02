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

#include "PrologProgram.hpp"
#include "FormulaClause.hpp"
#include "FormulaClauseBuilder.hpp"
#include "XSBToIDPTranslator.hpp"
#include "FormulaClauseToPrologClauseConverter.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/theory.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"
#include "structure/Structure.hpp"
#include "structure/MainStructureComponents.hpp"

using namespace std;



void PrologProgram::setDefinition(Definition* d) {
	_definition = d;

//	Defined symbols should be tabled
	for (auto symbol : d->defsymbols()) {
		table(symbol);
	}

//  Opens that are threevalued should be tabled
	for (auto symbol : DefinitionUtils::opens(d)) {
		if (not _structure->inter(symbol)->approxTwoValued()) {
			table(symbol);
		}
	}
	FormulaClauseBuilder builder(this, d, _translator);
	FormulaClauseToPrologClauseConverter converter(this, _translator);
	for (auto clause = builder.clauses().begin(); clause != builder.clauses().end(); ++clause) {
		converter.visit(*clause);
	}
}

string PrologProgram::getCode() {
	for (auto clause : _clauses) {
		auto plterm = clause->head();
		auto name = plterm->name();
		auto predname = name.append("/").append(toString(plterm->arguments().size()));
		_all_predicates.insert(predname);
	}
	stringstream s;
	s <<":- set_prolog_flag(unknown, fail).\n";
	s << *this;
	return s.str();
}


string PrologProgram::getRanges() {
	stringstream output;
	for (auto it = _sorts.begin(); it != _sorts.end(); ++it) {
		if (_loaded.find((*it)->name()) == _loaded.end()) {
			SortTable* st = _structure->inter((*it));
			if (st->isRange() && not st->size().isInfinite()) {
				_loaded.insert((*it)->name());
				_all_predicates.insert(_translator->to_prolog_pred_and_arity(*it));
				auto sortname = _translator->to_prolog_sortname((*it));
				output << sortname << "(X) :- var(X), between(" << _translator->to_prolog_term(st->first()) << ","
						<< _translator->to_prolog_term(st->last()) << ",X).\n";
				output << sortname << "(X) :- nonvar(X), X >= " << _translator->to_prolog_term(st->first()) << ", X =< "
						<< _translator->to_prolog_term(st->last()) << ".\n";
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
				_all_predicates.insert(_translator->to_prolog_pred_and_arity(*it));
				auto factname = _translator->to_prolog_sortname((*it));
				for (auto tuple = st->begin(); !tuple.isAtEnd(); ++tuple) {
					output << factname << "(" << _translator->to_prolog_term((*tuple).front()) << ").\n";
				}
			}
		}
	}

	auto openSymbols = DefinitionUtils::opens(_definition);

	for (auto symbol : openSymbols) {
		if (_translator->isXSBBuiltIn(symbol->nameNoArity()) ||
				_translator->isXSBCompilerSupported(symbol)) {
			continue;
		}

		auto isSort = false;
		for (auto sort : _sorts) {
			if (sort->pred() == symbol)
				isSort = true;
				continue;
		}

		if (isSort) {
			continue;
		}

		_all_predicates.insert(_translator->to_prolog_pred_and_arity(symbol));
		printAsFacts(_translator->to_prolog_term(symbol), symbol, output);
	}
	return output.str();
}

void PrologProgram::printAsFacts(string symbol_name, PFSymbol* symbol, std::ostream& ss) {
	auto certainly_true = _structure->inter(symbol)->ct();
	auto possibly_true = _structure->inter(symbol)->pt();
	for (auto it = possibly_true->begin(); !it.isAtEnd(); ++it) {
		ss << symbol_name;
		ElementTuple tuple = *it;
		printTuple(tuple,ss);
		if (not certainly_true->contains(tuple)) {
			// If the tuple is not in the certainly true table, it is unknown
			// Unknown symbols needs to be printed as follows:
			//   symbol(arg1,..,argn) :- undef.
			ss << " :- undef";
		}
		ss << ".\n";
	}

}

void PrologProgram::printTuple(const ElementTuple& tuple, std::ostream& ss) {
	if (tuple.size()>0){
		ss << "(";
		printList(ss, tuple, ",", [&](std::ostream& output, const DomainElement* domelem){output << _translator->to_prolog_term(domelem); }, true);
		ss <<")";
	}
}

void PrologProgram::table(PFSymbol* pt) {
	_tabled.insert(pt);
}

void PrologProgram::addDomainDeclarationFor(Sort* s) {
	_sorts.insert(s);
}


std::ostream& operator<<(std::ostream& output, const PrologProgram& p) {
//	Generating table statements
	for (auto it = p._tabled.begin(); it != p._tabled.end(); ++it) {
		output << ":- table " << p._translator->to_prolog_pred_and_arity(*it) << ".\n";
	}
//	Generating clauses
	for (auto it = p._clauses.begin(); it != p._clauses.end(); ++it) {
		output << **it << "\n";
	}
	return output;
}

void PrologClause::reorder() {
	_body.sort(PrologTerm::term_ordering);
}

std::ostream& operator<<(std::ostream& output, const PrologClause& pc) {
	output << *(pc._head);
	if (!pc._body.empty()) {
		output << " :- ";
		auto instantiatedAndCheckedVars = new std::set<PrologVariable*>();
		for (std::list<PrologTerm*>::const_iterator it = (pc._body).begin(); it != (pc._body).end(); ++it) {
			if (it != (pc._body).begin()) {
				output << ", ";
			}
			// Add type generators for the necessary variables
			for (auto var = (*it)->inputvarsToCheck().begin(); var != (*it)->inputvarsToCheck().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << *(*var)->instantiation() << ", ";
					instantiatedAndCheckedVars->insert(*var);
				}
			}
			output << (**it);
			// Add type check to the necessary variables
			for (auto var = (*it)->outputvarsToCheck().begin(); var != (*it)->outputvarsToCheck().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << ", " << *(*var)->instantiation();
					instantiatedAndCheckedVars->insert(*var);
				}
			}
			// All variables of the printed call have to be checked - they could be over a stricter type than the call
			for (auto var = (*it)->variables().begin(); var != (*it)->variables().end(); ++var) {
				if(instantiatedAndCheckedVars->find(*var) == instantiatedAndCheckedVars->end()) {
					output << ", " << *(*var)->instantiation();
					instantiatedAndCheckedVars->insert(*var);
				}
			}
		}
		for (auto it = (pc._head)->variables().begin(); it != (pc._head)->variables().end(); ++it) {
			if(instantiatedAndCheckedVars->find(*it) == instantiatedAndCheckedVars->end()) {
				output << ", " << *(*it)->instantiation();
			}
		}
	}
	output << ".";
	return output;
}
