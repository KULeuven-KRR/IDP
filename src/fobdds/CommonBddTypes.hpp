/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef COMMONBDDTYPES_HPP_
#define COMMONBDDTYPES_HPP_

#include "common.hpp"

enum class AtomKernelType {
	AKT_CT, AKT_CF, AKT_TWOVALUED
};
enum class KernelOrderCategory {
	STANDARDCATEGORY, DEBRUIJNCATEGORY, TRUEFALSECATEGORY
};

inline bool operator<(const KernelOrderCategory x,const  KernelOrderCategory y) {
	if (x == KernelOrderCategory::TRUEFALSECATEGORY) {
		return false;
	} else if (x == KernelOrderCategory::DEBRUIJNCATEGORY) {
		return y == KernelOrderCategory::TRUEFALSECATEGORY;
	} else {
		return y != KernelOrderCategory::STANDARDCATEGORY;
	}
}

//Some random ordering for AtomKernelType to be used in maps
inline bool operator<(const AtomKernelType x,const  AtomKernelType y) {
	if (x == AtomKernelType::AKT_TWOVALUED) {
		return false;
	} else if (x == AtomKernelType::AKT_CF) {
		return y == AtomKernelType::AKT_TWOVALUED;
	} else {
		return y != AtomKernelType::AKT_CT;
	}
}

#endif /* COMMONBDDTYPES_HPP_ */