/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "Entails.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include "printers/tptpprinter.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "internalargument.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

class TheorySupportedChecker: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool _arithmeticFound;
	bool _theorySupported;
	bool _definitionFound;
	bool _aggregateFound;
public:
	template<typename T>
	void runCheck(T t) {
		_arithmeticFound = false;
		_theorySupported = true;
		_definitionFound = false;
		_aggregateFound = false;
		t->accept(this);
	}

	bool arithmeticFound() {
		return _arithmeticFound;
	}
	bool theorySupported() {
		return _theorySupported;
	}
	bool definitionFound() {
		return _definitionFound;
	}
	bool aggregateFound() {
		return _aggregateFound;
	}

protected:
	void visit(const EnumSetExpr*) {
		_theorySupported = false;
	}
	void visit(const QuantSetExpr*) {
		_theorySupported = false;
	}
	void visit(const AggTerm*) {
		_aggregateFound = true;
	}
	void visit(const AggForm*) {
		_aggregateFound = true;
	}
	void visit(const FixpDef*) {
		_theorySupported = false;
	}
	void visit(const Definition* d) {
		_definitionFound = true;
		for(auto r: d->rules()){
			r->accept(this);
		}
	}

	void visit(const EqChainForm* f) {
		auto arithmeticComparators = { CompType::LEQ, CompType::GEQ, CompType::LT, CompType::GT };
		for (unsigned int n = 0; n < f->comps().size(); ++n) {
			for (auto arithcomparison : arithmeticComparators) {
				if (f->comps()[n] == arithcomparison) {
					_arithmeticFound = true;
				}
			}
		}
		traverse(f);
	}

	void visit(const FuncTerm* f) {
		auto arithFunctions = { "+", "-", "/", "*", "%", "abs", "MAX", "MIN", "SUCC", "PRED" };
		for (auto arithFunc : arithFunctions) {
			if (toString(f->function()) == arithFunc) {
				_arithmeticFound = true;
			}
		}
		if (toString(f->function()) == "%") {
			_theorySupported = false;
		}
		traverse(f);
	}
};

State Entails::doCheckEntailment(Theory* axioms, Theory* conjectures) {
	auto state = State::UNKNOWN;
	try{
		Entails c(axioms, conjectures);
		state = c.checkEntailment();
	}catch(const IdpException& ex){
		Warning::warning(ex.getMessage());
	}
	return state;
}

Entails::Entails(Theory* axioms, Theory* conjectures)
		: 	axioms(axioms),
			conjectures(conjectures),
			hasArithmetic(true) {

	provenStrings.push_back("SZS status Theorem");
	provenStrings.push_back("SPASS beiseite: Proof found.");
	disprovenStrings.push_back("SZS status CounterSatisfiable");
	disprovenStrings.push_back("SPASS beiseite: Completion found.");

	axioms = dynamic_cast<Theory*>(FormulaUtils::splitComparisonChains(axioms,axioms->vocabulary()));
	conjectures = dynamic_cast<Theory*>(FormulaUtils::splitComparisonChains(conjectures,conjectures->vocabulary()));
	FormulaUtils::replaceCardinalitiesWithFOFormulas(conjectures, 6);
	FormulaUtils::replaceCardinalitiesWithFOFormulas(axioms, 6);

	FormulaUtils::replaceDefinitionsWithCompletion(axioms, NULL);

	// Determine whether the theories are compatible with this inference
	// and whether arithmetic support is required
	TheorySupportedChecker axiomsSupported;
	axiomsSupported.runCheck(axioms);
	if (axiomsSupported.definitionFound()) {
		Warning::warning("The input contains a definition. A (possibly) weaker form of entailment will be verified, based on its completion.");
		FormulaUtils::replaceDefinitionsWithCompletion(axioms, NULL);
	}

	TheorySupportedChecker conjecturesSupported;
	conjecturesSupported.runCheck(conjectures);
	if (not axiomsSupported.theorySupported() || not conjecturesSupported.theorySupported() || conjecturesSupported.definitionFound() || conjecturesSupported.aggregateFound()) {
		throw IdpException("Entailment checking is not supported for the provided theories. "
				"Only first-order theories (with arithmetic) are supported, with the addition of "
				"definitions and aggregates in the axiom theory. (No fixpoint definitions,...).");
	}

	// Turn functions into predicates (for partial function support)
	FormulaUtils::unnestTerms(axioms);
	axioms = FormulaUtils::graphFuncsAndAggs(axioms, NULL, {}, true, false);
	std::map<Function*, Formula*> axiom_funcconstraints;
	FormulaUtils::addFuncConstraints(axioms, axioms->vocabulary(), axiom_funcconstraints);
	for(auto func2formula: axiom_funcconstraints){
		axioms->add(func2formula.second);
	}

	FormulaUtils::unnestTerms(conjectures);
	conjectures = FormulaUtils::graphFuncsAndAggs(conjectures, NULL, {}, true, false);
	std::map<Function*, Formula*> conj_funcconstraints;
	FormulaUtils::addFuncConstraints(conjectures, conjectures->vocabulary(), conj_funcconstraints);
	for(auto func2formula: conj_funcconstraints){
		conjectures->add(func2formula.second);
	}

	if(axiomsSupported.aggregateFound()){
		Warning::warning("The input contains aggregates. Non-cardinality aggregates will be dropped, resulting in a weaker form of entailment.");
	}
	if (not conjecturesSupported.arithmeticFound() && not axiomsSupported.arithmeticFound()) {
		hasArithmetic = false;
	}
	if(not getOption(PROVER_SUPPORTS_TFA)){
		hasArithmetic = false;
	}
}

std::ostream& operator<<(std::ostream& output, const State& object) {
	switch(object){
	case State::DISPROVEN:
		output <<"Disproven";
		break;
	case State::PROVEN:
		output <<"Proven";
		break;
	case State::UNKNOWN:
		output <<"Unknown";
		break;
	}
	return output;
}

State Entails::checkEntailment() {
	char tptpinput_filename[L_tmpnam];
	char tptpoutput_filename[L_tmpnam];
	auto file = tmpnam(tptpinput_filename);
	auto file2 = tmpnam(tptpoutput_filename);
	Assert(file==tptpinput_filename);
	Assert(file2==tptpoutput_filename);
	std::ofstream tptpFile(tptpinput_filename);
	auto printer = new TPTPPrinter<std::ofstream>(hasArithmetic, tptpFile);

	// Print the theories to a TPTP file
	if (getOption(VERBOSE_ENTAILMENT) > 1) {
		clog << "Adding axioms " << print(axioms) << "\n";
	}
	printer->print(axioms->vocabulary());
	printer->print(axioms);
	printer->conjecture(true);
	if (axioms->vocabulary() != conjectures->vocabulary()) {
		printer->print(conjectures->vocabulary());
	}
	if (getOption(VERBOSE_ENTAILMENT) > 1) {
		clog << "Adding conjectures " << print(conjectures) << "\n";
	}
	printer->print(conjectures);
	delete (printer);
	tptpFile.close();

	auto tempcommand = getOption(PROVERCOMMAND);
	if(tempcommand==""){
		tempcommand = getInstallDirectoryPath()+"/bin/SPASS -TimeLimit=%t -TPTP %i > %o";
	}
	auto pos = tempcommand.find("%i");
	if (pos == std::string::npos) {
		throw IdpException("The argument string must contain the string \"%i\", indicating where to insert the input file.");
	}
	tempcommand.replace(pos, 2, tptpinput_filename);

	pos = tempcommand.find("%o");
	if (pos == std::string::npos) {
		// If %o was not found, assume output redirection
		tempcommand += " > %o";
		pos = tempcommand.find("%o");
	}
	tempcommand.replace(pos, 2, tptpoutput_filename);

	pos = tempcommand.find("%t");
	if (pos == std::string::npos) {
		// Cannot set prover timeout
		Warning::warning("Prover command does not support time limit.");
	}else{
		tempcommand.replace(pos, 2, toString(getOption(TIMEOUT_ENTAILMENT)).c_str());
	}

	// Call the prover with timeout.
	if (getOption(VERBOSE_ENTAILMENT) > 0) {
		clog << "Calling " << tempcommand << "\n";
	}
	auto callresult = system(tempcommand.c_str());
	// TODO call the prover with the prover timeout
	if (callresult != 0) {
		stringstream ss;
		ss <<"The automated theorem prover ran out of time, gave up or stopped in an irregular state.\n";
		ss <<"\tThe call issued was \"" <<tempcommand <<"\"";
		throw IdpException(ss.str());
	}

	// Retrieve the status from the result
	std::ifstream tptpResult;
	tptpResult.open(tptpoutput_filename);
	if (!tptpResult.is_open()) {
		stringstream ss;
		ss << "Could not open file " << tptpoutput_filename << " for reading.\n";
		throw IdpException(ss.str());
	}

	auto state = State::UNKNOWN;

	std::string line;
	getline(tptpResult, line);
	pos = std::string::npos;
	while (pos == std::string::npos && !tptpResult.eof()) {
		for (auto provenString : provenStrings) {
			pos = line.find(provenString);
			if (pos != std::string::npos) {
				state = State::PROVEN;
				break;
			}
		}
		if (state != State::UNKNOWN) {
			break;
		}
		for (auto disProvenString : disprovenStrings) {
			pos = line.find(disProvenString);
			if (pos != std::string::npos) {
				state = State::DISPROVEN;
				break;
			}
		}
		if (state != State::UNKNOWN) {
			break;
		}

		getline(tptpResult, line);
	}
	tptpResult.close();

	if (pos == std::string::npos) {
		state = State::UNKNOWN;
		if (getOption(VERBOSE_ENTAILMENT) > 0) {
			Warning::warning("The automated theorem prover gave up or stopped in an irregular state.");
		}
	}

	if (getOption(VERBOSE_ENTAILMENT) > 0) {
		clog <<"The prover answered " <<state <<"\n";
	}

	return state;
}
