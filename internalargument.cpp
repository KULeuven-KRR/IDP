/************************************
	internalargument.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
