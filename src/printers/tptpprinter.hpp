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

#include <algorithm>
#include "printers/print.hpp"
#include "IncludeComponents.hpp"

#include "groundtheories/GroundPolicy.hpp"
#include "visitors/VisitorFriends.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"

template<typename Stream>
class TPTPPrinter: public StreamPrinter<Stream> {
	VISITORFRIENDS()
private:
	bool _ignore;
	bool _conjecture;
	bool _arithmetic;
	bool _nats;
	bool _ints;
	bool _floats;
	unsigned int _count;
	std::set<DomainTerm*> _typedDomainTerms;
	std::set<DomainElement*> _typedDomainElements;
	std::set<Sort*> _types;
	std::stringstream _os;
	std::stringstream _typeStream; // The types. (for TFF)
	std::stringstream _typeAxiomStream; // The type predicates (defining what atomic symbols have what type,
										// and which functions/predicates take which types)
	std::stringstream _axiomStream; // The first theory as axioms
	std::stringstream _conjectureStream; // The second theory as conjectures

	using StreamPrinter<Stream>::output;

public:
	TPTPPrinter(bool arithmetic, Stream& stream)
			: 	StreamPrinter<Stream>(stream),
			  	_ignore(false),
				_conjecture(false),
				_arithmetic(arithmetic),
				_nats(false),
				_ints(false),
				_floats(false),
				_count(0) {
	}

	bool conjecture() {
		return _conjecture;
	}

	void conjecture(bool conjecture) {
		_conjecture = conjecture;
	}

	void startTheory() {
	}
	void endTheory() {
	}

	// TODO future: separate code and resolve reuse with e.g. XSB interface.
	//		solve by curl_easy_escape approach, recoding all special characters to a string of ASCII letters and digits.
	std::string encodeToASCII(const std::string& in) const {
		auto result = replaceAllIn(in, "a", "ab");
		result = replaceAllIn(result, "'", "aa");
		return result;
	}

	std::string decodeFromASCII(const std::string& in) const {
		auto result = replaceAllIn(in, "aa", "'");
		result = replaceAllIn(result, "ab", "a");
		return result;
	}

	template<class NamedObject>
	std::string getSupportedName(const NamedObject& object) const {
		return encodeToASCII(object->name());
	}

protected:

	void visit(const Vocabulary* v) {
		for (auto it = v->firstSort(); it != v->lastSort(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) {
				visit(it->second);
			}
		}
		for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) {
				visit(it->second);
			}
		}
		for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
			if (not it->second->builtin() || v == Vocabulary::std()) {
				visit(it->second);
			}
		}
	}

	void visit(const Theory* theory) {
		auto newtheory = theory->clone();
		newtheory = FormulaUtils::graphFuncsAndAggs(newtheory, NULL,  {}, true, false /*TODO check*/, Context::POSITIVE);

		if (not _conjecture) {
			for (auto c: newtheory->getComponents()) {
				startAxiom("a");
				c->accept(this);
				endAxiom(_axiomStream);
				_count++;
			}
		} else if (not newtheory->getComponents().empty()) {
			// Output a conjecture as a conjunction.
			startAxiom("cnj");
			bool begin = true;
			for (auto sentence : newtheory->getComponents()) {
				if(not begin){
					_os << " & ";
				}
				begin = false;
				_os << "(";
				sentence->accept(this);
				_os << ")";
			}
			if(_ignore){
				throw IdpException("Could not transform the conjecture theory into FO, so nothing can be proven.");
		}
			endAxiom(_conjectureStream);
		}
		// Do nothing with definitions or fixpoint definitions
		if (_conjecture) {
			outputDomainTermTypeAxioms();
			writeStreams();
		}
	}

	/** Formulas **/

	void visit(const PredForm* f) {
		if (isNeg(f->sign())) {
			_os << "~";
		}
		if (VocabularyUtils::isPredicate(f->symbol(), STDPRED::EQ)) {
			_os << "(";
			f->subterms()[0]->accept(this);
			_os << " = ";
			f->subterms()[1]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isPredicate(f->symbol(), STDPRED::GT)) {
			_os << "$greater(";
			f->subterms()[0]->accept(this);
			_os << ",";
			f->subterms()[1]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isPredicate(f->symbol(), STDPRED::LT)) {
			_os << "$less(";
			f->subterms()[0]->accept(this);
			_os << ",";
			f->subterms()[1]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isFunction(f->symbol(), STDFUNC::ADDITION)) {
			_os << "($sum(";
			f->subterms()[0]->accept(this);
			_os << ",";
			f->subterms()[1]->accept(this);
			_os << ") = ";
			f->subterms()[2]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isFunction(f->symbol(), STDFUNC::SUBSTRACTION)) {
			_os << "($difference(";
			f->subterms()[0]->accept(this);
			_os << ",";
			f->subterms()[1]->accept(this);
			_os << ") = ";
			f->subterms()[2]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isFunction(f->symbol(), STDFUNC::UNARYMINUS)) {
			_os << "($uminus(";
			f->subterms()[0]->accept(this);
			_os << ") = ";
			f->subterms()[1]->accept(this);
			_os << ")";
		} else if (VocabularyUtils::isFunction(f->symbol(), STDFUNC::PRODUCT)) {
			_os << "($product(";
			f->subterms()[0]->accept(this);
			_os << ",";
			f->subterms()[1]->accept(this);
			_os << ") = ";
			f->subterms()[2]->accept(this);
			_os << ")";
		} /*else if (toString(f->symbol()) == "PRED") {
			_ignore = true; // TODO
		} else if (toString(f->symbol()) == "SUCC") {
			_ignore = true; // TODO
		} else if (toString(f->symbol()) == "MAX") {
			_ignore = true; // TODO, not the function max/2
		} else if (toString(f->symbol()) == "MIN") {
			_ignore = true; // TODO, not the function min/2
		} */ else if (VocabularyUtils::isFunction(f->symbol(), STDFUNC::ABS)) {
			// NOTE: $itett is often unsupported
			_os << "($itett($greater(";
			f->subterms()[0]->accept(this);
			_os << ",$uminus(";
			f->subterms()[0]->accept(this);
			_os << ")),";
			f->subterms()[0]->accept(this);
			_os << ",$uminus(";
			f->subterms()[0]->accept(this);
			_os << ")) = ";
			f->subterms()[1]->accept(this);
			_os << ")";
		} else {
			_os << "p_" << rewriteLongname(toString(f->symbol()));
			if (!f->subterms().empty()) {
				_os << "(";
				f->subterms()[0]->accept(this);
				for (unsigned int n = 1; n < f->subterms().size(); ++n) {
					_os << ",";
					f->subterms()[n]->accept(this);
				}
				_os << ")";
			}
		}
	}

	void visit(const EqChainForm* f) {
		if (isNeg(f->sign())) {
			_os << "~";
		}
		_os << "(";
		f->subterms()[0]->accept(this);
		for (unsigned int n = 0; n < f->comps().size(); ++n) {
			if (f->comps()[n] == CompType::EQ)
				_os << " = ";
			else if (f->comps()[n] == CompType::NEQ)
				_os << " != ";
			f->subterms()[n + 1]->accept(this);
			if (n + 1 < f->comps().size()) {
				if (f->conj())
					_os << " & ";
				else
					_os << " | ";
				f->subterms()[n + 1]->accept(this);
			}
		}
		_os << ")";
	}

	void visit(const EquivForm* f) {
		if (isNeg(f->sign())) {
			_os << "~";
		}
		_os << "(";
		f->left()->accept(this);
		_os << " <=> ";
		f->right()->accept(this);
		_os << ")";
	}

	void visit(const BoolForm* f) {
		if (f->subformulas().empty()) {
			if (f->isConjWithSign()) {
				_os << "$true";
			} else {
				_os << "$false";
			}
		} else {
			if (isNeg(f->sign())) {
				_os << "~";
			}
			_os << "(";
			f->subformulas()[0]->accept(this);
			for (unsigned int n = 1; n < f->subformulas().size(); ++n) {
				if (f->conj())
					_os << " & ";
				else
					_os << " | ";
				f->subformulas()[n]->accept(this);
			}
			_os << ")";
		}
	}

	void visit(const QuantForm* f) {
		if (isNeg(f->sign())) {
			_os << "~";
		}
		if(f->quantVars().empty()){
			f->subformula()->accept(this);
			return;
		}
		_os << "(";
		if (f->isUniv()) {
			_os << "! [";
		} else {
			_os << "? [";
			}

		bool begin = true;
		for (auto var : f->quantVars()) {
			if(not begin){
				_os << ",";
		}
			begin = false;
			_os << "V_" << getSupportedName(var);
			if (_arithmetic && var->sort()) {
				_os << ": ";
				_os << TFFTypeString(var->sort());
			}
		}
		_os << "] : (";

		// When quantifying over types, add these.
			if (f->isUniv()) {
			_os << "~";
			}
		_os << "(";
		begin = true;
		for (auto var: f->quantVars()) {
			auto sort = var->sort();
			if(not begin){
				_os <<" & ";
			}
			begin = false;
			_os << "t_" << getSupportedName(sort) << "(V_" << getSupportedName(var) << ")";

			if (not contains(_types, sort)) {
				_types.insert(var->sort());
			}
			if (SortUtils::isSubsort(sort, get(STDSORT::NATSORT))) {
				_nats = true;
			} else if (SortUtils::isSubsort(sort, get(STDSORT::INTSORT))) {
				_ints = true;
			} else if (SortUtils::isSubsort(sort, get(STDSORT::FLOATSORT))) {
				_floats = true;
			}
				}
		_os << ")";
			if (f->isUniv()) {
			_os << " | ";
			} else {
			_os << " & ";
		}

		_os << "(";
		f->subformulas()[0]->accept(this);
		_os << ")))";
	}

	void turnCardGeqBoundIntoFO(AggTerm* term, int bound){
		if(bound>5){
			_ignore = true;
			return;
			}

		if(bound<=0){
			_os << " $true ";
			return;
		}

		auto origvars = term->set()->getSets().front()->quantVars();
		auto formula = term->set()->getSets().front()->getCondition();
		std::vector<std::vector<Variable*> > varsets;
		std::vector<Formula*> conditions;
		for(int i = 0; i<bound; ++i){
			std::vector<Variable*> vars;
			std::map<Variable*, Variable*> var2var;
			for(auto var: origvars){
				auto newvar = new Variable(var->sort());
				vars.push_back(newvar);
				var2var[var]=newvar;
			}
			conditions.push_back(formula->clone(var2var));
			varsets.push_back(vars);
	}

		for(uint i=0; i<varsets.size(); ++i){
			const auto& firstset = varsets[i];
			for(uint j=0; j<i; ++j){
				const auto& secondset = varsets[j];
				std::vector<Formula*> formulas;
				for(uint index = 0; index<firstset.size(); ++index){
					auto firstvar = firstset[index];
					auto secondvar = secondset[index];
					formulas.push_back(new PredForm(SIGN::NEG, get(STDPRED::EQ, firstvar->sort()), {new VarTerm(firstvar, TermParseInfo()), new VarTerm(secondvar, TermParseInfo())}, FormulaParseInfo()));
				}
				conditions.push_back(new BoolForm(SIGN::POS, false, formulas, FormulaParseInfo()));
			}
		}

		varset allvars;
		for(auto varset: varsets){
			for(auto var: varset){
				allvars.insert(var);
			}
		}
		auto constraint = new QuantForm(SIGN::POS, QUANT::EXIST, allvars, new BoolForm(SIGN::POS, true, conditions, FormulaParseInfo()), FormulaParseInfo());
		constraint->accept(this);
		constraint->recursiveDelete();
	}

	virtual void visit(const AggForm* af) {
		if(af->getAggTerm()->set()->getSubSets().size()!=1){
			_ignore = true;
			return;
		}
		if(not TermUtils::isCard(af->getAggTerm())){
			_ignore = true;
			return;
		}
		auto bound = dynamic_cast<DomainTerm*>(af->getBound());
		if (bound == NULL || bound->value()->type() != DomainElementType::DET_INT) {
			_ignore = true;
			return;
	}
		auto value = bound->value()->value()._int;

		if(af->sign()==SIGN::NEG){
			_os <<"~ (";
		}
		switch(af->comp()){ // NOTE term comp agg!
		case CompType::EQ:
			_os << " ( ~ ( ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value+1);
			_os << " ) & ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value);
			_os << " ) ";
			break;
		case CompType::NEQ:
			_os << " ( ~ ( ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value);
			_os << " ) | ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value+1);
			_os << " ) ";
			break;
		case CompType::GEQ:
			_os <<" ~ ( ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value+1);
			_os <<" ) ";
			break;
		case CompType::LEQ:
			turnCardGeqBoundIntoFO(af->getAggTerm(), value);
			break;
		case CompType::GT:
			_os <<" ~ ( ";
			turnCardGeqBoundIntoFO(af->getAggTerm(), value);
			_os <<" ) ";
			break;
		case CompType::LT:
			turnCardGeqBoundIntoFO(af->getAggTerm(), value+1);
			break;
		}
		if(af->sign()==SIGN::NEG){
			_os <<" ) ";
		}
	}

	/** Terms **/

	void visit(const VarTerm* t) {
		_os << "V_" << getSupportedName(t->var());
	}

	void visit(const DomainTerm* t) {
		if (t->sort() && _typedDomainElements.find(const_cast<DomainElement*>(t->value())) == _typedDomainElements.cend()) {
			_typedDomainElements.insert(const_cast<DomainElement*>(t->value()));
			_typedDomainTerms.insert(const_cast<DomainTerm*>(t));
			if (SortUtils::isSubsort(t->sort(), get(STDSORT::NATSORT))) {
				_nats = true;
			} else if (SortUtils::isSubsort(t->sort(), get(STDSORT::INTSORT))) {
				_ints = true;
			} else if (SortUtils::isSubsort(t->sort(), get(STDSORT::FLOATSORT))) {
				_floats = true;
			}
		}
		if (t->sort() && not contains(_types, t->sort())) {
			_types.insert(t->sort());
		}
		_os << domainTermNameString(t);
	}

	void visit(const Sort* s) {
		if (!(s->parents().empty())) {
			startAxiom("ta");
			_os << "! [X";
			if (_arithmetic) {
				_os << ": " << TFFTypeString(s);
			}
			_os << "] : (~" << "t_" << getSupportedName(s) << "(X) | (";
			bool begin = true;
			for (auto parent : s->parents()) {
				if(not begin){
					_os <<" & ";
			}
				begin = false;
				_os << "t_" << getSupportedName(parent) << "(X)";
			}
			_os << ") )";
			endAxiom(_typeAxiomStream);
			_count++;
		}
	}

	void visit(const Predicate* p) {
		outputPFSymbolType(p);
		if (!p->overloaded() && p->arity() > 0) {
			_count++;
		}
	}

	void visit(const Function* f) {
		outputPFSymbolType(f);
		if (!f->overloaded()) {
			outputFuncAxiom(f);
			_count++;
		}
	}

private:
	void startAxiom(std::string prefix) {
		_os.str(std::string());
		startAxiom(prefix, (_conjecture ? "conjecture" : "axiom"));
	}

	void startAxiomSentence(std::string prefix) {
		startAxiom(prefix, "axiom");
	}

	void startAxiom(std::string prefix, std::string type) {
		_ignore = false;
		if (_arithmetic) {
			_os << "tff";
		} else {
			_os << "fof";
		}
		_os << "(" << prefix << _count << "," << type << ",(";
	}

	void endAxiom(std::ostream& stream) {
		if (not _ignore) {
			_os << ")).\n";
			stream << _os.str();
		}
		_os.str(std::string());
	}

	void outputPFSymbolType(const PFSymbol* pfs) {
		if (!pfs->overloaded() && pfs->nrSorts() > 0) {
			startAxiomSentence("ta");
			_os << "! [" << "V0";
			if (_arithmetic) {
				_os << ": " << TFFTypeString(pfs->sort(0));
			}
			for (unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				_os << ",V" << n;
				if (_arithmetic) {
					_os << ": " << TFFTypeString(pfs->sort(n));
				}
			}
			_os << "] : (";
			if (pfs->nrSorts() != 1 || toString(pfs) != pfs->sort(0)->name()) {
				_os << "~";
			}
			_os << "p_" << rewriteLongname(toString(pfs)) << "("; //TODO: because of the rewrite, the longname options cant be given here!
			_os << "V0";
			for (unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				_os << ",V" << n;
			}
			if (pfs->nrSorts() == 1 && toString(pfs) == pfs->sort(0)->name()) {
				_os << ") <=> (";
			} else {
				_os << ") | (";
			}
			_os << "t_" << getSupportedName(pfs->sort(0)) << "(V0)";
			for (unsigned int n = 1; n < pfs->nrSorts(); ++n) {
				_os << " & " << "t_" << getSupportedName(pfs->sort(n)) << "(V" << n << ")";
			}
			_os << "))";
			if (_arithmetic) {
				outputTFFPFSymbolType(pfs);
			}
			endAxiom(_typeAxiomStream);
		}
	}

	void outputTFFPFSymbolType(const PFSymbol* pfs) {
		startAxiom("t", "type");
		_os << "p_" << rewriteLongname(toString(pfs)) << ": ";
		if (pfs->nrSorts() > 1) {
			_os << "(";
		}
		for (unsigned int n = 0; n < pfs->nrSorts(); ++n) {
			if (pfs->sort(n)) {
				_os << TFFTypeString(pfs->sort(n));
			} else {
				_os << "$i";
			}
			if (n + 1 < pfs->nrSorts()) {
				_os << " * ";
			}
		}
		if (pfs->nrSorts() > 1) {
			_os << ")";
		}
		_os << " > $o";
		endAxiom(_typeStream);
	}

	void outputDomainTermTypeAxioms() {
		std::vector<DomainTerm*> strings;
		for (auto domterm : _typedDomainTerms) {
			startAxiomSentence("dtta");
			auto sortName = getSupportedName(domterm->sort());
			_os << "t_" << sortName << "(" << domainTermNameString(domterm) << ")";
			endAxiom(_typeAxiomStream);
			if (SortUtils::isSubsort(domterm->sort(), get(STDSORT::STRINGSORT))) {
				strings.push_back(domterm);
			}
			_count++;
		}
		if (_arithmetic) {
			for (auto type : _types) {
				_typeStream << "tff(" << "dtt" << _count << ",type,(";
				_typeStream << "t_" << getSupportedName(type) << ": ";
				_typeStream << TFFTypeString(type) << " > $o" << ")).\n";
				_count++;
			}
		}
		if (strings.size() >= 2) {
			startAxiomSentence("stringineqa");
			for (unsigned int i = 0; i < strings.size() - 1; i++) {
				for (unsigned int j = i + 1; j < strings.size(); j++) {
					_os << domainTermNameString(strings[i]) << " != " << domainTermNameString(strings[j]);
					if (!(i == strings.size() - 2 && j == strings.size() - 1)) {
						_os << " & ";
					}
				}
			}
			endAxiom(_typeAxiomStream);
		}
		_typedDomainElements.clear();
		_typedDomainTerms.clear();
		_types.clear();
	}

	// FIXME apply proper rewriting here, using getsupportedname
	std::string rewriteLongname(const std::string& longname) {
		// Fancy stuff here.
		std::string result = longname;

		// Remove types
		auto pos = result.find("[");
		if (pos != std::string::npos) {
			result.erase(pos);
		}

		// Append 1 more '_' to sequences of 3 or more '_'s
		pos = result.find("___");
		while (pos != std::string::npos) {
			while (result[pos + 3] == '_') {
				++pos;
			}
			result.replace(pos, 3, "____");
			pos = result.find("___", pos + 3);
		}

		// Replace :: with ___
		pos = result.find("::");
		while (pos != std::string::npos) {
			result.replace(pos, 2, "___");
			pos = result.find("::", pos + 3);
		}

		if(result.length()>2){
			pos = result.find("<");
			while (pos != std::string::npos) {
				result.replace(pos, 2, "__l__");
				pos = result.find("<", pos + 3);
			}
			pos = result.find(">");
			while (pos != std::string::npos) {
				result.replace(pos, 2, "__g__");
				pos = result.find(">", pos + 3);
			}
		}

		return result;
	}

	std::string TFFTypeString(const Sort* s) {
		if (SortUtils::isSubsort(const_cast<Sort*>(s), get(STDSORT::NATSORT))) {
			return "$int";
		} else if (SortUtils::isSubsort(const_cast<Sort*>(s), get(STDSORT::INTSORT))) {
			return "$int";
		} else if (SortUtils::isSubsort(const_cast<Sort*>(s), get(STDSORT::FLOATSORT))) {
			return "$float";
		} else {
			return "$i";
		}
	}

	std::string domainTermNameString(const DomainTerm* t) {
		std::string str = toString(t->value());
		if (t->sort()) {
			if (SortUtils::isSubsort(t->sort(), get(STDSORT::STRINGSORT))) {
				std::stringstream result;
				result << "str_" << t->value()->value()._string;
				return result.str();
			} else if (SortUtils::isSubsort(t->sort(), get(STDSORT::FLOATSORT))) {
				return str;
			} else {
				return "tt_" + str;
			}
		} else {
			return "utt_" + str;
	}
	}

	void outputFuncAxiom(const Function* f) {
		startAxiomSentence("fa");
		if (f->arity() > 0) {
			_os << "! [" << "V0";
			if (_arithmetic) {
				_os << ": " << TFFTypeString(f->sort(0));
			}
			for (unsigned int n = 1; n < f->arity(); ++n) {
				_os << ",V" << n;
				if (_arithmetic) {
					_os << ": " << TFFTypeString(f->sort(n));
				}
			}
			_os << "] : (" << "~(" << "t_" << getSupportedName(f->sort(0)) << "(V0)";
			for (unsigned int n = 1; n < f->arity(); ++n) {
				_os << " & " << "t_" << getSupportedName(f->sort(n)) << "(V" << n << ")";
			}
			_os << ") | ";
		}
		_os << "(? [X1";
		if (_arithmetic) {
			_os << ": " << TFFTypeString(f->outsort());
		}
		_os << "] : (" << "t_" << getSupportedName(f->outsort()) << "(X1) & " << "(! [X2";
		if (_arithmetic) {
			_os << ": " << TFFTypeString(f->outsort());
		}
		_os << "] : (";
		if (f->partial()) {
			_os << "~";
		}
		_os << "p_" << rewriteLongname(toString(f)) << "(";
		if (f->arity() > 0) {
			_os << "V0";
			for (unsigned int n = 1; n < f->arity(); ++n) {
				_os << ",V" << n;
			}
			_os << ",";
		}
		_os << "X2)";
		if (f->partial()) {
			_os << " | ";
		} else {
			_os << " <=> ";
	}
		_os << "X1 = X2";
		_os << "))))";
		if (f->arity() > 0) {
			_os << ")";
		}
		endAxiom(_typeAxiomStream);
	}

	void writeStreams() {
		output() << _typeStream.str();
		// Add t_nat > t_int > t_float hierarchy
		output() << _typeAxiomStream.str();
		if (_arithmetic) {
			if (_nats && _ints) {
				output() << "tff(nat_is_int,axiom,(! [X: $int] : (~t_nat(X) | t_int(X)))).\n";
			}
			if (_ints && _floats) {
				output() << "tff(int_is_float,axiom,(! [X: $int] : (~t_int(X) | t_float(X)))).\n";
			}
			if (_nats && _floats) {
				output() << "tff(nat_is_float,axiom,(! [X: $int] : (~t_nat(X) | t_float(X)))).\n";
		}
		}
		startAxiomSentence("char_is_string");
		_os << "! [X] : (~t_char(X) | t_string(X))";
		endAxiom(_axiomStream);
		output() << _axiomStream.str();
		output() << _conjectureStream.str();
		_typeStream.str(std::string());
		_typeAxiomStream.str(std::string());
		_axiomStream.str(std::string());
		_conjectureStream.str(std::string());
	}

protected:
	void visit(const GroundSet*) {
		_ignore = true;
	}
	void visit(const GroundClause&) {
		_ignore = true;
	}
	void visit(const AggGroundRule*) {
		_ignore = true;
	}
	void visit(const GroundAggregate*) {
		_ignore = true;
	}
	void visit(const CPReification*) {
		_ignore = true;
	}
	virtual void visit(const AggTerm*) {
		_ignore = true;
	}
	virtual void visit(const CPVarTerm*) {
		_ignore = true;
	}
	virtual void visit(const CPSetTerm*) {
		_ignore = true;
	}
	virtual void visit(const EnumSetExpr*) {
		_ignore = true;
	}
	virtual void visit(const QuantSetExpr*) {
		_ignore = true;
	}
	void visit(const FuncTerm*) {
		// Should have been transformed into predicates
		throw IdpException("Invalid code path");
	}
	virtual void visit(const GroundDefinition*) {
		_ignore = true;
	}
	virtual void visit(const Rule*) {
		_ignore = true;
	}
	virtual void visit(const Definition*) {
		_ignore = true;
	}
	virtual void visit(const FixpDef*) {
		_ignore = true;
	}
	void visit(const GroundFixpDef*) {
		_ignore = true;
	}
	void visit(const PCGroundRule*) {
		_ignore = true;
	}

	void visit(const Namespace*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	virtual void visit(const AbstractGroundTheory*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	virtual void visit(const GroundTheory<GroundPolicy>*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	void visit(const Structure*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	void visit(const PredTable*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	void visit(const Query*) {
		throw notyetimplemented("Trying to print out theory component which cannot be printed in TPTP format.");
	}
	void visit(const FOBDD*) {
		throw notyetimplemented("Converting FOBDD to tptp format.");
	}
	void visit(const UserProcedure*) {
		throw notyetimplemented("Converting FOBDD to tptp format.");
	}
	void visit(const Compound*) {
		throw notyetimplemented("Converting Compounds to tptp format.");
	}
};
