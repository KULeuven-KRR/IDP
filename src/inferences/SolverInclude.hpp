/************************************
	SolverInclude.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SOLVERINCLUDE_HPP_
#define SOLVERINCLUDE_HPP_

#include "Space.hpp"
#include "Tasks.hpp"
#include "SearchMonitor.hpp"
#include "Translator.hpp"
#include "Constraints.hpp"

typedef MinisatID::Space PCSolver;
typedef MinisatID::Translator PCPrinter;
typedef MinisatID::ModelExpand PCModelExpand;
typedef MinisatID::UnitPropagate PCUnitPropagate;
typedef MinisatID::PropAndBackMonitor SearchMonitor;

#endif /* SOLVERINCLUDE_HPP_ */
