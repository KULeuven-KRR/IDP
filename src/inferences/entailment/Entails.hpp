/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ENTAILS_HPP_
#define ENTAILS_HPP_

#include <vector>
class Options;
class AbstractTheory;
class InternalArgument;

// TODO remove internalargument dependency
// TODO cleanup code

enum class State {
	PROVEN, DISPROVEN, UNKNOWN
};

struct EntailmentData {
	std::vector<InternalArgument> fofCommands;
	std::vector<InternalArgument> tffCommands;
	std::vector<InternalArgument> fofTheoremStrings;
	std::vector<InternalArgument> fofCounterSatisfiableStrings;
	std::vector<InternalArgument> tffTheoremStrings;
	std::vector<InternalArgument> tffCounterSatisfiableStrings;
	AbstractTheory* axioms;
	AbstractTheory* conjectures;
};

class Entails {
public:
	static State doCheckEntailment(EntailmentData* data) {
		Entails c;
		return c.checkEntailment(data);
	}

private:
	State checkEntailment(EntailmentData* data) const;
};

#endif /* ENTAILS_HPP_ */
