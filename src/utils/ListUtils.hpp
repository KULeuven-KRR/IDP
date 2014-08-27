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

#include <vector>
#include <set>
#include <map>
#include <stddef.h>


template<typename C1, typename C2>
void addAll(C1& list, const C2& toadd){
	for(const auto& elem: toadd){
		list.insert(elem);
	}
}

// Support for deleting lists of pointer elements
template<typename T>
void deleteList(std::vector<T*>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		if (*i != nullptr) {
			delete (*i);
		}
	}
	l.clear();
}

template<typename T>
void deleteList(std::vector<std::vector<T*>>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		deleteList(*i);
	}
	l.clear();
}

template<typename T>
void deleteList(std::set<T*>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		if (*i != nullptr) {
			delete (*i);
		}
	}
	l.clear();
}

template<typename T, typename K>
void deleteList(std::map<K, T*>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		if ((*i).second != nullptr) {
			delete ((*i).second);
		}
	}
	l.clear();
}

template<typename T, typename K>
void deleteList(std::map<K, std::map<K, std::vector<T*> > >& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		for (auto j = (*i).second.cbegin(); j != (*i).second.cend(); ++j) {
			for (auto k = (*j).second.cbegin(); k != (*j).second.cend(); ++k) {
				if ((*k).second != nullptr) {
					delete ((*k).second);
				}
			}
		}
	}
	l.clear();
}

template<typename Elem, class List>
bool contains(const List& l, const Elem& e) {
	return l.find(e) != l.cend();
}
template<typename Elem>
bool contains(const std::vector<Elem>& l, const Elem& e) {
	for(auto elem:l){
		if(elem==e){
			return true;
		}
	}
	return false;
}



/**
 * Check whether the container has an element that returns true for the function
 */
template<class List, class FunctionFromElementToBool>
bool hasElem(const List& l, const FunctionFromElementToBool& func) {
	for (auto el : l) {
		if (func(el)) {
			return true;
		}
	}
	return false;
}


/**
 * Returns true if the contents of the first list are a subset or equal to the contents of the second
 */
template<class List, class List2>
bool isSubset(const List& smaller, const List2& larger){
	for (auto elem : smaller) {
		if (not contains(larger, elem)) {
			return false;
		}
	}
	return true;
}

/*
 * Inserts elements at the end of a list.
 * MUCH FASTER than e.g. vector::insert!
 */
template<class List, class List2>
void insertAtEnd(List& list, const List2& addition){
	auto i=list.size();
	list.resize(list.size()+addition.size());
	for(const auto& lit:addition){
		list[i]=lit;
		i++;
	}
}

template<class Elem, class Comparator>
std::set<Elem, Comparator> getComplement(const std::set<Elem, Comparator>& total, const std::set<Elem, Comparator>& partition){
	std::set<Elem, Comparator> complement;
	for(auto elem:total){
		if(not contains(partition, elem)){
			complement.insert(elem);
		}
	}
	return complement;
}

template<class Elem>
std::set<Elem> getSet(const std::vector<Elem>& list){
	std::set<Elem> setoflist;
	for(auto elem: list){
		setoflist.insert(elem);
	}
	return setoflist;
}

template<class Elem, class Comparator>
std::set<Elem, Comparator> getSet(const std::vector<Elem>& list){
	std::set<Elem, Comparator> setoflist;
	for(auto elem: list){
		setoflist.insert(elem);
	}
	return setoflist;
}
