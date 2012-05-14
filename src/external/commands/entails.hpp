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

#include "commandinterface.hpp"
#include "inferences/entailment/Entails.hpp"
#include "errorhandling/error.hpp"

typedef std::vector<InternalArgument> ialist;
typedef TypedInference<LIST(AbstractTheory*, AbstractTheory*, ialist*, ialist*, ialist*, ialist*, ialist*, ialist*)> EntailsInferenceBase;
class EntailsInference: public EntailsInferenceBase {
public:
	EntailsInference()
			: EntailsInferenceBase("entails", "Checks whether the first theory entails the second. ") {
		setNameSpace(getInternalNamespaceName());
		/*		add(AT_THEORY);
		 add(AT_THEORY);
		 // Prover commands
		 add(AT_TABLE); // fof
		 add(AT_TABLE); // tff
		 // theorem/countersatisfiable strings
		 add(AT_TABLE); // fof theorem
		 add(AT_TABLE); // fof countersatisfiable
		 add(AT_TABLE); // tff theorem
		 add(AT_TABLE); // tff countersatisfiable
		 add(AT_OPTIONS);*/
	}

	// TODO passing options as internalarguments (e.g. the xsb path) is very ugly and absolutely not intended!
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		EntailmentData* data = new EntailmentData();
		//TODO: check that no input is changed by this command.
		data->fofCommands = *get<2>(args);
		data->tffCommands = *get<3>(args);

		data->fofTheoremStrings = *get<4>(args);
		data->fofCounterSatisfiableStrings = *get<5>(args);
		data->tffTheoremStrings = *get<6>(args);
		data->tffCounterSatisfiableStrings = *get<7>(args);

		data->axioms = get<0>(args)->clone();
		data->conjectures = get<1>(args)->clone();
		if (not sametypeid<Theory>(*data->axioms) || not sametypeid<Theory>(*data->conjectures)) {
			Error::error("\"entails\" can only take regular Theory objects as axioms and conjectures.");
			return nilarg();
		}

		State state = Entails::doCheckEntailment(data);
		delete (data);

		switch (state) {
		case State::PROVEN:
			return InternalArgument(true);
		case State::DISPROVEN:
			return InternalArgument(false);
		default:
			return nilarg();
		}
	}
};

#endif /* ENTAILSINFERENCE_HPP_ */
