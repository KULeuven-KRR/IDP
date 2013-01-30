/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include <string>
#include <sstream>
#include <utility>
#include "errorhandling/IdpException.hpp"

#ifndef NDEBUG
#define Assert(condition) { if(!(condition)){ std::stringstream ss; ss << "ASSERT FAILED: " << #condition << " @ " << __FILE__ << " (" << __LINE__ << ")"; throw AssertionException(ss.str());} }
#else
#define Assert(x) do { (void)sizeof(x); } while(0)
#endif
