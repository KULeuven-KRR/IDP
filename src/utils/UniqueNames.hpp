#pragma once
#include "common.hpp"
#include "IncludeComponents.hpp"

#include <map>
#include <sstream>

template<class T>
class UniqueNames{
private:
	int id;
	std::map<T,int> tounique;
	std::map<int,T> fromunique;
public:
	UniqueNames(){
		id = 0;
	}
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

template<class T>
class UniqueStringNames{
private:
	int id;
	std::map<T,std::string> tounique;
	std::map<std::string,T> fromunique;
public:
	UniqueStringNames(){
		id = 0;
	}
	T getOriginal(std::string id){
		auto it = fromunique.find(id);
		if(it==fromunique.end()){
			throw IdpException("Invalid code path");
		}else{
			return it->second;
		}
	}
	std::string getUnique(T elem){
		auto it = tounique.find(elem);
		if(it==tounique.end()){
			id++;
			std::stringstream ss;
			ss <<print(elem) <<"_" <<id;
			tounique[elem]=ss.str();
			fromunique[ss.str()]=elem;
			return ss.str();
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

