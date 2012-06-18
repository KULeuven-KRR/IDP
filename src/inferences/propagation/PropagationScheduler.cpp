/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#include "PropagationScheduler.hpp"


/****************************
 Propagation scheduler
 ****************************/

void FOPropScheduler::add(FOPropagation* propagation) {
	// TODO: don't schedule a propagation that is already on the queue
	_queue.push(propagation);
}

bool FOPropScheduler::hasNext() const {
	return (not _queue.empty());
}

FOPropagation* FOPropScheduler::next() {
	FOPropagation* propagation = _queue.front();
	_queue.pop();
	return propagation;
}

