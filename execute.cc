/************************************
	execute.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.h"
#include <iostream>

void PrintTheory::execute(const vector<InfArg>& args, InfArg res) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._theory)->to_string();
	cout << s;
}

void PrintVocabulary::execute(const vector<InfArg>& args, InfArg res) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._vocabulary)->to_string();
	cout << s;
}

void PrintStructure::execute(const vector<InfArg>& args, InfArg res) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._structure)->to_string();
	cout << s;
}

void PrintNamespace::execute(const vector<InfArg>& args, InfArg res) const {
	assert(args.size() == 1);
	// TODO
	//string s = (args[0]._namespace)->to_string();
	//cout << s;
}

void PushNegations::execute(const vector<InfArg>& args, InfArg res) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
}

