/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GroundUtils.hpp"

#include "GlobalData.hpp"

bool useLazyGrounding() {
	return getOption(TSEITINDELAY) || getOption(SATISFIABILITYDELAY);
}

bool useUFSAndOnlyIfSem() {
	// Correct because when using sat delay, duplicate model detection is done based on the input voc, while otherwise it is done on all literals and
	// so equivalence should be preserved.
	// FIXME guarantee this in a more general way!
	return false;
	//return (getOption(NBMODELS)==1 && getOption(TSEITINDELAY)) || getOption(SATISFIABILITYDELAY);
}
