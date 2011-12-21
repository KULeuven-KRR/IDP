/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*
* Use of this software is governed by the GNU LGPLv3.0 license
*
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CMD_SETASCURRENTOPTIONS_HPP_
#define CMD_SETASCURRENTOPTIONS_HPP_

#include "commandinterface.hpp"

class SetOptionsInference: public TypedInference<LIST(Options*)> {
public:
	SetOptionsInference(): TypedInference("setascurrentoptions", "Sets the given options as the current options, used by all other commands.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		getGlobal()->setOptions(get<0>(args));
		return nilarg();
	}
};

#endif /* CMD_SETASCURRENTOPTIONS_HPP_ */
