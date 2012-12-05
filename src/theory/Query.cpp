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

#include "IncludeComponents.hpp"
#include "Query.hpp"
#include "utils/ListUtils.hpp"
#include <vector>
#include <sstream>

using namespace std;

Query::Query(std::string name, const std::vector<Variable*>& vars, Formula* q, const ParseInfo& pi)
		: 	_variables(vars),
			_query(q),
			_pi(pi),
			_name(name),
			_vocabulary(NULL) {
	//NOTE: these things are checked at parsetime. However, they are checked here again for in case queries are constructed directly (without going through parser)
	varset queriedvars;
	for(auto var: vars){
		queriedvars.insert(var);
	}
	for(auto freevar: q->freeVars()){
		if(not contains(queriedvars, freevar)){
			stringstream ss;
			ss <<"The query {" <<print(vars) <<" : " <<print(q) <<"} contains the completely free variable " <<print(freevar) <<" which is not allowed.\n";
			throw IdpException(ss.str());
		}
	}
}

void Query::recursiveDelete() {
	_query->recursiveDelete(); //also deletes the variables.
}

void Query::put(std::ostream& stream) const {
	stream << "Query " << name() << " represents tuples of assignments to " << toString(variables()) << " satisfying " << toString(query()) << "\n";
}
