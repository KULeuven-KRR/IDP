/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <set>
#include "structure.hpp"
#include "internalargument.hpp"

std::set<SortTable*> domains;

void addToGarbageCollection(SortTable* table){
	domains.insert(table);
}

void garbageCollect(SortTable* table){
	auto it = domains.find(table);
	if(it != domains.cend()) {
		delete(*it);
		domains.erase(table);
	}
}
