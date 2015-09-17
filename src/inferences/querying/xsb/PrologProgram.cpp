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

PrologProgram::~PrologProgram() {
	_definition->recursiveDelete();
}

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
	for (auto clause : builder.clauses()) {
		clause->recursiveDelete();
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

	// Always consider built-in sorts
	for (auto name2sort : Vocabulary::std()->getSorts()) {
		_sorts.insert(name2sort.second);
	}

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

		if (not hasElem(_sorts, [&](const Sort* sort){return sort->pred() == symbol;}) ) {
			_all_predicates.insert(_translator->to_prolog_pred_and_arity(symbol));
			printAsFacts(_translator->to_prolog_term(symbol), symbol, output);
		}
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
		delete(instantiatedAndCheckedVars);
	}
	output << ".";
	return output;
}


string PrologProgram::getCompilerCode() {
	return "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n%   Note: When adding new predicates to this file as a result of them being\n%   'built-in' in IDP, make sure to also adapt the \n%   XSBToIDPTranslator::isXSBCompilerSupported procedure\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n% ixexponential(Solution,Base,Power)\n% Base case: right hand side is known\nixexponential(Solution,Base,Power) :-\n	nonvar(Base),\n	nonvar(Power),\n	TMP is Base ** Power,\n	ixsame_number(TMP,Solution).\n	\n% Solution equals the base and is 1 -> power could be anything.\nixexponential(1,1,Power) :-\n	ixint(Power).\n\n% Solution equals the base and is not 1 -> power has to be one.\nixexponential(Base,Base,1) :-\n	nonvar(Base),\n	ixdifferent_number(Base,1). % Rule out double answer generation for previous case\n\n% ixdivision(Solution,Numerator,Denominator)\n% Represents the built-in \"Solution is Numerator/Denominator\"\n% Handle special cases of this expression first: X is Y/X, Y known\nixdivision(Denominator,Numerator,Denominator) :-\n	nonvar(Numerator),\n	var(Denominator),\n	Denominator is sqrt(Numerator).\nixdivision(Denominator,Numerator,Denominator) :-\n	var(Numerator),\n	nonvar(Denominator),\n	ixdifferent_number(Denominator,0),\n	Numerator is Denominator*Denominator.\n\n% Handle special cases of this expression first: X is X/Y, Y known \n% -> infinite generator\nixdivision(Numerator,Numerator,1) :-\n	var(Numerator),\n    throw_infinite_type_generation_error.\n\n% Handle special cases of this expression first: X is X/Y, Y known, with Y ~= 1 \n% -> this always fails\nixdivision(Numerator,Numerator,Denominator) :-\n	nonvar(Denominator),\n	ixdifferent_number(Denominator,1),\n	fail.\n	\n% Handle special cases of this expression first: O is O/Y with Y known\n% -> succeeds \nixdivision(Numerator,Numerator,Denominator) :-\n	nonvar(Numerator),\n	ixsame_number(Numerator,0),\n	nonvar(Denominator),\n	ixdifferent_number(Denominator,0).\n\n% Only normal cases left: X is Y/Z with Y and Z variables\nixdivision(Solution,Numerator,Denominator) :-\n	nonvar(Numerator),\n	nonvar(Denominator),\n	TMP is Numerator / Denominator,\n	ixsame_number(TMP,Solution).\n\n\nixabs(X,Y) :- \n	number(X),\n	Y is abs(X).\n\nixabs(X,_) :-\n	var(X),\n    throw_infinite_type_generation_error.\n\nixsum(List,Sum) :- ixsum(List,Sum,0).\nixsum([],X,X).\nixsum([H|T],Sum,Agg) :- Agg2 is Agg + H, ixsum(T,Sum,Agg2).\n\nixprod(List,Prod) :- ixprod(List,Prod,1).\nixprod([],X,X).\nixprod([H|T],Prod,Agg) :- Agg2 is Agg * H, ixprod(T,Prod,Agg2).\n\nixcard(List,Card) :- length(List,Card).\n\nixmin([X|Rest],Min) :- ixmin(Rest,Min,X).\n\nixmin([],Min,Min).\nixmin([X|Rest],Min,TmpMin) :-\n	X < TmpMin,\n	ixmin(Rest,Min,X).\nixmin([X|Rest],Min,TmpMin) :-\n	X >= TmpMin,\n	ixmin(Rest,Min,TmpMin).\n\nixmax([X|Rest],Max) :- ixmax(Rest,Max,X).\nixmax([],Max,Max).\nixmax([X|Rest],Max,TmpMax) :-\n	X > TmpMax,\n	ixmax(Rest,Max,X).\nixmax([X|Rest],Max,TmpMax) :-\n	X =< TmpMax,\n	ixmax(Rest,Max,TmpMax).\n\n% ixforall(Generator,Verifier)\n% Only succeeds if for every succeeding call to Generator, the Verifier also succeeds\n%\n% Implementation-wise, tables:not_exists/1 is used for handling negation because it \n% supports the mixed usage of tabled and non-tabled predicates (as well as the\n% conjunction/disjunction of these).\n%\n% Additionaly, this has to be surrounded by builtin call_tv([...], true), because\n% otherwise, the answer may be tagged as \"undefined\", even it if does not show this\n% when printing the answer. An example of this is the following program, in which\n% ?- p. \n% is answered as \"undefined\", even though calling the body of the second rule for p\n% is answered as \"false\".\n%\n%   :- set_prolog_flag(unknown, fail).\n%   :- table p/0, d/1, c/0, or/1.\n% \n%   p :- p.\n%   p :- tables:not_exists((type(X), tables:not_exists(or(X)))).\n% \n%   or(2).\n%   or(_) :- c.\n% \n%   c :- d(X), \\+ 1 = X.\n% \n%   d(1) :- p.\n% \n%   type(1).\n%   type(2).\n%\nixforall(CallA, CallB) :-\n    tables:not_exists((call(CallA), tables:not_exists(CallB))).\n\n% ixthreeval_findall(Var,Query,Ret)\n%   1: Gather all \"true\" answers\n%   2: Gather all \"undefined\" answers of the Query\n%   3: Append each possible subset of \"undefined\" answers list to the \"true\" \n%      answers list\n%   4: Introduce loop to make return tuple undefined if some \"undefined\" answers\n%      were added to the Ret list (generate_CT_or_U_answers/1 is used for this)\nixthreeval_findall(Var,Query,Ret) :-\n  findall(Var,call_tv(Query,true),CTList),\n  findall(Var,call_tv(Query,undefined),Ulist),\n  ixsubset(Ulist,S),\n  append(S,CTList,Ret),\n  generate_CT_or_U_answer(Ulist,S).\n\ngenerate_CT_or_U_answer([],_).\ngenerate_CT_or_U_answer([_|_],_) :- undef.\n\n:- table undef/0.\nundef :- tnot(undef).\n\nixsubset([],[]).\nixsubset([E|Tail],[E|NTail]) :-\n  ixsubset(Tail,NTail).\nixsubset([_|Tail],NTail) :-\n  ixsubset(Tail,NTail).\n\nixint(X) :- \n    nonvar(X),\n    ROUNDEDNUMBER is round(X),\n    ZERO is ROUNDEDNUMBER - X,\n    \\+ 0 < ZERO,\n    \\+ 0 > ZERO.\n    \nixint(X) :- \n    var(X),\n    throw_infinite_type_generation_error.\n     \nixfloat(X) :- \n    nonvar(X),\n    number(X).\n    \nixfloat(X) :- \n    var(X),\n    throw_infinite_type_generation_error.\n\nixnat(X) :- \n    nonvar(X),\n    X >= 0,\n    ixint(X).\n\nixnat(X) :- \n    var(X),\n    throw_infinite_type_generation_error.\n\n% TODO: leaves through too much!\nixchar(X) :- \n    nonvar(X),\n    atomic(X). % Possible todo - maintain strings during translation and check for is_charlist(X,1) (of size 1) here\n    \nixchar(X) :- \n    var(X),\n    throw_infinite_type_generation_error.\n    \nixstring(X) :- \n    nonvar(X),\n    atomic(X). % Possible todo - maintain strings during translation and check for is_charlist/1 here\n\nixstring(X) :-\n    var(X),\n    throw_infinite_type_generation_error.\n\n% First argument has to be instantiated\n% Second argument can be output variable or instantiated\nixsame_number(X,Y) :-\n    ixconvert_to_int(X,X1),\n    X1 = Y.\n     \n% First argument has to be instantiated\n% Second argument has to be instantiated\nixdifferent_number(X,Y) :-\n    ixconvert_to_int(X,X1),\n    ixconvert_to_int(Y,Y1),\n    X1 \\== Y1.\n\nixconvert_to_int(Num,Int) :-\n	nonvar(Num),\n    ixint(Num), \n    Int is round(Num).\n\nixconvert_to_int(Num,Int) :-\n	nonvar(Num),\n    ixfloat(Num), \n    Int = Num.\n    \nthrow_infinite_type_generation_error :-\n    error_handler:misc_error('Trying to generate an infinite type with XSB\\ntry to rerun with stdoptions.xsb=false to see if that works.').\n";
}
