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

#ifndef CMD_SETASCURRENTOPTIONS_HPP_
#define CMD_SETASCURRENTOPTIONS_HPP_

#include "commandinterface.hpp"
#include "options.hpp"


class SetOptionsInference: public OptionsBase {
public:
	SetOptionsInference()
			: OptionsBase("setascurrentoptions", "Sets the given options as the current options, used by all other commands.") {
		setNameSpace(getOptionsNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		getGlobal()->setOptions(get<0>(args));
		return nilarg();
	}
};


class NewOptionsInference: public EmptyBase {
public:
	NewOptionsInference()
			: EmptyBase("newoptions", "Create new options, equal to the standard options.") {
		setNameSpace(getOptionsNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>&) const {
		auto opts = new Options();
		return InternalArgument(opts);
	}
};

class GetOptionsInference: public EmptyBase {
public:
	GetOptionsInference()
			: EmptyBase("getoptions", "Get the current options.") {
		setNameSpace(getOptionsNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>&) const {
		auto opts = getGlobal()->getOptions();
		return InternalArgument(opts);
	}
};

#endif /* CMD_SETASCURRENTOPTIONS_HPP_ */
