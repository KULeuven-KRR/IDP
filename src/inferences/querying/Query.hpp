/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef QUERY_HPP_
#define QUERY_HPP_

class PredTable;
class Query;
class AbstractStructure;

class Querying {
public:
	static PredTable* doSolveQuery(Query* q, AbstractStructure* s) {
		Querying c;
		return c.solveQuery(q, s);
	}

private:
	PredTable* solveQuery(Query* q, AbstractStructure* s) const;
};

#endif /* QUERY_HPP_ */
