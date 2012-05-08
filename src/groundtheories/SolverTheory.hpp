/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef SOLVERTHEORY_HPP_
#define SOLVERTHEORY_HPP_

#include "GroundTheory.hpp"
#include "SolverPolicy.hpp"

typedef GroundTheory<SolverPolicy<PCSolver> > SolverTheory;

#endif /* SOLVERTHEORY_HPP_ */
