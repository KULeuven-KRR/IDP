#ifndef GROUNDUTILS_HPP_
#define GROUNDUTILS_HPP_

#include "commontypes.hpp"
#include "inferences/grounding/Utils.hpp"

template<typename DomElemList, typename DomInstList>
void overwriteVars(DomElemList& originst, const DomInstList& freevarinst){
	for(auto var2domelem=freevarinst.cbegin(); var2domelem<freevarinst.cend(); ++var2domelem){
		originst.push_back(var2domelem->first->get());
		(*var2domelem->first)=var2domelem->second;
	}
}

template<typename DomElemList, typename DomInstList>
void restoreOrigVars(DomElemList& originst, const DomInstList& freevarinst){
	for(uint i=0; i<freevarinst.size(); ++i){
		(*freevarinst[i].first)=originst[i];
	}
}

#endif /* GROUNDUTILS_HPP_ */
