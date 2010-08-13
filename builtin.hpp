/************************************
	builtin.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUILTIN_H
#define BUILTIN_H

#include "vocabulary.hpp"
#include "structure.hpp"

/* 
 * The standard builtin vocabulary
 *		sorts:			nat, int, float, char, string
 *		predicates:		=/2, </2, >/2, SUCC/2, 
 *		functions:		+/2, -/2, * /2, //2, ^/2, %/2, abs/1, -/1, MIN/0, MAX/0
 */
class StdBuiltin : public Vocabulary {
	public:
		// Constructor
		StdBuiltin();
};

#endif
