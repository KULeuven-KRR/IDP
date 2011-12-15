/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef ENTAILSINFERENCE_HPP_
#define ENTAILSINFERENCE_HPP_

#include "inferences/Entails.hpp"
#include "error.hpp"

class EntailsInference: public Inference {
public:
	EntailsInference() :
			Inference("entails") {
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
		EntailmentData* data = new EntailmentData();

		data->fofCommands = *args[2]._value._table;
		data->tffCommands = *args[3]._value._table;

		data->fofTheoremStrings = *args[4]._value._table;
		data->fofCounterSatisfiableStrings = *args[5]._value._table;
		data->tffTheoremStrings = *args[6]._value._table;
		data->tffCounterSatisfiableStrings = *args[7]._value._table;

		data->axioms = args[0].theory()->clone();
		data->conjectures = args[1].theory()->clone();
		if (not sametypeid<Theory>(*data->axioms) || not sametypeid<Theory>(*data->conjectures)) {
			Error::error("\"entails\" can only take regular Theory objects as axioms and conjectures.\n");
			return nilarg();
		}

		data->options = args[8].options();

		State state = Entails::doCheckEntailment(data);
		delete (data);
		switch (state) {
		case State::PROVEN:
			return InternalArgument(true);
		case State::DISPROVEN:
			return InternalArgument(false);
		case State::UNKNOWN:
			return nilarg();
		}
	}
}
;

#endif /* ENTAILSINFERENCE_HPP_ */
