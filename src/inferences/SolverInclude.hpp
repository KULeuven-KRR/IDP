/************************************
	SolverInclude.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SOLVERINCLUDE_HPP_
#define SOLVERINCLUDE_HPP_

#include "external/Space.hpp"
#include "external/DataAndInference.hpp"
#include "external/SearchMonitor.hpp"

typedef MinisatID::Space PCSolver;
typedef MinisatID::ModelExpand PCModelExpand;
typedef MinisatID::UnitPropagate PCUnitPropagation;
typedef MinisatID::Transform PCTransform;
typedef MinisatID::PropAndBackMonitor SearchMonitor;

#endif /* SOLVERINCLUDE_HPP_ */
