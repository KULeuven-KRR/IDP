/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef IDP_LISTUTILS_HPP_
#define IDP_LISTUTILS_HPP_

#include <vector>
#include <set>
#include <map>

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
	for (auto i = l.cbegin(); i != l.cend(); ++i){
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

template<typename List, typename Elem>
bool contains(const List& l, const Elem& e) {
	return l.find(e) != l.cend();
}

#endif /* IDP_LISTUTILS_HPP_ */
