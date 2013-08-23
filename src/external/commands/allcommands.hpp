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

//TODO add support for easily using these inferences directly in lua, by also providing a help/usage text and replacing idp_intern. with something easier

#include <vector>
#include "commandinterface.hpp"
#include <memory>

// Important: pointer owner is transferred to receiver!
const std::vector<std::shared_ptr<Inference>>& getAllInferences();
