/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef IDP_GENUTILS_HPP_
#define IDP_GENUTILS_HPP_

#include <vector>
#include <map>
#include <exception>
#include <fstream>

#include <memory>
template<class T>
struct sharedptr{
	typedef std::shared_ptr<T> ptr;
};

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

template<typename T>
void replaceAll(T& str, const T& from, const T& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

#endif /* GENUTILS_HPP_ */
