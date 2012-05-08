/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "Entails.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include "printers/print.hpp"
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
public:
	template<typename T>
	void runCheck(T t) {
		_arithmeticFound = false;
		_theorySupported = true;
		_definitionFound = false;
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
	void definitionFound(bool definitionFound) {
		_definitionFound = definitionFound;
	}
	void theorySupported(bool theorySupported) {
		_theorySupported = theorySupported;
	}
	void arithmeticFound(bool arithmeticFound) {
		_arithmeticFound = arithmeticFound;
	}

protected:
	void visit(const FuncTerm* f);
	void visit(const EqChainForm* f);
	void visit(const EnumSetExpr*) {
		_theorySupported = false;
	}
	void visit(const QuantSetExpr*) {
		_theorySupported = false;
	}
	void visit(const AggTerm*) {
		_theorySupported = false;
	}
	void visit(const FixpDef*) {
		_theorySupported = false;
	}
	void visit(const Definition*) {
		_definitionFound = true;
	}
};

void TheorySupportedChecker::visit(const EqChainForm* f) {
	CompType arithmeticComparator[4] = { CompType::LEQ, CompType::GEQ, CompType::LT, CompType::GT };
	for (unsigned int n = 0; n < f->comps().size(); ++n) {
		for (unsigned int i = 0; i < 4; ++i) {
			if (f->comps()[n] == arithmeticComparator[i]) {
				_arithmeticFound = true;
			}
		}
	}
	if (!_arithmeticFound) {
		traverse(f);
	}
}

void TheorySupportedChecker::visit(const FuncTerm* f) {
	std::string arithmeticFunction[10] = { "+", "-", "/", "*", "%", "abs", "MAX", "MIN", "SUCC", "PRED" };
	for (unsigned int n = 0; n < 5; ++n) {
		if (toString(f->function()) == arithmeticFunction[n])
			_arithmeticFound = true;
	}
	if (toString(f->function()) == "%")
		_theorySupported = false;
	if (!_arithmeticFound) {
		traverse(f);
	}
}

static jmp_buf _timeoutJump;
void timeout(int) {
	longjmp(_timeoutJump, 1);
}

State Entails::checkEntailment(EntailmentData* data) const {
#if defined(__linux__)
#define COMMANDS_INDEX 0
#elif defined(_WIN32)
#define COMMANDS_INDEX 1
#elif defined(__APPLE__)
#define COMMANDS_INDEX 2
#else
#define COMMANDS_INDEX 3 // TODO purely for compilation, should remove this
	Error::error("\"entails\" is not supported on this platform.\n");
	return State::UNKNOWN;
#endif
	InternalArgument& fofCommand = data->fofCommands[COMMANDS_INDEX];
	InternalArgument& tffCommand = data->tffCommands[COMMANDS_INDEX];

	auto axioms = data->axioms->clone();
	auto conjectures = data->conjectures->clone();

	// Determine whether the theories are compatible with this inference
	// and whether arithmetic support is required
	TheorySupportedChecker sc;
	sc.runCheck(axioms);
	if (sc.definitionFound()) {
		Info::print("Replacing a definition by its (potentially weaker) completion. "
				"The prover may wrongly decide that the first theory does not entail the second theory.");
		FormulaUtils::addCompletion(axioms);
		sc.definitionFound(false);
	}
	sc.runCheck(conjectures);
	if (!sc.theorySupported()) {
		Error::error("\"entails\" is not supported for the given theories. "
				"Only first-order theories (with arithmetic) are supported, with the addition of "
				"definitions in the axiom theory. (No aggregates, fixpoint definitions,...)\n");
	}
	if (sc.definitionFound()) {
		Error::error("Definitions in the conjecture are not supported for \"entails\".\n");
		return State::UNKNOWN;
	}
	bool arithmeticFound = sc.arithmeticFound();

	// Turn functions into predicates (for partial function support)
	FormulaUtils::unnestTerms(axioms);
	axioms = FormulaUtils::graphFuncsAndAggs(axioms);

	FormulaUtils::unnestTerms(conjectures);
	conjectures = FormulaUtils::graphFuncsAndAggs(conjectures);

	// Clean up possibly existing files
	remove(".tptpfile.tptp");
	remove(".tptpresult.txt");

	std::stringstream stream;
	TPTPPrinter<std::stringstream>* printer;
	try {
		printer = dynamic_cast<TPTPPrinter<std::stringstream>*>(Printer::create<std::stringstream>(stream, arithmeticFound));
	} catch (std::bad_cast&) {
		Error::error("\"entails\" requires the printer to be set to the TPTPPrinter.\n");
		return State::UNKNOWN;
	}

	// Print the theories to a TPTP file
	printer->print(axioms->vocabulary());
	printer->print(axioms);
	printer->conjecture(true);
	if (axioms->vocabulary() != conjectures->vocabulary()) {
		printer->print(conjectures->vocabulary());
	}
	printer->print(conjectures);
	delete (printer);

	std::ofstream tptpFile;
	tptpFile.open(".tptpfile.tptp");
	if (!tptpFile.is_open()) {
		Error::error("Could not successfully open file \".tptpfile.tptp\" for writing. "
				"Check whether you have write rights in the current directory.\n");
		return State::UNKNOWN;
	}
	tptpFile << stream.str();
	tptpFile.close();

	// Assemble the command of the prover.
	std::vector<InternalArgument> command;
	if (arithmeticFound) {
		if (tffCommand._type != AT_TABLE) {
			Error::error("No prover command was specified. Please add a prover command to .idprc that "
					"accepts input in TPTP TFF syntax.\n");
			return State::UNKNOWN;
		}
		command = *tffCommand._value._table;
	} else {
		if (fofCommand._type != AT_TABLE) {
			Error::error("No prover command was specified. Please add a prover command to .idprc that "
					"accepts input in TPTP FOF syntax.\n");
			return State::UNKNOWN;
		}
		command = *fofCommand._value._table;
	}
	if (command.size() != 2 || command[0]._type != AT_STRING || command[1]._type != AT_STRING) {
		Error::error("The prover command must contain 2 strings: The prover application and its "
				"arguments. Please check your .idprc file.\n");
		return State::UNKNOWN;
	}
	std::string& arguments = *command[1]._value._string;

	std::stringstream applicationStream;
	applicationStream << getenv("PROVERDIR") << "/" << *command[0]._value._string;

	if (access(applicationStream.str().c_str(), X_OK)) {
		Error::error("Prover application:\n" + applicationStream.str() + "\nnot found or not executable. "
				"Please check your .idprc file or set the PROVERDIR environment variable.\n");
		return State::UNKNOWN;
	}

	auto pos = arguments.find("%i");
	if (pos == std::string::npos) {
		Error::error("The argument string for the prover must indicate where the input file "
				"must be inserted. (Marked by '%i')\n");
		return State::UNKNOWN;
	}
	arguments.replace(pos, 2, ".tptpfile.tptp");
	pos = arguments.find("%o");
	if (pos == std::string::npos) {
		// If %o was not found, assume output redirection
		arguments += " > %o";
		pos = arguments.find("%o");
	}
	arguments.replace(pos, 2, ".tptpresult.txt");

	// Call the prover with timeout.
	auto callresult = system((applicationStream.str() + " " + arguments).c_str());
	// TODO call the prover with the prover timeout
	if(callresult!=0){
		Info::print("The theorem prover did not finish within the specified timeout.");
		return State::UNKNOWN;
	}

	std::vector<InternalArgument> theoremStrings;
	std::vector<InternalArgument> counterSatisfiableStrings;
	if (arithmeticFound) {
		theoremStrings = data->tffTheoremStrings;
		counterSatisfiableStrings = data->tffCounterSatisfiableStrings;
	} else {
		theoremStrings = data->fofTheoremStrings;
		counterSatisfiableStrings = data->fofCounterSatisfiableStrings;
	}

	// Retrieve the status from the result
	std::string line;
	std::ifstream tptpResult;
	tptpResult.open(".tptpresult.txt");
	if (!tptpResult.is_open()) {
		Error::error("Could not open file \".tptpresult.txt\" for reading.\n");
	}
	getline(tptpResult, line);
	pos = std::string::npos;
	bool result = false;
	while (pos == std::string::npos && !tptpResult.eof()) {
		unsigned int i = 0;
		while (i < theoremStrings.size() && pos == std::string::npos) {
			pos = line.find(*theoremStrings[i]._value._string);
			result = true;
			i++;
		}
		i = 0;
		while (i < counterSatisfiableStrings.size() && pos == std::string::npos) {
			pos = line.find(*counterSatisfiableStrings[i]._value._string);
			result = false;
			i++;
		}
		getline(tptpResult, line);
	}
	tptpResult.close();

#ifndef DEBUG
	remove(".tptpfile.tptp");
	remove(".tptpresult.txt");
#endif

	if (pos == std::string::npos) {
		Info::print("The automated theorem prover gave up or stopped in an irregular state.");
		return State::UNKNOWN;
	}

	return result ? State::PROVEN : State::DISPROVEN;
}
