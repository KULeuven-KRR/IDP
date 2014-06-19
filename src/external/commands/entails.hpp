/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "inferences/entailment/Entails.hpp"
#include "errorhandling/error.hpp"

typedef std::vector<InternalArgument> ialist;
typedef TypedInference<LIST(AbstractTheory*, AbstractTheory*)> EntailsInferenceBase;
class EntailsInference: public EntailsInferenceBase {
public:
	EntailsInference()
			: EntailsInferenceBase("entails", "Checks whether the first theory entails the second, using the set prover and arguments. ") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		if (not isa<Theory>(*get<0>(args)) || not isa<Theory>(*get<1>(args))) {
			throw IdpException("Entails only accepts regular Theory objects (not ground theories) as axioms and conjectures.");
		}

		auto axioms = dynamic_cast<Theory*>(get<0>(args)->clone());
		auto conjectures = dynamic_cast<Theory*>(get<1>(args)->clone());
		auto state = Entails::doCheckEntailment(axioms, conjectures);
		delete (axioms);
		delete (conjectures);

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
