/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "print.hpp"
#include "data.hpp"
#include <iostream>

void PrintTheory::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	Printer* printer = Printer::create();
	Theory* t = dynamic_cast<Theory*>(args[0]._theory);
	printer->print(t);
	delete(printer);
}

void PrintVocabulary::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	Printer* printer = Printer::create();
	printer->print(args[0]._vocabulary);
	delete(printer);
}

void PrintStructure::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	Printer* printer = Printer::create();
	Structure* s = dynamic_cast<Structure*>(args[0]._structure);
	printer->print(s);
	delete(printer);
}

void PrintNamespace::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	// TODO
	//string s = (args[0]._namespace)->to_string();
	//cout << s;
}

void PushNegations::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
}

void RemoveEquivalences::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
}

void FlattenFormulas::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
}

void RemoveEqchains::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
}

GroundingInference::GroundingInference() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;	
	_description = "Ground the theory and structure and print the grounding";
}

void GroundingInference::execute(const vector<InfArg>& args, const string& res, Namespace*) const {
	assert(args.size() == 2);
	AbstractTheory* t = args[0]._theory;
	AbstractStructure* s = args[1]._structure;
	TheoryUtils::move_functions(t);
	NaiveGrounder ng(s);	
	AbstractTheory* gr = ng.ground(t);
	TheoryUtils::remove_eqchains(gr);
	TheoryUtils::reduce(gr,s);
	TheoryUtils::tseitin(gr);
	EcnfTheory* ecnfgr = TheoryUtils::convert_to_ecnf(gr);
	GroundPrinter* printer = new outputECNF(stdout);
	ecnfgr->print(printer);
	gr->recursiveDelete();
	delete(ecnfgr);
	delete(printer);
}

GroundingWithResult::GroundingWithResult() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_THEORY;	
	_description = "Ground the theory and structure and store the grounding";
}

void GroundingWithResult::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	assert(args.size() == 2);
	NaiveGrounder ng(args[1]._structure);
	AbstractTheory* gr = ng.ground(args[0]._theory);
	gr->name(res);
	cn->add(gr);
}

ModelExpansionInference::ModelExpansionInference() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;
	_description = "Performs model expansion on the structure given the theory it should satisfy.";
}

void ModelExpansionInference::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	assert(args.size() == 2);
	AbstractTheory* t = args[0]._theory;
	AbstractStructure* s = args[1]._structure;
	TheoryUtils::move_functions(t);
	NaiveGrounder ng(s);	
	AbstractTheory* gr = ng.ground(t);
	TheoryUtils::remove_eqchains(gr);
	TheoryUtils::reduce(gr,s);
	TheoryUtils::tseitin(gr);
	EcnfTheory* ecnfgr = TheoryUtils::convert_to_ecnf(gr);
	ECNF_mode modes;
	modes.nbmodels = 1;
	SATSolver* solver = new SATSolver(modes);
	GroundPrinter* printer = new outputToSolver(solver);
	ecnfgr->print(printer);
	gr->recursiveDelete();
	vector<vector<Literal> > models;
	bool sat = solver->solve(models);
	//example use
	if(sat){
		for(int i=0; i<models.size(); i++){
			cout <<"=== Model " << (i+1) << " ===\n";
			for(int j=0; j<models[i].size(); j++){
				if(models[i][j].getAtom().getValue() > 0) {
					//cout <<models[i][j] <<" ";
					cout << ecnfgr->translator()->symbol((models[i][j].getAtom().getValue())-1)->to_string() << '(';
					vector<string> args = ecnfgr->translator()->args(models[i][j].getAtom().getValue()-1);
					for(unsigned int n = 0; n < args.size(); ++n) {
						cout << args[n];
						if(n < args.size()-1) cout << ',';
					}
					cout << "). ";
				}
			}
			cout <<"\n\n";
		}
	}
	delete(solver);
	delete(ecnfgr);
	delete(printer);
}

void StructToTheory::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	assert(args.size() == 1);
	AbstractTheory* t = StructUtils::convert_to_theory(args[0]._structure);
	t->name(res);
	cn->add(t);
}

void MoveQuantifiers::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	TheoryUtils::move_quantifiers(args[0]._theory);
}

void ApplyTseitin::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	TheoryUtils::tseitin(args[0]._theory);
}

GroundReduce::GroundReduce() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;
	_description = "Replace ground atoms in the theory by their truth value in the structure";
}

void GroundReduce::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	TheoryUtils::reduce(args[0]._theory,args[1]._structure);
}

void MoveFunctions::execute(const vector<InfArg>& args, const string& res, Namespace* cn) const {
	TheoryUtils::move_functions(args[0]._theory);
}
