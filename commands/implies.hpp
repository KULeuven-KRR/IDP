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
#include <stdio.h>
#include <string>
#include "vocabulary.hpp"

class ArithmeticDetector : public TheoryVisitor {
	private:
		bool		_arithmeticFound;
	public:
		ArithmeticDetector() : _arithmeticFound(false) { }
		void visit(const FuncTerm* f);
		void visit(const EqChainForm* f);
		void visit(const Rule* r);
		bool arithmeticFound() { return _arithmeticFound; }
};

void ArithmeticDetector::visit(const EqChainForm* f) {
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

void ArithmeticDetector::visit(const FuncTerm* f) {
	std::string arithmeticFunction [10] = {"+", "-", "/", "*", "%", "abs", "MAX", "MIN", "SUCC", "PRED"};
	for(unsigned int n = 0; n < 5; ++n) {
		if(f->function()->to_string(false) == arithmeticFunction[n]) {
			_arithmeticFound = true;
		}
	}
	if(!_arithmeticFound) {
		traverse(f);
	}
}

void ArithmeticDetector::visit(const Rule* r) {
	
}

// TODO: Neem support checks en arithmetic detection samen!
// class TheorySupportChecker : public TheoryVisitor {
// 	private:
// 		bool		_theorySupported;
// 	public:
// 		TheorySupportChecker() : _theorySupported(true) { }
// 		void visit(const FuncTerm* f);
// 		void visit(EqChainForm* f);
// 		bool theorySupported() { return _theorySupported; }
// };

class ImpliesInference: public Inference {
public:
	ImpliesInference(): Inference("implies") {
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
		Error::error("Implies inference is not supported on this platform.\n");
		return nilarg();
		#endif
		
		Theory* axioms;
		Theory* conjectures;
		try {
			axioms = dynamic_cast<Theory*>(args[0].theory()->clone());
			conjectures = dynamic_cast<Theory*>(args[1].theory()->clone());
		} catch (std::bad_cast) {
			// TODO error?
			Error::error("Implies can only take regular Theory objects as axioms and conjectures.\n");
			return nilarg();
		}
		
		Options* opts = args[8].options();
		
		// Determine whether the theories are compatible with this inference and whether arithmetic support is required
		ArithmeticDetector ad;
		axioms->accept(&ad);
		conjectures->accept(&ad);
		bool arithmeticFound = ad.arithmeticFound();
		
		// Replace definitions by their completion
		TheoryUtils::completion(axioms);
		
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
		
		//arithmeticFound = true; // TODO REMOVE ME
		
		std::stringstream stream;
		TPTPPrinter<std::stringstream>* printer;
		try {
			printer = dynamic_cast<TPTPPrinter<std::stringstream>*>(Printer::create<std::stringstream>(opts, stream, arithmeticFound));
		} catch (std::bad_cast) {
			// TODO error?
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
		// TODO: Hoe itereren?
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
		
		// TODO uncomment
		//remove(".tptpfile.tptp");
		//remove(".tptpresult.txt");
		
		if(pos == std::string::npos) {
			Error::error("The prover did not finish in time or stopped in an irregular state.\n");
			return nilarg();
		}
		
		InternalArgument ia = InternalArgument();
		ia._type = AT_BOOLEAN;
		ia._value._boolean = result;
		return ia;
	}

};

#endif /* IMPLIES_HPP_ */