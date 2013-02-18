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

template<typename T, typename T2>
void addAll(std::map<T, T2>& list, const std::map<T, T2>& toadd){
	for(const auto& elem: toadd){
		list.insert(elem);
	}

}

// Support for deleting lists of pointer elements
template<typename T>
void deleteList(std::vector<T*>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		if (*i != NULL) {
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
		if (*i != NULL) {
			delete (*i);
		}
	}
	l.clear();
}

template<typename T, typename K>
void deleteList(std::map<K, T*>& l) {
	for (auto i = l.cbegin(); i != l.cend(); ++i) {
		if ((*i).second != NULL) {
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
				if ((*k).second != NULL) {
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
