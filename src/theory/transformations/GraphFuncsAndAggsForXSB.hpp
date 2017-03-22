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

#include "GraphFuncsAndAggs.hpp"
#include "parseinfo.hpp"
#include "commontypes.hpp"
#include "utils/CPUtils.hpp"
#include "IncludeComponents.hpp"

class Structure;

/**
 * Graph all direct occurrences of functions in equality and aggregates in any comparison in a predform,
 * unless they can be turned into cp (and it is enabled)
 */
class GraphFuncsAndAggsForXSB: public GraphFuncsAndAggs {
	VISITORFRIENDS()

public:
	template<typename T>
	T execute(T t, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestAll = true, bool cpsupport = false, Context c = Context::POSITIVE) {
		return GraphFuncsAndAggs::execute(t, str, definedsymbols, unnestAll, cpsupport, c);
	}

protected:
	bool wouldGraph(Term*) const;
};
