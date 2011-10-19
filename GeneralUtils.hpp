/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */
#ifndef GENUTILS_HPP_
#define GENUTILS_HPP_

#include <vector>
#include <map>
#include <exception>
#include <fstream>

#ifdef __GXX_EXPERIMENTAL_CXX0X__
	#include <memory>
	template<class T>
	struct sharedptr{
		typedef std::shared_ptr<T> ptr;
	};
#else
	#include <tr1/memory>
	template<class T>
	struct sharedptr{
		typedef std::tr1::shared_ptr<T> ptr;
	};
#endif

// Support for deleting lists of pointer elements
template<class T>
void deleteList(std::vector<T*>& l){
	for(auto i=l.cbegin(); i!=l.cend(); ++i){
		if(*i!=NULL){
			delete(*i);
		}
	}
	l.clear();
}

template<class T>
void deleteList(std::vector<std::vector<T*> >& l){
	for(auto i=l.cbegin(); i!=l.cend(); ++i){
		deleteList(*i);
	}
	l.clear();
}

template<class T, class K>
void deleteList(std::map<K, T*>& l){
	for(auto i=l.cbegin(); i!=l.cend(); ++i){
		if((*i).second!=NULL){
			delete((*i).second);
		}
	}
	l.clear();
}

template<class T, class K>
void deleteList(std::map<K, std::map<K, std::vector<T*> > >& l){
	for(auto i=l.cbegin(); i!=l.cend(); ++i){
		for(auto j=(*i).second.cbegin(); j!=(*i).second.cend(); ++j){
			for(auto k=(*j).second.cbegin(); k!=(*j).second.cend(); ++k){
				if((*k).second!=NULL){
					delete((*k).second);
				}
			}
		}
	}
	l.clear();
}

template<class List, class Elem>
bool contains(const List& l, const Elem& e){
	return l.find(e)!=l.cend();
}

template<class T>
bool fileIsReadable(T* filename) { //quick and dirty
	std::ifstream f(filename, std::ios::in);
	bool exists = f.is_open();
	if(exists){
		f.close();
	}
	return exists;
}

#endif /* GENUTILS_HPP_ */
