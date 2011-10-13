/************************************
	entails.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ENTAILS_HPP_
#define ENTAILS_HPP_

#include "commandinterface.hpp"
#include "printers/print.hpp"
#include "printers/tptpprinter.hpp"
#include "theory.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include "vocabulary.hpp"
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

class TheorySupportedChecker : public TheoryVisitor {
	private:
		bool		_arithmeticFound;
		bool		_theorySupported;
		bool		_definitionFound;
	public:
		TheorySupportedChecker() : _arithmeticFound(false), _theorySupported(true), _definitionFound(false) { }
		void visit(const FuncTerm* f);
		void visit(const EqChainForm* f);
		void visit(const EnumSetExpr*) { _theorySupported = false; }
		void visit(const QuantSetExpr*) { _theorySupported = false; }
		void visit(const AggTerm*) { _theorySupported = false; }
		void visit(const FixpDef*) { _theorySupported = false; }
		void visit(const Definition*) { _definitionFound = true; }
		bool arithmeticFound() { return _arithmeticFound; }
		bool theorySupported() { return _theorySupported; }
		bool definitionFound() { return _definitionFound; }
		void definitionFound(bool definitionFound) { _definitionFound = definitionFound; }
		void theorySupported(bool theorySupported) { _theorySupported = theorySupported; }
		void arithmeticFound(bool arithmeticFound) { _arithmeticFound = arithmeticFound; }
};

void TheorySupportedChecker::visit(const EqChainForm* f) {
	CompType arithmeticComparator [4] = {CompType::LEQ, CompType::GEQ, CompType::LT, CompType::GT};
	for(unsigned int n = 0; n < f->comps().size(); ++n) {
		for(unsigned int i = 0; i < 4; ++i) {
			if(f->comps()[n] == arithmeticComparator[i]) {
				_arithmeticFound = true;
			}
		}
	}
	if(!_arithmeticFound) {
		traverse(f);
	}
}

void TheorySupportedChecker::visit(const FuncTerm* f) {
	std::string arithmeticFunction [10] = {"+", "-", "/", "*", "%", "abs", "MAX", "MIN", "SUCC", "PRED"};
	for(unsigned int n = 0; n < 5; ++n) {
		if(f->function()->toString(false) == arithmeticFunction[n])
			_arithmeticFound = true;
	}
	if(f->function()->toString(false) == "%")
		_theorySupported = false;
	if(!_arithmeticFound) {
		traverse(f);
	}
}

namespace Entails {
	static jmp_buf _timeoutJump;
	void timeout(int) {
		longjmp(_timeoutJump, 1);
	}
}

class EntailsInference: public Inference {
public:
	EntailsInference(): Inference("entails") {
		add(AT_THEORY);
		add(AT_THEORY);
		// Prover commands
		add(AT_TABLE); // fof
		add(AT_TABLE); // tff
		// theorem/countersatisfiable strings
		add(AT_TABLE); // fof theorem
		add(AT_TABLE); // fof countersatisfiable
		add(AT_TABLE); // tff theorem
		add(AT_TABLE); // tff countersatisfiable
		add(AT_OPTIONS);
	}

	// TODO passing options as internalarguments (e.g. the xsb path) is very ugly and absolutely not intended!
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		#if defined(__linux__)
		#define COMMANDS_INDEX 0
		#elif defined(_WIN32)
		#define COMMANDS_INDEX 1
		#elif defined(__APPLE__)
		#define COMMANDS_INDEX 2
		#else
		Error::error("\"entails\" is not supported on this platform.\n");
		return nilarg();
		#endif
		
		std::vector<InternalArgument>& fofCommands = *args[2]._value._table;
		std::vector<InternalArgument>& tffCommands = *args[3]._value._table;

		std::vector<InternalArgument>& fofTheoremStrings = *args[4]._value._table;
		std::vector<InternalArgument>& fofCounterSatisfiableStrings = *args[5]._value._table;
		std::vector<InternalArgument>& tffTheoremStrings = *args[6]._value._table;
		std::vector<InternalArgument>& tffCounterSatisfiableStrings = *args[7]._value._table;

		#ifdef COMMANDS_INDEX
		InternalArgument& fofCommand = fofCommands[COMMANDS_INDEX];
		InternalArgument& tffCommand = tffCommands[COMMANDS_INDEX];
		#endif
		
		Theory* axioms;
		Theory* conjectures;
		try {
			axioms = dynamic_cast<Theory*>(args[0].theory()->clone());
			conjectures = dynamic_cast<Theory*>(args[1].theory()->clone());
		} catch (std::bad_cast&) {
			Error::error("\"entails\" can only take regular Theory objects as axioms and conjectures.\n");
			return nilarg();
		}
		
		Options* opts = args[8].options();
		
		// Determine whether the theories are compatible with this inference
		// and whether arithmetic support is required
		TheorySupportedChecker sc;
		axioms->accept(&sc);
		if (sc.definitionFound()) {
			Info::print("Replacing a definition by its (potentially weaker) completion. "
					"The prover may wrongly decide that the first theory does not entail the second theory.");
			TheoryUtils::completion(axioms);
			sc.definitionFound(false);
		}
		conjectures->accept(&sc);
		if (!sc.theorySupported()) {
			Error::error("\"entails\" is not supported for the given theories. "
					"Only first-order theories (with arithmetic) are supported, with the addition of "
					"definitions in the axiom theory. (No aggregates, fixpoint definitions,...)\n");
		}
		if (sc.definitionFound()) {
			Error::error("Definitions in the conjecture are not supported for \"entails\".\n");
			return nilarg();
		}
		bool arithmeticFound = sc.arithmeticFound();
		
		// Turn functions into predicates (for partial function support)
		TheoryUtils::removeNesting(axioms);
		TheoryUtils::removeNesting(conjectures);
		for(auto it = axioms->sentences().begin(); it != axioms->sentences().end(); ++it) {
			FormulaUtils::graphFunctions(*it);
		}
		for(auto it = conjectures->sentences().begin(); it != conjectures->sentences().end(); ++it) {
		 	FormulaUtils::graphFunctions(*it);
		}
		
		// Clean up possibly existing files
		remove(".tptpfile.tptp");
		remove(".tptpresult.txt");

		std::stringstream stream;
		TPTPPrinter<std::stringstream>* printer;
		try {
			printer = dynamic_cast<TPTPPrinter<std::stringstream>*>(Printer::create<std::stringstream>(opts, stream, arithmeticFound));
		} catch (std::bad_cast&) {
			Error::error("\"entails\" requires the printer to be set to the TPTPPrinter.\n");
			return nilarg();
		}
		
		// Print the theories to a TPTP file
		printer->visit(axioms->vocabulary());
		printer->visit(axioms);
		printer->conjecture(true);
		if(axioms->vocabulary() != conjectures->vocabulary())
			printer->visit(conjectures->vocabulary());
		printer->visit(conjectures);
		delete(printer);
		
		std::ofstream tptpFile;
		tptpFile.open(".tptpfile.tptp");
		if(!tptpFile.is_open()) {
			Error::error("Could not successfully open file \".tptpfile.tptp\" for writing. "
					"Check whether you have write rights in the current directory.\n");
			return nilarg();
		}
		tptpFile << stream.str();
		tptpFile.close();
		
		// Assemble the command of the prover.
		std::vector<InternalArgument> command;
		if (arithmeticFound) {
			if(tffCommand._type != AT_TABLE) {
				Error::error("No prover command was specified. Please add a prover command to .idprc that "
						"accepts input in TPTP TFF syntax.\n");
				return nilarg();
			}
			command = *tffCommand._value._table;
		}
		else {
			if(fofCommand._type != AT_TABLE) {
				Error::error("No prover command was specified. Please add a prover command to .idprc that "
						"accepts input in TPTP FOF syntax.\n");
				return nilarg();
			}
			command = *fofCommand._value._table;
		}
		if(command.size() != 2 || command[0]._type != AT_STRING || command[1]._type != AT_STRING) {
			Error::error("The prover command must contain 2 strings: The prover application and its "
					"arguments. Please check your .idprc file.\n");
			return nilarg();
		}
		std::string& arguments = *command[1]._value._string;

		std::stringstream applicationStream;
		applicationStream << getenv("PROVERDIR") << "/" << *command[0]._value._string;

		if (access(applicationStream.str().c_str(), X_OK)) {
			Error::error("Prover application:\n" + applicationStream.str() + "\nnot found or not executable. "
					"Please check your .idprc file or set the PROVERDIR environment variable.\n");
			return nilarg();
		}

		auto pos = arguments.find("%i");
		if(pos == std::string::npos) {
			Error::error("The argument string for the prover must indicate where the input file "
					"must be inserted. (Marked by '%i')\n");
			return nilarg();
		}
		arguments.replace(pos, 2, ".tptpfile.tptp");
		pos = arguments.find("%o");
		if(pos == std::string::npos) {
			// If %o was not found, assume output redirection
			arguments += " > %o";
			pos = arguments.find("%o");
		}
		arguments.replace(pos, 2, ".tptpresult.txt");

		// Call the prover with timeout.
		// TODO replace signals with sigaction structures
		if (!setjmp(Entails::_timeoutJump)) {
			signal(SIGALRM, &Entails::timeout);
			ualarm(opts->getValue(IntType::PROVERTIMEOUT), 0);
			system((applicationStream.str() + " " + arguments).c_str());
		}
		else {
			Info::print("The theorem prover did not finish within the specified timeout.");
			return nilarg();
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);

		std::vector<InternalArgument> theoremStrings;
		std::vector<InternalArgument> counterSatisfiableStrings;
		if(arithmeticFound) {
			theoremStrings = tffTheoremStrings;
			counterSatisfiableStrings = tffCounterSatisfiableStrings;
		}
		else {
			theoremStrings = fofTheoremStrings;
			counterSatisfiableStrings = fofCounterSatisfiableStrings;
		}
		
		// Retrieve the status from the result
		std::string line;
		std::ifstream tptpResult;
		tptpResult.open(".tptpresult.txt");
		if(!tptpResult.is_open()) {
			Error::error("Could not open file \".tptpresult.txt\" for reading.\n");
		}
		getline(tptpResult, line);
		pos = std::string::npos;
		bool result = false;
		while(pos == std::string::npos && !tptpResult.eof()) {
			unsigned int i = 0;
			while(i < theoremStrings.size() && pos == std::string::npos) {
				pos = line.find(*theoremStrings[i]._value._string);
				result = true;
				i++;
			}
			i = 0;
			while(i < counterSatisfiableStrings.size() && pos == std::string::npos) {
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
		
		if(pos == std::string::npos) {
			Info::print("The automated theorem prover gave up or stopped in an irregular state.");
			return nilarg();
		}
		
		InternalArgument ia = InternalArgument();
		ia._type = AT_BOOLEAN;
		ia._value._boolean = result;
		return ia;
	}
};

#endif /* ENTAILS_HPP_ */
