/************************************
	GroundUtils.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUNDUTILS_HPP_
#define GROUNDUTILS_HPP_

#include "ground.hpp"

template<class DomElemList>
void overwriteVars(DomElemList& originst, const dominstlist& freevarinst){
	for(auto var2domelem=freevarinst.cbegin(); var2domelem<freevarinst.cend(); ++var2domelem){
		originst.push_back(var2domelem->first->get());
		(*var2domelem->first)=var2domelem->second;
	}
}

template<class DomElemList>
void restoreOrigVars(DomElemList& originst, const dominstlist& freevarinst){
	for(uint i=0; i<freevarinst.size(); ++i){
		(*freevarinst[i].first)=originst[i];
	}
}

#endif /* GROUNDUTILS_HPP_ */
