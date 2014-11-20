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

#ifndef SOLVERINCLUDE_HPP_
#define SOLVERINCLUDE_HPP_

#include "Space.hpp"
#include "Tasks.hpp"
#include "ModelIterationTask.hpp"
#include "SearchMonitor.hpp"
#include "Translator.hpp"
#include "Constraints.hpp"

#include "ECNFPrinter.hpp"
#include "FlatZincRewriter.hpp"

class InteractivePrintMonitor;

typedef MinisatID::Space PCSolver;
typedef MinisatID::Translator PCPrinter;
typedef MinisatID::UnitPropagate PCUnitPropagate;
typedef MinisatID::PropAndBackMonitor SearchMonitor;
//typedef MinisatID::RealECNFPrinter<InteractivePrintMonitor> ECNFPrinter; TODO should become (gcc 4.6):
/*
 * template<class Stream>
 * using ECNFPrinter = MinisatID::RealECNFPrinter<Stream>
 */
typedef MinisatID::FlatZincRewriter<std::ostream> FZPrinter;

#endif /* SOLVERINCLUDE_HPP_ */
