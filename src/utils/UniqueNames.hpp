#pragma once
#include "common.hpp"
#include "IncludeComponents.hpp"

#include <map>

template<class T>
class UniqueNames{
private:
	int id;
	std::map<T,int> tounique;
	std::map<int,T> fromunique;
public:
	T getOriginal(int id){
		auto it = fromunique.find(id);
		if(it==fromunique.end()){
			throw IdpException("Invalid code path");
		}else{
			return it->second;
		}
	}
	int getUnique(T elem){
		auto it = tounique.find(elem);
		if(it==tounique.end()){
			id++;
			tounique[elem]=id;
			fromunique[id]=elem;
			return id;
		}else{
			return it->second;
		}
	}
};

std::string createName();

template<class T>
const DomainElement* mapName(T elem, UniqueNames<T>& uniquenames) {
	return createDomElem(uniquenames.getUnique(elem));
}

