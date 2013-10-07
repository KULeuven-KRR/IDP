/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

class Theory;
class Structure;
class Query;

class AtomQuerying {
public:
	static bool doSolveAtomQuery(Query* p, Theory* t, Structure* s) {
		AtomQuerying c;
		return c.queryAtom(p, t, s);
	}

private:
	bool queryAtom(Query* p, Theory* t, Structure* s) const;
};
