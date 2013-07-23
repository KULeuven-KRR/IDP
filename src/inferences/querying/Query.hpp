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

#ifndef QUERY_HPP_
#define QUERY_HPP_

class PredTable;
class Query;
class Structure;

class Querying {
public:
	static PredTable* doSolveQuery(Query* q, Structure* s) {
		Querying c;
		return c.solveQuery(q, s);
	}

private:
	PredTable* solveQuery(Query* q, Structure* s) const;
};

#endif /* QUERY_HPP_ */
