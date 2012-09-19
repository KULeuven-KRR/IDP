/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef PRINTBDD_HPP_
#define PRINTBDD_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "printers/bddprinter.hpp"


#include "options.hpp"

template<class Object>
void printAsBDD(Object o) {
	auto printer = new BDDPrinter<std::ostream>(std::clog);
	printer->startTheory();
	printer->print(o);
	printer->endTheory();
	delete(printer);
}

typedef TypedInference<LIST(AbstractTheory*)> PrintTheoryInferenceBase;
class PrintAsBDDInference: public PrintTheoryInferenceBase {
public:
	PrintAsBDDInference()
			: PrintTheoryInferenceBase("printasbdd", "Prints the given theory as a bdd: prints all of its formulas as a bdd.") {
		setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		printAsBDD(theory);
		return InternalArgument();
	}
};


#endif /* PRINTBDD_HPP_ */
