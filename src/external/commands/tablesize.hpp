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

#ifndef TABLESIZEINFERENCE_HPP_
#define TABLESIZEINFERENCE_HPP_

#include "commandinterface.hpp"
#include "structure/TableSize.hpp"

class TableSizeInference: public PredTableBase {
public:
	TableSizeInference()
			: PredTableBase("size", "Get the size of the given table.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pt = get<0>(args);
		const auto& size = pt->size();

		InternalArgument ia;
		ia._type = AT_INT;

		switch (size._type) {
		case TST_INFINITE:
			ia._value._int = -1;
			break; // TODO maybe a bit ugly?
		case TST_EXACT:
			ia._value._int = size._size;
			break;
		case TST_APPROXIMATED:
			ia._value._int = size._size;
			break; // TODO notify that it is approximate?
		}
		return ia;
	}
};

#endif /* TABLESIZEINFERENCE_HPP_ */
