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

#ifndef CREATETUPLE_HPP_
#define CREATETUPLE_HPP_

#include "commandinterface.hpp"

class CreateTupleInference: public EmptyBase {
public:
	CreateTupleInference()
			: EmptyBase("createdummytuple", "Create a dummy empty tuple") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>&) const {
		InternalArgument ia;
		ia._type = AT_TUPLE;
		ia._value._tuple = 0;
		return ia;
	}
};

#endif /* CREATETUPLE_HPP_ */
