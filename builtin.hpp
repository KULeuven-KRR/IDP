/************************************
	builtin.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include "vocabulary.hpp"
#include "structure.hpp"

/* 
 * The standard builtin vocabulary
 *		sorts:			nat, int, float, char, string
 *		predicates:		=/2, </2, >/2, SUCC/2, 
 *		functions:		+/2, -/2, * /2, //2, ^/2, %/2, abs/1, -/1, MIN/0, MAX/0
 */
class StdBuiltin : public Vocabulary {
	private:
		static StdBuiltin* _instance;	
		StdBuiltin();
	public:
		static StdBuiltin* instance();
		~StdBuiltin() { }
};


#endif
