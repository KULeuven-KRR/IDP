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

#include "LazyInst.hpp"
#include "LazyDisjunctiveGrounders.hpp"

void LazyInstantiation::notifyTheoryOccurrence(TsType type){
	grounder->notifyTheoryOccurrence(this, type);
}

void LazyInstantiation::notifyGroundingRequested(int ID, bool groundall, bool& stilldelayed){
	grounder->notifyGroundingRequested(ID, groundall, this, stilldelayed);
}
