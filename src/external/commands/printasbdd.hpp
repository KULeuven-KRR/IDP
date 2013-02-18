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

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "printers/bddprinter.hpp"

#include "options.hpp"

typedef TypedInference<LIST(AbstractTheory*)> PrintTheoryInferenceBase;
class PrintAsBDDInference: public PrintTheoryInferenceBase {
public:
	PrintAsBDDInference()
			: PrintTheoryInferenceBase("printasbdd", "Prints the given theory as a bdd: prints all of its formulas as a bdd.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);

		auto printer = new BDDPrinter<std::ostream>(std::cout);
		printer->startTheory();
		printer->print(theory);
		printer->endTheory();
		delete(printer);

		return InternalArgument();
	}
};
