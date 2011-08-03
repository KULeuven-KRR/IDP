/************************************
	implies.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef IMPLIES_HPP_
#define IMPLIES_HPP_

#include "commandinterface.hpp"
#include "printers/print.hpp"
#include "theory.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>

class ArithmeticDetector : public TheoryVisitor {
	private:
		bool		_arithmeticFound;
	public:
		ArithmeticDetector() : _arithmeticFound(false) { }
		void visit(const FuncTerm* f);
		void visit(const EqChainForm* f);
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
	std::string arithmeticFunction [6] = {"+", "-", "/", "*", "%", "abs"};
	for(unsigned int n = 0; n < 5; ++n) {
		if(f->function()->to_string(false) == arithmeticFunction[n]) {
			_arithmeticFound = true;
		}
	}
	if(!_arithmeticFound) {
		traverse(f);
	}
}

class ImpliesInference: public Inference {
public:
	ImpliesInference(): Inference("implies") {
		add(AT_THEORY);
		add(AT_THEORY);
		add(AT_STRING);
		add(AT_STRING);
		add(AT_OPTIONS);
	}
	
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		// Doe wat voorverwerking van de theories.
		Theory* axioms = dynamic_cast<Theory*>(args[0].theory()->clone());
		Theory* conjectures = dynamic_cast<Theory*>(args[1].theory()->clone());
		// TODO: What if the cast fails?
		
		Options* opts = args[4].options();
		
		ArithmeticDetector ad;
		axioms->accept(&ad);
		conjectures->accept(&ad);
		bool arithmeticFound = ad.arithmeticFound();
		
		TheoryUtils::completion(axioms);
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
		Printer* printer = Printer::create<std::stringstream>(opts, stream, false, arithmeticFound);
		
		printer->visit(axioms->vocabulary());
		printer->visit(axioms);
		delete(printer);
		printer = Printer::create<std::stringstream>(opts, stream, true, arithmeticFound);
		if(axioms->vocabulary() != conjectures->vocabulary())
			printer->visit(conjectures->vocabulary());
		printer->visit(conjectures);
		delete(printer);
		
		std::ofstream tptpFile;
		tptpFile.open(".tptpfile.tptp");
		tptpFile << stream.str();
		tptpFile.close();
		
		// TODO remove me
		std::cout << stream.str();
		
		// Stuff die de externe dinges oproept.
		std::string& fofCommandString = *args[2]._value._string;
		std::string& tffCommandString = *args[3]._value._string; // TODO: after arithmetic test, use TFF version if result is true
		std::stringstream commandStream;
		if (arithmeticFound)
			commandStream << tffCommandString;
		else
			commandStream << fofCommandString;
		commandStream << ".tptpfile.tptp > .tptpresult.txt";
		system(commandStream.str().c_str());
		
		// Haal daar het antwoord uit. "SZS status ... "
		std::string line;
		std::ifstream tptpResult;
		tptpResult.open(".tptpresult.txt");
		getline(tptpResult, line);
		while(line.find("SZS status") == std::string::npos && !tptpResult.eof()) {
			getline(tptpResult, line);
		}
		if(tptpResult.eof()) {
			tptpResult.close();
			// TODO uncomment
			//remove(".tptpfile.tptp");
			//remove(".tptpresult.txt");
			return nilarg();
		}
		
		tptpResult.close();
		
		// TODO uncomment
		//remove(".tptpfile.tptp");
		//remove(".tptpresult.txt");
		
		bool result = false;
		std::string acceptStrings [12] = {"Theorem", "Equivalent", "TautologousConclusion", "WeakerConclusion", "EquivalentTheorem", "Tautology", "WeakerTautologousConclusion", "WeakerTheorem", "ContradictoryAxioms", "SatisfiableConclusionContradictoryAxioms", "TautologousConclusionContradictoryAxioms", "WeakerConclusionContradictoryAxioms"};
		std::string unkStrings [22] = {"NoSuccess", "Open", "Unknown", "Assumed", "Stopped", "Error", "OSError", "InputError", "SyntaxError", "SemanticError", "TypeError", "Forced", "User", "ResourceOut", "Timeout", "MemoryOut", "GaveUp", "Incomplete", "Inappropriate", "InProgress", "NotTried", "NotTriedYet"};
		
		// Match on the strings
		// TODO: make sure that there is only whitespace following, but not necessary I think
		for(unsigned int n = 0; n < 22; n++) {
			if(line.find("SZS status " + unkStrings[n]) != std::string::npos) {
				return nilarg();
			}
		}
		for(unsigned int n = 0; n < 12; n++) {
			if(line.find("SZS status " + acceptStrings[n]) != std::string::npos) {
				result = true;
			}
		}
		
		InternalArgument ia = InternalArgument();
		ia._type = AT_BOOLEAN;
		ia._value._boolean = result;
		return ia;
	}

};

#endif /* IMPLIES_HPP_ */