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
#include "printers/print.hpp"
#include "IncludeComponents.hpp"
#include "theory/Query.hpp"

#include "options.hpp"

template<class Object>
std::string printToString(Object o) {
	std::stringstream stream;
	auto printer = Printer::create<std::stringstream>(stream);
	printer->startTheory();
	printer->print(o);
	printer->endTheory();
	delete(printer);
	return stream.str();
}

template<typename Base>
class PrintInference: public TypedInference<Base> {
public:
	PrintInference()
			: TypedInference<Base>("tostring", "Prints the given object.") {
		TypedInference<Base>::setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto a = this-> template get<0>(args);
		return InternalArgument(new std::string(printToString(a)));
	}
};

