/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDUTILS_HPP_
#define GROUNDUTILS_HPP_

#include "commontypes.hpp"
#include "inferences/grounding/Utils.hpp"

template<typename DomElemList, typename DomInstList>
void overwriteVars(DomElemList& originst, const DomInstList& freevarinst) {
	for (auto var2domelem = freevarinst.cbegin(); var2domelem < freevarinst.cend(); ++var2domelem) {
		originst.push_back(var2domelem->first->get());
		(*var2domelem->first) = var2domelem->second;
	}
}

template<typename DomElemList, typename DomInstList>
void restoreOrigVars(DomElemList& originst, const DomInstList& freevarinst) {
	for (unsigned int i = 0; i < freevarinst.size(); ++i) {
		(*freevarinst[i].first) = originst[i];
	}
}

#endif /* GROUNDUTILS_HPP_ */
