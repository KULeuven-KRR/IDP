/************************************
	implies.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef IMPLIES_HPP_
#define IMPLIES_HPP_

#include "commandinterface.hpp"
#include "printers/print.hpp"
#include "printers/tptpprinter.hpp"
#include "theory.hpp"
#include <cstdlib>
#include <fstream>
#include "vocabulary.hpp"

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
	CompType arithmeticComparator [4] = {CT_LEQ, CT_GEQ, CT_LT, CT_GT};
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
		if(f->function()->to_string(false) == arithmeticFunction[n])
			_arithmeticFound = true;
	}
	if(f->function()->to_string(false) == "%")
		_theorySupported = false;
	if(!_arithmeticFound) {
		traverse(f);
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
	
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		std::vector<InternalArgument>& fofCommands = *args[2]._value._table;
		std::vector<InternalArgument>& tffCommands = *args[3]._value._table;
		
		std::vector<InternalArgument>& fofTheoremStrings = *args[4]._value._table;
		std::vector<InternalArgument>& fofCounterSatisfiableStrings = *args[5]._value._table;
		std::vector<InternalArgument>& tffTheoremStrings = *args[6]._value._table;
		std::vector<InternalArgument>& tffCounterSatisfiableStrings = *args[7]._value._table;
		
		#ifdef __linux__
		#define COMMANDS_INDEX 0
		#endif
		#ifdef _WIN32
		#define COMMANDS_INDEX 1
		#endif
		#ifdef __APPLE__
		#define COMMANDS_INDEX 2
		#endif
		
		#ifdef COMMANDS_INDEX
		std::string& fofCommandString = *fofCommands[COMMANDS_INDEX]._value._string;
		std::string& tffCommandString = *tffCommands[COMMANDS_INDEX]._value._string;
		#else
		Error::error("Determining entailment using ATP systems is not supported on this platform.\n");
		return nilarg();
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
			Info::print("Replacing a definition by its (potentially weaker) completion."
					    "The prover may wrongly decide TODO.");
			TheoryUtils::completion(axioms);
			sc.definitionFound(false);
		}
		conjectures->accept(&sc);
		if (sc.definitionFound() || !sc.theorySupported()) {
			Error::error("Definitions in the conjecture are not supported for \"entails\".");
			return nilarg();
		}
		bool arithmeticFound = sc.arithmeticFound();
		
		// Turn functions into predicates (for partial function support)
		TheoryUtils::remove_nesting(axioms);
		TheoryUtils::remove_nesting(conjectures);
		for(auto it = axioms->sentences().begin(); it != axioms->sentences().end(); ++it) {
			FormulaUtils::graph_functions(*it);
		}
		for(auto it = conjectures->sentences().begin(); it != conjectures->sentences().end(); ++it) {
		 	FormulaUtils::graph_functions(*it);
		}
		
		// Clean up possibly existing files
		remove(".tptpfile.tptp");
		remove(".tptpresult.txt");

		std::stringstream stream;
		TPTPPrinter<std::stringstream>* printer;
		try {
			printer = dynamic_cast<TPTPPrinter<std::stringstream>*>(Printer::create<std::stringstream>(opts, stream, arithmeticFound));
		} catch (std::bad_cast&) {
			Error::error("The printer type must be set to a TPTPPrinter.\n");
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
		tptpFile << stream.str();
		tptpFile.close();
		
		// Call the external prover
		std::string commandString;
		if (arithmeticFound)
			commandString = tffCommandString;
		else
			commandString = fofCommandString;
		auto pos = commandString.find("%i");
		commandString.replace(pos, 2, ".tptpfile.tptp");
		pos = commandString.find("%o");
		commandString.replace(pos, 2, ".tptpresult.txt");
		
		system(commandString.c_str());
		
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
			Error::error("The automated theorem prover did not finish in time or stopped in an irregular state.\n");
			return nilarg();
		}
		
		InternalArgument ia = InternalArgument();
		ia._type = AT_BOOLEAN;
		ia._value._boolean = result;
		return ia;
	}
};

#endif /* IMPLIES_HPP_ */
