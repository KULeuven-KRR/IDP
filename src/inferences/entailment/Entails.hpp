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

#include <vector>
#include <string>
class Theory;

enum class State {
	PROVEN, DISPROVEN, UNKNOWN
};

class Entails {
private:
	std::string command;
	Theory *axioms, *conjectures;
	bool hasArithmetic; // Note: not writing out arithmetic format output if none is present allows to use more provers.
	std::vector<std::string> provenStrings, disprovenStrings;

public:
	static State doCheckEntailment(const std::string& command, const Theory* axioms, const Theory* conjectures);

private:
	Entails(const std::string& command, Theory* axioms, Theory* conjectures);
	State checkEntailment();
};
